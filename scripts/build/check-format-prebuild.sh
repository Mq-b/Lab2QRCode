#!/usr/bin/env bash

dir="${1:-./src/}"

CLANG_FORMAT=""
for ver in 22 21 20 19 18 17 16 15; do
  if command -v clang-format-$ver >/dev/null 2>&1; then
    CLANG_FORMAT="clang-format-$ver"
    break
  fi
done

if [ -z "$CLANG_FORMAT" ]; then
  echo "未找到可用的 clang-format 版本（22-15），请安装后重试。"
  exit 0
fi

ver_num=$(echo "$CLANG_FORMAT" | grep -o '[0-9]\+' | head -1)

if [ "$ver_num" -ne 22 ]; then
  echo "注意：当前使用的 clang-format 与 ci 使用的版本不一致，可能导致格式检查结果不同。"
fi

if [ "$ver_num" -le 19 ]; then
  echo "警告：检测到的 clang-format 版本为 $ver_num，已知存在一些问题，请考虑升级。"
fi

for file in $(find "$dir" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \)); do
  diff=$($CLANG_FORMAT "$file" | diff -u "$file" -)
  if [ $? -ne 0 ]; then
    echo "文件: $file"
    echo "========================= 发现不规范的格式 ========================="
    echo "$diff"
    echo "=========================       结束       ========================="
  fi
done
