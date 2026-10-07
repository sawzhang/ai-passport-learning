import asyncio
import importlib.util
import os
import pty
import select
import threading
from pathlib import Path
import subprocess
import tempfile
import unittest
import sys

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('mac_proxy', ROOT / 'mac_proxy.py')
bridge = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(bridge)


class ProtocolTests(unittest.TestCase):
    def test_c_transport(self):
        component = ROOT / 'overlay/esp32/components/passport_proxy'
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'proxy'
            subprocess.run(['cc', '-std=c11', '-D_POSIX_C_SOURCE=200809L', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-pthread',
                            '-I' + str(ROOT / 'tests/proxy_stubs'), '-I' + str(component),
                            '-I' + str(component / 'include'), str(ROOT / 'tests/test_proxy_runtime.c'),
                            str(component / 'passport_proxy.c'), str(component / 'proxy_protocol.c'),
                            '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

    def test_usb_configuration_and_readback(self):
        master, slave = pty.openpty()
        commands = []

        def device():
            pending = b''
            for response in (b'@proxy.saved reboot to reconnect\n', b'@proxy 192.168.1.10:18087\n'):
                while b'\n' not in pending:
                    if not select.select([master], [], [], 3)[0]:
                        return
                    pending += os.read(master, 4096)
                command, pending = pending.split(b'\n', 1)
                commands.append(command)
                os.write(master, b'unrelated private log\n' + response)

        worker = threading.Thread(target=device)
        worker.start()
        try:
            result = subprocess.run([sys.executable, str(ROOT / 'configure_proxy.py'),
                                     os.ttyname(slave), '192.168.1.10:18087'],
                                    capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(commands, [b'>proxy=192.168.1.10:18087', b'>proxy.status'])
            self.assertNotIn('private log', result.stdout)
            self.assertIn('@proxy 192.168.1.10:18087', result.stdout)
        finally:
            worker.join(timeout=4)
            os.close(master); os.close(slave)


class BridgeTests(unittest.IsolatedAsyncioTestCase):
    async def test_bidirectional_half_close(self):
        payload = bytes(range(256)) * 256

        async def upstream(reader, writer):
            data = await reader.read()
            self.assertEqual(data, payload)
            writer.write(data[::-1])
            await writer.drain()
            writer.close()
            await writer.wait_closed()

        remote = await asyncio.start_server(upstream, '127.0.0.1', 0)
        remote_port = remote.sockets[0].getsockname()[1]
        relay = await asyncio.start_server(
            lambda r, w: bridge.bridge(r, w, '127.0.0.1', remote_port, [], asyncio.Semaphore(16)),
            '127.0.0.1', 0)
        try:
            reader, writer = await asyncio.open_connection('127.0.0.1', relay.sockets[0].getsockname()[1])
            writer.write(payload)
            await writer.drain()
            writer.write_eof()
            result = await asyncio.wait_for(reader.read(), 3)
            self.assertEqual(result, payload[::-1])
            writer.close()
            await writer.wait_closed()
        finally:
            relay.close(); remote.close()
            await relay.wait_closed(); await remote.wait_closed()

    async def test_upstream_outage_then_recovery(self):
        remote = await asyncio.start_server(lambda r, w: None, '127.0.0.1', 0)
        port = remote.sockets[0].getsockname()[1]
        remote.close(); await remote.wait_closed()
        slots = asyncio.Semaphore(1)
        relay = await asyncio.start_server(
            lambda r, w: bridge.bridge(r, w, '127.0.0.1', port, [], slots), '127.0.0.1', 0)
        async def respond(r, w):
            w.write(b'recovered'); await w.drain(); w.close(); await w.wait_closed()
        try:
            r, w = await asyncio.open_connection('127.0.0.1', relay.sockets[0].getsockname()[1])
            self.assertEqual(await asyncio.wait_for(r.read(), 2), b'')
            w.close(); await w.wait_closed()
            remote = await asyncio.start_server(respond, '127.0.0.1', port)
            r, w = await asyncio.open_connection('127.0.0.1', relay.sockets[0].getsockname()[1])
            self.assertEqual(await asyncio.wait_for(r.read(), 2), b'recovered')
            w.close(); await w.wait_closed()
        finally:
            relay.close(); remote.close()
            await relay.wait_closed(); await remote.wait_closed()

    async def test_one_way_activity_keeps_tunnel_alive(self):
        async def upstream(r, w):
            for _ in range(8):
                w.write(b'x'); await w.drain(); await asyncio.sleep(.04)
            w.close(); await w.wait_closed()
        remote = await asyncio.start_server(upstream, '127.0.0.1', 0)
        relay = await asyncio.start_server(lambda r, w: bridge.bridge(
            r, w, '127.0.0.1', remote.sockets[0].getsockname()[1], [], asyncio.Semaphore(1),
            idle_timeout=.15), '127.0.0.1', 0)
        try:
            r, w = await asyncio.open_connection('127.0.0.1', relay.sockets[0].getsockname()[1])
            self.assertEqual(await asyncio.wait_for(r.read(), 2), b'x'*8)
            w.close(); await w.wait_closed()
        finally:
            relay.close(); remote.close()
            await relay.wait_closed(); await remote.wait_closed()

    async def test_disallowed_client(self):
        relay = await asyncio.start_server(
            lambda r, w: bridge.bridge(r, w, '127.0.0.1', 1, ['192.168.1.5'], asyncio.Semaphore(16)),
            '127.0.0.1', 0)
        try:
            reader, writer = await asyncio.open_connection('127.0.0.1', relay.sockets[0].getsockname()[1])
            self.assertEqual(await asyncio.wait_for(reader.read(), 1), b'')
            writer.close(); await writer.wait_closed()
        finally:
            relay.close(); await relay.wait_closed()


if __name__ == '__main__':
    unittest.main()
