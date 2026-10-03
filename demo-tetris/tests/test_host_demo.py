"""Exercise the actual terminal executable through a pseudo-terminal."""
import os
import pty
import select
import subprocess
import termios
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
master, slave = pty.openpty()
before = termios.tcgetattr(slave)
process = subprocess.Popen([str(ROOT / 'build/tetris-host')],
                           stdin=slave, stdout=slave, stderr=slave)


def read_for(seconds):
    end = time.monotonic() + seconds
    output = b''
    while time.monotonic() < end:
        ready, _, _ = select.select([master], [], [], max(0, end - time.monotonic()))
        if ready:
            output += os.read(master, 65536)
    return output


def wait_for(text):
    end = time.monotonic() + 2
    output = b''
    while text not in output and time.monotonic() < end:
        output += read_for(0.05)
    assert text in output, (text, output[-200:])


try:
    wait_for(b'PLAYING')
    os.write(master, b'p')
    wait_for(b'PAUSED')
    read_for(.05)
    os.write(master, b'adw ')
    assert not read_for(.1), 'Paused input changed the display'
    os.write(master, b'p')
    wait_for(b'PLAYING')
    os.write(master, b'adw ')
    wait_for(b'Score')
    read_for(.1)
    os.write(master, b'r')
    wait_for(b'Score 0 ')
    os.write(master, b'q')
    process.wait(timeout=2)
    assert process.returncode == 0
    after = termios.tcgetattr(slave)
    # macOS sets the transient PENDIN bit when switching back to canonical mode.
    # Compare every other terminal flag, speed and control character exactly.
    pending = getattr(termios, 'PENDIN', 0)
    before[3] &= ~pending
    after[3] &= ~pending
    assert before == after, 'Terminal settings were not restored'
    print('PASS: PTY startup, pause, frozen inputs, resume, controls, restart, exit, terminal restoration')
finally:
    if process.poll() is None:
        process.terminate()
        process.wait(timeout=2)
    os.close(master)
    os.close(slave)
