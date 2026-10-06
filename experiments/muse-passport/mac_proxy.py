#!/usr/bin/env python3
"""LAN TCP bridge to the Mac's existing HTTP proxy. No TLS termination."""
import argparse
import asyncio
import ipaddress


async def pump(reader, writer):
    while data := await asyncio.wait_for(reader.read(16384), 300):
        writer.write(data)
        await writer.drain()
    if writer.can_write_eof():
        writer.write_eof()
        await writer.drain()


async def bridge(reader, writer, upstream_host, upstream_port, clients, slots):
    peer = writer.get_extra_info('peername')[0]
    address = ipaddress.ip_address(peer)
    upstream = None
    try:
        if clients and peer not in clients:
            return
        if not clients and not (address.is_private or address.is_loopback):
            return
        if slots.locked():
            return
        async with slots:
            remote, upstream = await asyncio.wait_for(
                asyncio.open_connection(upstream_host, upstream_port), 5)
            pumps = [asyncio.create_task(pump(reader, upstream)), asyncio.create_task(pump(remote, writer))]
            try:
                await asyncio.gather(*pumps)
            finally:
                for task in pumps:
                    task.cancel()
                await asyncio.gather(*pumps, return_exceptions=True)
    except (OSError, asyncio.TimeoutError):
        pass
    finally:
        for stream in (upstream, writer):
            if stream:
                stream.close()
                try:
                    await stream.wait_closed()
                except OSError:
                    pass


async def serve(args):
    slots = asyncio.Semaphore(16)
    server = await asyncio.start_server(
        lambda r, w: bridge(r, w, args.upstream_host, args.upstream_port, args.allow_client, slots),
        args.listen, args.port)
    print(f'HTTP proxy bridge listening on {args.listen}:{args.port}; '
          f'upstream {args.upstream_host}:{args.upstream_port}', flush=True)
    async with server:
        await server.serve_forever()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--listen', default='0.0.0.0')
    parser.add_argument('--port', type=int, default=18087)
    parser.add_argument('--upstream-host', default='127.0.0.1')
    parser.add_argument('--upstream-port', type=int, default=1087)
    parser.add_argument('--allow-client', action='append', default=[],
                        help='Only accept this device IPv4; repeat for multiple devices.')
    args = parser.parse_args()
    if not 1 <= args.port <= 65535 or not 1 <= args.upstream_port <= 65535:
        parser.error('ports must be in 1..65535')
    for address in args.allow_client:
        ipaddress.IPv4Address(address)
    try:
        asyncio.run(serve(args))
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
