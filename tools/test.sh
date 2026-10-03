#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/demo-tetris/test.sh"
cd "$root/firmware"
./tools/validate.sh --static
