#!/usr/bin/env python3
"""LAN TCP bridge to the Mac's HTTP proxy. No TLS termination or payload logging."""
import argparse
import asyncio
import ipaddress
import logging
from logging.handlers import RotatingFileHandler
import signal
import time

LOG = logging.getLogger('muse.proxy')


async def pump(reader, writer, activity=None):
    while data := await reader.read(16384):
        if activity is not None:
            activity[0] = time.monotonic()
        writer.write(data)
        await writer.drain()
    if writer.can_write_eof():
        writer.write_eof()
        await writer.drain()


async def idle_watch(activity, timeout):
    while True:
        remaining = timeout - (time.monotonic() - activity[0])
        if remaining <= 0:
            raise asyncio.TimeoutError('connection idle')
        await asyncio.sleep(remaining)


async def close_stream(writer):
    if writer is not None:
        writer.close()
        try:
            await asyncio.wait_for(writer.wait_closed(), 2)
        except (OSError, asyncio.TimeoutError):
            pass


async def bridge(reader, writer, upstream_host, upstream_port, clients, slots, idle_timeout=300):
    upstream = None
    jobs = []
    try:
        peer = writer.get_extra_info('peername')[0]
        address = ipaddress.ip_address(peer)
        if (clients and peer not in clients) or (not clients and not (address.is_private or address.is_loopback)):
            return
        if slots.locked():
            LOG.warning('connection rejected: capacity reached')
            return
        async with slots:
            remote, upstream = await asyncio.wait_for(
                asyncio.open_connection(upstream_host, upstream_port), 5)
            activity = [time.monotonic()]
            jobs = [asyncio.create_task(pump(reader, upstream, activity)),
                    asyncio.create_task(pump(remote, writer, activity))]
            transfer = asyncio.gather(*jobs)
            watchdog = asyncio.create_task(idle_watch(activity, idle_timeout))
            jobs += [transfer, watchdog]
            done, _ = await asyncio.wait([transfer, watchdog], return_when=asyncio.FIRST_COMPLETED)
            for task in done:
                task.result()
    except (OSError, asyncio.TimeoutError) as error:
        LOG.warning('connection closed: %s', type(error).__name__)
    except Exception:
        LOG.exception('connection handler failed; listener remains available')
    finally:
        for job in jobs:
            job.cancel()
        if jobs:
            await asyncio.gather(*jobs, return_exceptions=True)
        await asyncio.gather(close_stream(upstream), close_stream(writer))


async def serve(args):
    slots = asyncio.Semaphore(16)
    stop = asyncio.Event()
    loop = asyncio.get_running_loop()
    for sig in (signal.SIGINT, signal.SIGTERM):
        loop.add_signal_handler(sig, stop.set)
    server = await asyncio.start_server(
        lambda r, w: bridge(r, w, args.upstream_host, args.upstream_port, args.allow_client, slots),
        args.listen, args.port)
    LOG.info('listening on %s:%s; upstream %s:%s', args.listen, args.port, args.upstream_host, args.upstream_port)
    async with server:
        await stop.wait()
    LOG.info('stopping listener')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--listen', default='0.0.0.0')
    parser.add_argument('--port', type=int, default=18087)
    parser.add_argument('--upstream-host', default='127.0.0.1')
    parser.add_argument('--upstream-port', type=int, default=1087)
    parser.add_argument('--allow-client', action='append', default=[])
    parser.add_argument('--log-file', help='Rotating metadata-only log (1 MiB × 4 files)')
    args = parser.parse_args()
    if not 1 <= args.port <= 65535 or not 1 <= args.upstream_port <= 65535:
        parser.error('ports must be in 1..65535')
    for address in args.allow_client:
        ipaddress.IPv4Address(address)
    handler = RotatingFileHandler(args.log_file, maxBytes=1024*1024, backupCount=3) if args.log_file else logging.StreamHandler()
    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s', handlers=[handler])
    try:
        asyncio.run(serve(args))
    except KeyboardInterrupt:
        pass
    except Exception:
        LOG.exception('listener failed; supervisor should restart it')
        raise SystemExit(1)


if __name__ == '__main__':
    main()
