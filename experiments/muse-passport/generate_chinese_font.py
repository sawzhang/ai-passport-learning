#!/usr/bin/env python3
"""Regenerate the Flash-resident CJK asset from the pinned LVGL font source."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
ASSETS = ROOT / 'overlay/esp32/components/muse/fonts'
manifest = json.loads((ASSETS / 'manifest.json').read_text())
source = ROOT / '.work/muse-gadget-sdk/esp32/managed_components/lvgl__lvgl/tests/src/test_files/fonts/noto/NotoSansSC-Regular.ttf'
converter = ROOT / '.local/font-tools/node_modules/.bin/lv_font_conv'
package = converter.parent.parent / 'lv_font_conv/package.json'
if not package.exists() or json.loads(package.read_text())['version'] != '1.5.3':
    raise SystemExit('Install lv_font_conv@1.5.3 with npm --prefix experiments/muse-passport/.local/font-tools')
if hashlib.sha256(source.read_bytes()).hexdigest() != manifest['source_sha256']:
    raise SystemExit('Source font differs from the reviewed manifest')
output = ASSETS / 'passport_font_cjk_16.c'
subprocess.run([str(converter), '--font', str(source.relative_to(REPO)),
    '--range', manifest['ranges'], '--size', '16', '--bpp', '2', '--format', 'lvgl',
    '--no-compress', '--no-kerning', '--lv-font-name', 'passport_font_cjk_16',
    '--lv-include', 'lvgl.h', '--output', str(output.relative_to(REPO))], cwd=REPO, check=True)
output.write_text(output.read_text().rstrip() + '\n')
if hashlib.sha256(output.read_bytes()).hexdigest() != manifest['generated_sha256']:
    raise SystemExit('Generated font differs from the reviewed asset; inspect before updating the manifest')
print('Font reproduced with matching SHA-256')
