#!/bin/sh

gcc -O0 -g3 -ggdb -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-format -std=c99 main.c \
    config.d/config.c mem.d/mem_utils.c sheets.d/utils.c -o whm
