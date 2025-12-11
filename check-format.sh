#!/usr/bin/env bash

dir="${1:-./src/}"

fail=0
for file in $(find "$dir" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \)); do
  echo "正在检查: $file"
  diff=$(clang-format-22 "$file" | diff -u "$file" -)
  if [ $? -ne 0 ]; then
    echo "========================= 发现不规范的格式 ========================="
    echo "$diff"
    echo "=========================       结束       ========================="
    fail=1
  fi
done

if [ $fail -ne 0 ]; then
  echo "有文件格式不规范！"
  exit 1
else
  echo "所有文件格式规范。"
  exit 0
fi
