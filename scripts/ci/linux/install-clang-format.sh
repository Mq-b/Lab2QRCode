#!/usr/bin/env bash

sudo apt update
sudo apt install -y curl gnupg
curl https://apt.llvm.org/llvm.sh | grep -Ev '^\s*apt(-get)?\s+install' | sudo bash -s 22
sudo apt install -y clang-format-22
