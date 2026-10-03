#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p build
cc -std=c11 -Wall -Wextra -Werror -Imain host_demo.c main/tetris_model.c -o build/tetris-host
exec ./build/tetris-host "$@"
