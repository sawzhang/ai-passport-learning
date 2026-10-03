"""Fetch the omitted upstream artwork and verify its exact Git blob identity."""
import hashlib
import json
from pathlib import Path
from urllib.request import urlopen

ROOT = Path(__file__).resolve().parents[1]
manifest = json.loads((ROOT / 'tools/source-assets.json').read_text())
for entry in manifest['files']:
    path = ROOT / entry['path']
    if path.exists():
        data = path.read_bytes()
    else:
        url = 'https://raw.githubusercontent.com/folotoy/ai-passport/' + manifest['revision'] + '/' + entry['path']
        with urlopen(url, timeout=60) as response:
            data = response.read()
    digest = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
    if digest != entry['sha']:
        raise SystemExit('Asset hash mismatch; preserving existing file: ' + entry['path'])
    if not path.exists():
        path.parent.mkdir(parents=True, exist_ok=True)
        # Exclusive creation prevents replacing a concurrently added user file.
        with path.open('xb') as output:
            output.write(data)
    print('Verified:', entry['path'])
