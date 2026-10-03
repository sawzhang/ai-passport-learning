#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
exec "$root/firmware/tools/flash-games.sh" "${1:?Usage: flash.sh /dev/cu.usbmodemXXXX}"
