#!/usr/bin/env bash

dir="${1:-./src/}"

find "$dir" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format-22 -i {} \;
