"""Read a bounded log window without intentionally resetting the device."""
import argparse
import time
from pathlib import Path
import serial

parser = argparse.ArgumentParser()
parser.add_argument('port')
parser.add_argument('--seconds', type=int, default=30)
parser.add_argument('--output', default='build/device-test.log')
args = parser.parse_args()
if not 1 <= args.seconds <= 3600:
    parser.error('--seconds must be between 1 and 3600')
port = args.port
output = Path(args.output)
output.parent.mkdir(parents=True, exist_ok=True)
connection = serial.Serial(port=None, baudrate=115200, timeout=.2)
connection.dtr = False
connection.rts = False
connection.port = port
connection.open()
print(f'Capturing {args.seconds} seconds of logs. Opening this USB serial port may reset the device. Full log stays local.', flush=True)
try:
    deadline = time.monotonic() + args.seconds
    with output.open('ab') as log:
        while time.monotonic() < deadline:
            data = connection.readline()
            if not data:
                continue
            log.write(data)
            log.flush()
            text = data.decode('utf-8', errors='replace').rstrip()
            if any(marker in text for marker in ['passport_games:', 'Guru Meditation', 'assert failed', 'Task watchdog']):
                print(text, flush=True)
finally:
    connection.close()
print('Capture ended. Flash/log success does not establish screen or physical-button acceptance.')
