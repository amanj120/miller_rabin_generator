#! /usr/bin/bash
yes | rm run
clang-format -i --style="{IndentWidth: 4, TabWidth: 4, UseTab: "Always"}" *.c
gcc -o run *.c

