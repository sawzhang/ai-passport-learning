#!/usr/bin/env python3
"""Validate Muse C3 flash artifacts before using the upstream flash helper."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

root = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--build', type=Path, default=root / '.work/muse-gadget-sdk/esp32/build-muse-folotoy-ai-passport')
args = parser.parse_args()
build = args.build.resolve()
source = root.parents[1] / 'firmware/tools/verify_firmware.py'
spec = importlib.util.spec_from_file_location('passport_image_verifier', source)
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)
expected = {'0x0': 'bootloader/bootloader.bin', '0x10000': 'partition_table/partition-table.bin', '0x1d000': 'ota_data_initial.bin', '0x20000': 'muse-gadget.bin'}
flasher = json.loads((build / 'flasher_args.json').read_text())
if flasher['extra_esptool_args']['chip'] != 'esp32c3' or flasher['flash_files'] != expected:
    raise SystemExit('Unexpected chip or flash layout; refuse flashing.')
config = (build / 'sdkconfig').read_text()
required = ['CONFIG_IDF_TARGET="esp32c3"', 'CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y', 'CONFIG_MUSE_BOARD_FOLOTOY_AI_PASSPORT=y']
for item in required:
    if item not in config:
        raise SystemExit('Missing required setting: ' + item)
for key in ['CONFIG_SECURE_BOOT', 'CONFIG_FLASH_ENCRYPTION_ENABLED', 'CONFIG_HOMEHUB_PAIRING_EFUSE_AUTH', 'CONFIG_NVS_ENCRYPTION', 'CONFIG_HOMEHUB_TUNNEL', 'CONFIG_HOMEHUB_OTA_ENABLED', 'CONFIG_SPIRAM']:
    if key + '=y' in config:
        raise SystemExit('Unsupported experimental setting: ' + key)
parts, md5 = module.parse_partition_table((build / expected['0x10000']).read_bytes(), 0x11000)
if not md5:
    raise SystemExit('Missing partition-table MD5.')
parts.sort(key=lambda p: p.offset)
for first, second in zip(parts, parts[1:]):
    if first.end > second.offset:
        raise SystemExit('Overlapping partitions.')
manifest = {'chip': 'esp32c3', 'flash_bytes': 8388608, 'partition_md5': md5, 'images': []}
previous_end = 0
for offset, filename in expected.items():
    data = (build / filename).read_bytes()
    start = int(offset, 0)
    if not data or start < previous_end or start + len(data) > 8388608:
        raise SystemExit('Image bounds invalid: ' + filename)
    if filename in ['muse-gadget.bin', 'bootloader/bootloader.bin']:
        if data[0] != 0xe9 or int.from_bytes(data[12:14], 'little') != 5:
            raise SystemExit('Not an ESP32-C3 image: ' + filename)
    if filename in ['ota_data_initial.bin', 'muse-gadget.bin']:
        part = next((p for p in parts if p.offset == start), None)
        if not part or len(data) > part.size:
            raise SystemExit('Image does not fit partition: ' + filename)
    previous_end = start + len(data)
    manifest['images'].append({'offset': offset, 'file': filename, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()})
print(json.dumps(manifest, indent=2))
