#!/usr/bin/env python3
"""Configure Muse's HTTP proxy over USB without changing Wi-Fi or pairing."""
import argparse
import ipaddress
import os
import select
import termios
import time


def endpoint(value):
    if value == 'off':
        return value
    try:
        host, port = value.split(':')
        ip = ipaddress.IPv4Address(host)
        if ip.is_unspecified or ip.is_loopback or ip.is_multicast or int(host.split('.')[0]) >= 224:
            raise ValueError()
        if not 1 <= int(port) <= 65535:
            raise ValueError()
        return f'{ip}:{int(port)}'
    except ValueError as error:
        raise argparse.ArgumentTypeError('use Mac LAN IPv4:port or off') from error


def exchange(fd, command):
    os.write(fd, ('>' + command + '\n').encode())
    deadline = time.monotonic() + 8
    pending = b''
    while time.monotonic() < deadline:
        readable, _, _ = select.select([fd], [], [], max(0, deadline - time.monotonic()))
        if not readable:
            break
        data = os.read(fd, 4096)
        if not data:
            raise RuntimeError('device disconnected')
        pending += data
        while b'\n' in pending:
            line, pending = pending.split(b'\n', 1)
            marker = line.find(b'@proxy')
            if marker >= 0:
                result = line[marker:].decode(errors='replace').strip()
                print(result)
                if result.startswith('@proxy.error') or 'error:' in result:
                    raise RuntimeError('device rejected proxy configuration')
                return result
        pending = pending[-4096:]
    raise RuntimeError('no proxy response: install proxy-enabled firmware and close other serial monitors')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('port', help='/dev/cu.usbmodem…')
    parser.add_argument('endpoint', nargs='?', type=endpoint, help='Mac LAN IPv4:port, or off; omit to query')
    args = parser.parse_args()
    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        attrs = termios.tcgetattr(fd)
        attrs[0] = attrs[1] = attrs[3] = 0
        attrs[2] &= ~(termios.HUPCL | termios.PARENB | termios.CSTOPB | termios.CSIZE)
        attrs[2] |= termios.CLOCAL | termios.CREAD | termios.CS8
        attrs[4] = attrs[5] = termios.B115200
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        # Drain stale data, without showing logs or device secrets.
        termios.tcflush(fd, termios.TCIFLUSH)
        if args.endpoint is not None:
            result = exchange(fd, 'proxy=' + args.endpoint)
            if result != '@proxy.saved reboot to reconnect':
                raise RuntimeError('unexpected configuration acknowledgment')
        result = exchange(fd, 'proxy.status')
        if args.endpoint is not None:
            expected = '@proxy ' + args.endpoint
            if result != expected:
                raise RuntimeError('saved proxy does not match requested endpoint')
            print('Restart the device to reconnect all Muse services with this setting.')
    finally:
        os.close(fd)


if __name__ == '__main__':
    main()
