#!/usr/bin/env bash
set -euo pipefail
experiment="$(cd "$(dirname "$0")" && pwd)"
project="$(cd "$experiment/../.." && pwd)"
toolchain="${PASSPORT_MUSE_TOOLCHAIN_ROOT:-$project/.passport-toolchain/muse-6.0.1}"
export IDF_TOOLS_PATH="$toolchain/tools"
if [[ ! -f "$toolchain/esp-idf/export.sh" ]]; then
    echo 'Install ESP-IDF 6.0.1 in .passport-toolchain/muse-6.0.1/esp-idf first.' >&2
    exit 1
fi
source "$toolchain/esp-idf/export.sh" >/dev/null
export MUSE_TOKEN_FILE="${MUSE_TOKEN_FILE:-$experiment/.local/sdk-token.txt}"
cd "$experiment"
python build.py
python verify.py --require-proxy
