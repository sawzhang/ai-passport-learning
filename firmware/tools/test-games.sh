#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/host-tests
flags=(-std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Imain)
"${CC:-cc}" "${flags[@]}" tests/test_tetris_model.c main/tetris_model.c -o build/host-tests/tetris
"${CC:-cc}" "${flags[@]}" tests/test_tetris_stress.c main/tetris_model.c -o build/host-tests/tetris-stress
"${CC:-cc}" "${flags[@]}" tests/test_word_game.c main/word_game.c main/word_bank.c -o build/host-tests/words
build/host-tests/tetris
build/host-tests/tetris-stress
build/host-tests/words
