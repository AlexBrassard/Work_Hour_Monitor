#!/bin/bash 

gcc -g3 -ggdb -O0 -std=c99 -D_POSIX_C_SOURCE=200809L \
    -Wpedantic -Wextra -Wall -Wno-unused-parameter -Wno-unused-variable \
    whm_mem_utils.c whm_gen_utils.c whm_config.c whm_sheet.c\
    function_tester.c -o function_tester
