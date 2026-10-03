#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p build
for name in test_tetris_model test_stress; do
    cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Imain \
        "tests/$name.c" main/tetris_model.c -o "build/$name"
    "./build/$name"
done
cc -std=c11 -Wall -Wextra -Werror -Imain host_demo.c main/tetris_model.c -o build/tetris-host
python3 tests/test_host_demo.py
