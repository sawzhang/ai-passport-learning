#!/usr/bin/env bash
# Use only for the connected AI Passport and the verified local build.
set -euo pipefail
repo="$(cd "$(dirname "$0")/.." && pwd)"
port="${1:?Usage: flash-games.sh /dev/cu.usbmodemXXXX}"
tool_root="${PASSPORT_TOOLCHAIN_ROOT:-$(dirname "$repo")/.passport-toolchain}"
export IDF_TOOLS_PATH="$tool_root/espressif"
source "$tool_root/bootstrap/bin/activate"
source "$tool_root/esp-idf-v5.5.3/export.sh"
cd "$repo"
python - "$port" <<'PY'
import sys
from serial.tools.list_ports import comports
matches=[p for p in comports() if p.device==sys.argv[1] and p.vid==0x303a and p.pid==0x1001]
if len(matches)!=1: raise SystemExit('Expected Espressif USB Serial/JTAG device not found at the selected port')
PY
bundle="$(python - <<'PY'
from pathlib import Path
import hashlib
p=Path('build/FoloToy-AI-Passport-full.bin')
if not p.is_file(): raise SystemExit('No merged firmware; build first')
digest=hashlib.sha256(p.read_bytes()).hexdigest()
bundle=Path('build/firmware')/digest
if not (bundle/'manifest.json').is_file(): raise SystemExit('Matching verified bundle is missing')
print(bundle)
PY
)"
python tools/archive_firmware.py verify "$bundle"
help_text="$(python -m esptool --help)"
if [[ "$help_text" == *write-flash* ]]; then
    flash_id=flash-id; write_flash=write-flash
else
    flash_id=flash_id; write_flash=write_flash
fi
flash_info="$(python -m esptool --chip esp32c3 --port "$port" "$flash_id")"
printf '%s\n' "$flash_info"
[[ "$flash_info" == *'Detected flash size: 8MB'* ]] || {
    echo 'Expected 8 MB Flash was not confirmed; not writing.' >&2; exit 1;
}
echo "Writing verified merged firmware to $port at 0x0; existing app/data may be replaced."
python -m esptool --chip esp32c3 --port "$port" --baud 460800 \
    "$write_flash" 0x0 "$bundle/FoloToy-AI-Passport-full.bin"
echo 'Write complete. Physical screen/button/gameplay checks are still required.'
