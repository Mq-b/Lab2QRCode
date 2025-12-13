#!/usr/bin/env pwsh

param(
    [string]$dir = "./src/"
)

$CLANG_FORMAT = ""
foreach ($ver in 22, 21, 20, 19, 18, 17, 16, 15) {
    if (Get-Command "clang-format-$ver" -ErrorAction SilentlyContinue) {
        $CLANG_FORMAT = "clang-format-$ver"
        break
    }
}

if (-not $CLANG_FORMAT) {
    Write-Host "未找到可用的 clang-format 版本（22-15），请安装后重试。"
    exit 2
} else {
    Write-Host "使用 $CLANG_FORMAT 进行格式检查。"
}

$ver_num = ($CLANG_FORMAT -split "-")[-1] -as [int]

if ($ver_num -ne 22) {
    Write-Host "注意当前使用的 clang-format 与 ci 使用的版本不一致，可能导致格式检查结果不同。"
}

if ($ver_num -le 19) {
    Write-Host "警告：检测到的 clang-format 版本为 $ver_num，已知存在一些问题，请考虑升级。"
    Write-Host "已知的问题如下："
    Write-Host "========================= 高版本 ========================="
    Write-Host "MyClass obj = {"
    Write-Host "    .member1 = value1,"
    Write-Host "    .member2 = value2,"
    Write-Host "};"
    Write-Host "========================= 低版本 ========================="
    Write-Host "MyClass obj = {"
    Write-Host "       .member1 = value1,"
    Write-Host "       .member2 = value2,"
    Write-Host "};"
    Write-Host "=========================        ========================="
}

$fail = 0
$files = Get-ChildItem -Path $dir -Recurse -Include *.c,*.cpp,*.h,*.hpp -File
foreach ($file in $files) {
    Write-Host "正在检查: $($file.FullName)"
    $formatted = & $CLANG_FORMAT $file.FullName
    $original = Get-Content $file.FullName -Raw
    $diff = Compare-Object ($original -split "`n") ($formatted -split "`n") -PassThru | Out-String
    if ($diff.Trim() -ne "") {
        Write-Host "========================= 发现不规范的格式 ========================="
        Write-Host $diff
        Write-Host "=========================       结束       ========================="
        $fail = 1
    }
}

if ($fail -ne 0) {
    Write-Host "有文件格式不规范！"
    exit 1
} else {
    Write-Host "所有文件格式规范。"
    exit 0
}
