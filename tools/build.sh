#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
unset PASSPORT_DEVICE_SELF_TEST
exec "$root/firmware/tools/prepare-and-build-macos.sh"
