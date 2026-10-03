#!/usr/bin/env bash
# Local-only setup and build. This script does not flash a device.
set -euo pipefail
repo="$(cd "$(dirname "$0")/.." && pwd)"
tool_root="${PASSPORT_TOOLCHAIN_ROOT:-$(dirname "$repo")/.passport-toolchain}"
idf_root="$tool_root/esp-idf-v5.5.3"
expected_idf=2c211b236707889e8400c4dc5644dd5c4ee071e0
[[ "$(uname -s)" == Darwin ]] || { echo 'This helper is for macOS.' >&2; exit 1; }
command -v git >/dev/null
command -v cc >/dev/null
python3 -c 'import sys; assert sys.version_info >= (3,9), "Python 3.9+ required"'
mkdir -p "$tool_root"
if [[ ! -x "$tool_root/bootstrap/bin/python" ]]; then
    python3 -m venv "$tool_root/bootstrap"
fi
source "$tool_root/bootstrap/bin/activate"
python -m pip install --retries 0 --timeout 15 'cmake>=3.22,<4' 'ninja>=1.11,<2'
export IDF_TOOLS_PATH="$tool_root/espressif"
export IDF_GITHUB_ASSETS=dl.espressif.cn/github_assets
export IDF_COMPONENT_STORAGE_URL=https://components-file.espressif.cn
if [[ ! -d "$idf_root" ]]; then
    git clone --branch v5.5.3 --depth 1 --recursive --shallow-submodules \
        https://git.espressif.com.cn/espressif/esp-idf.git "$idf_root"
fi
[[ "$(git -C "$idf_root" rev-parse HEAD)" == "$expected_idf" ]] || {
    echo 'Existing ESP-IDF checkout is not the expected version; no files removed.' >&2; exit 1;
}
git -C "$idf_root" submodule update --init --recursive
deactivate
"$idf_root/install.sh" esp32c3
export PATH="$tool_root/bootstrap/bin:$PATH"
source "$idf_root/export.sh"
[[ "$(idf.py --version)" == 'ESP-IDF v5.5.3' ]] || { echo 'Unexpected IDF version' >&2; exit 1; }
cd "$repo"
python3 tools/restore-source-assets.py
./tools/validate.sh
echo "Build and validation complete: $repo/build/FoloToy-AI-Passport-full.bin"
