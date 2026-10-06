#!/usr/bin/env python3
"""Capture real Passport LCD flush pixels over USB, without Pillow or board framebuffer."""
import argparse
import base64
import os
from pathlib import Path
import select
import struct
import termios
import time
import zlib

class Frame:
    def __init__(self):
        self.width = self.height = 0
        self.pixels = self.seen = None
        self.complete = False

    def feed(self, line):
        if not line.startswith('PXS '): return False
        parts = line.split()
        if len(parts) < 2: raise ValueError("Bad pixel record")
        if parts[1] == 'BEGIN':
            if len(parts) != 4: raise ValueError('Bad frame header')
            w, h = map(int, parts[2:])
            if not (1 <= w <= 1024 and 1 <= h <= 1024): raise ValueError('Frame bounds')
            self.width, self.height = w, h
            self.pixels = bytearray(w * h * 2)
            self.seen = bytearray(w * h)
            self.complete = False
        elif parts[1] == 'ABORT':
            raise ValueError('Device aborted capture')
        elif parts[1] == 'END':
            if self.seen is None or not all(self.seen): raise ValueError('Incomplete frame')
            self.complete = True
            return True
        else:
            if len(parts) != 5 or self.pixels is None: raise ValueError('Pixel row without header')
            y, x, count = map(int, parts[1:4])
            raw = base64.b64decode(parts[4], validate=True)
            if not (0 <= y < self.height and 0 <= x < self.width and 1 <= count <= self.width - x):
                raise ValueError('Pixel bounds')
            if len(raw) != count * 2: raise ValueError('Pixel count mismatch')
            offset = y * self.width + x
            self.pixels[offset*2:(offset+count)*2] = raw
            self.seen[offset:offset+count] = b'\1' * count
        return False

    def save(self, path):
        if not self.complete: raise ValueError('Incomplete frame')
        rows = bytearray()
        for y in range(self.height):
            rows.append(0)
            for x in range(self.width):
                pixel = struct.unpack_from('<H', self.pixels, 2 * (y * self.width + x))[0]
                rows.extend(((pixel >> 11) * 255 // 31, ((pixel >> 5) & 63) * 255 // 63, (pixel & 31) * 255 // 31))
        def chunk(kind, data):
            return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
        png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', self.width, self.height, 8, 2, 0, 0, 0))
        png += chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b'')
        fd = os.open(path, os.O_CREAT | os.O_WRONLY | os.O_TRUNC, 0o600)
        with os.fdopen(fd, 'wb') as stream: stream.write(png)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('port')
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    original = termios.tcgetattr(fd)
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[2] &= ~termios.HUPCL
    attrs[2] |= termios.CLOCAL | termios.CREAD
    attrs[4] = attrs[5] = termios.B115200
    frame = Frame(); pending = b''
    try:
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        os.write(fd, b'p')
        deadline = time.monotonic() + 20
        while time.monotonic() < deadline:
            if not select.select([fd], [], [], .1)[0]: continue
            pending += os.read(fd, 16384)
            while b'\n' in pending:
                line, pending = pending.split(b'\n', 1)
                if frame.feed(line.decode(errors='replace').strip()):
                    frame.save(args.output)
                    print(f'Captured {frame.width}x{frame.height} real LCD pixels: {args.output}')
                    return
        raise SystemExit('Capture timed out; no complete frame saved')
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, original)
        os.close(fd)

if __name__ == '__main__': main()
