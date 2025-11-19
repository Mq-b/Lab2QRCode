#!/usr/bin/env pwsh

# 获取当前仓库的 Git 哈希值
$git_hash = git rev-parse --short HEAD 2>$null

# 获取当前仓库的 Git 版本标签
$git_tag = git describe --tags --abbrev=0 2>$null

# 获取当前的 Git 分支名
$git_branch = git branch --show-current

# 获取当前仓库最新提交的时间
$git_commit_time = git log -1 --format=%cd --date=format:'%Y-%m-%d %H:%M:%S' 2>$null

# 获取构建的时间
$build_time = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'


# 生成 version.cpp 文件
@"
#include "version.h"

namespace version{
    constexpr std::string_view git_hash = "$git_hash";
    constexpr std::string_view git_tag = "$git_tag";
    constexpr std::string_view git_branch = "$git_branch";
    constexpr std::string_view git_commit_time = "$git_commit_time";
    constexpr std::string_view build_time = "$build_time";
};
"@ | Out-File -Encoding utf8 version.cpp

# 提示生成成功
Write-Output "version.cpp 文件已生成:"
Get-Content version.cpp

# Linux 中：pwsh version.ps1 需要第一行。