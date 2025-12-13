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
  exit 2
else
  echo "使用 $CLANG_FORMAT 进行格式检查。"
fi

ver_num=$(echo "$CLANG_FORMAT" | grep -o '[0-9]\+' | head -1)

if [ "$ver_num" -ne 22 ]; then
  echo "注意当前使用的 clang-format 与 ci 使用的版本不一致，可能导致格式检查结果不同。"
fi

if [ "$ver_num" -le 19 ]; then
  echo "警告：检测到的 clang-format 版本为 $ver_num，已知存在一些问题，请考虑升级。"
  echo "已知的问题如下："
  echo "========================= 高版本 ========================="
  echo "MyClass obj = {"
  echo "    .member1 = value1,"
  echo "    .member2 = value2,"
  echo "};"
  echo "========================= 低版本 ========================="
  echo "MyClass obj = {"
  echo "       .member1 = value1,"
  echo "       .member2 = value2,"
  echo "};"
  echo "=========================        ========================="
fi

echo "正在格式化所有文件..."

find "$dir" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec $CLANG_FORMAT -i {} \;

echo "所有文件已格式化完成。"
