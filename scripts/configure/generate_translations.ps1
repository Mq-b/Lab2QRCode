#!/usr/bin/env pwsh
param(
    [Parameter(Mandatory=$true)]
    [string]$LupdatePath,

    [Parameter(Mandatory=$true)]
    [string]$LreleasePath,

    [Parameter(Mandatory=$true)]
    [string]$SourceDir
)

# 规范化路径（去除可能的引号）
$LupdatePath = $LupdatePath.Trim('"')
$LreleasePath = $LreleasePath.Trim('"')
$SourceDir = $SourceDir.Trim('"')

# 定义需要的目录和文件
$TranslationsDir = Join-Path $SourceDir "translations"
$I18nDir        = Join-Path $SourceDir "resources\i18n"
$QrcFile        = Join-Path $SourceDir "resources\i18n.qrc"

# 确保目录存在
if (-not (Test-Path $TranslationsDir)) {
    New-Item -ItemType Directory -Path $TranslationsDir -Force | Out-Null
}
if (-not (Test-Path $I18nDir)) {
    New-Item -ItemType Directory -Path $I18nDir -Force | Out-Null
}

# 要生成的 .ts 文件
$TsFiles = @(
    Join-Path $TranslationsDir "app_en_US.ts"
    Join-Path $TranslationsDir "app_zh_CN.ts"
)

# ===============================================================================
# 自动生成 i18n.qrc（如果不存在）
$qrcContent = @'
<RCC>
    <qresource prefix="/">
        <file>i18n/app_zh_CN.qm</file>
        <file>i18n/app_en_US.qm</file>
    </qresource>
</RCC>
'@

if (-not (Test-Path $QrcFile)) {
    Write-Host "Creating missing resource file: $QrcFile"
    Set-Content -Path $QrcFile -Value $qrcContent.Trim() -Encoding UTF8
    Write-Host "Generated: $QrcFile"
} else {
    Write-Host "Resource file already exists: $QrcFile (skipped creation)"
}
# ===============================================================================

# 调用lupdate生成/更新.ts文件
Write-Host "Running lupdate to generate/update .ts files..."
& $LupdatePath (Join-Path $SourceDir "src") -ts $TsFiles[0] $TsFiles[1]

if ($LASTEXITCODE -ne 0) {
    Write-Error "lupdate failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

# 调用lrelease生成qm翻译文件写入到resources/i18n目录下配合i18n.qrc使用
Write-Host "Running lrelease to generate .qm files..."
foreach ($TsFile in $TsFiles) {
    if (Test-Path $TsFile) {
        $QmFile = Join-Path $I18nDir ([System.IO.Path]::GetFileNameWithoutExtension($TsFile) + ".qm")
        & $LreleasePath $TsFile -qm $QmFile
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Generated: $QmFile"
        } else {
            Write-Warning "lrelease failed for $TsFile (exit code $LASTEXITCODE)"
        }
    } else {
        Write-Warning ".ts file not found: $TsFile (skipped)"
    }
}

Write-Host "Translation files generation completed."