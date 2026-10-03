#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
tool_root="${PASSPORT_TOOLCHAIN_ROOT:-$root/.passport-toolchain}"
python_path=""
if [[ -n "${IDF_PYTHON_ENV_PATH:-}" ]]; then
    python_path="$IDF_PYTHON_ENV_PATH/bin/python"
fi
if [[ ! -x "$python_path" ]]; then
    shopt -s nullglob
    candidates=("$tool_root"/espressif/python_env/idf5.5_py*_env/bin/python)
    if [[ ${#candidates[@]} -eq 1 ]]; then python_path="${candidates[0]}"; fi
fi
if [[ ! -x "$python_path" ]]; then
    echo 'Activate ESP-IDF 5.5.3 and run firmware/tools/monitor-games.py with its Python, or run tools/build.sh first.' >&2
    exit 1
fi
cd "$root/firmware"
exec "$python_path" tools/monitor-games.py "$@"
