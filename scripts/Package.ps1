[CmdletBinding()]
param(
    [ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+$')]
    [string] $Version = '0.1.0',
    [ValidateSet('win-x64')]
    [string] $Runtime = 'win-x64'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$artifactsRoot = [System.IO.Path]::GetFullPath((Join-Path $root 'artifacts'))
$publishRoot = Join-Path $artifactsRoot "launcher\$Runtime"
$distributionRoot = Join-Path $artifactsRoot 'distribution'
$packageName = "HaumeaVR-$Version-$Runtime"
$zipPath = Join-Path $distributionRoot "$packageName.zip"
$hashPath = "$zipPath.sha256"
$stagingRoot = [System.IO.Path]::GetFullPath((Join-Path $artifactsRoot "staging-$([guid]::NewGuid().ToString('N'))"))
$stagingPackage = Join-Path $stagingRoot 'HaumeaVR'

$artifactsPrefix = $artifactsRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
if (-not $stagingRoot.StartsWith($artifactsPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "The staging directory escaped the repository artifacts folder: $stagingRoot"
}

& (Join-Path $PSScriptRoot 'Publish.ps1') -Runtime $Runtime
if ($LASTEXITCODE -ne 0) { throw "Publish failed with exit code $LASTEXITCODE." }

Add-Type -AssemblyName System.IO.Compression.FileSystem
New-Item -ItemType Directory -Path $distributionRoot -Force | Out-Null
New-Item -ItemType Directory -Path $stagingPackage -Force | Out-Null

try {
    Copy-Item -Path (Join-Path $publishRoot '*') -Destination $stagingPackage -Recurse -Force
    Copy-Item `
        -LiteralPath (Join-Path $root 'distribution\LEIA-ME-PTBR.txt') `
        -Destination (Join-Path $stagingPackage 'LEIA-ME-PTBR.txt') `
        -Force

    if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
    if (Test-Path -LiteralPath $hashPath) { Remove-Item -LiteralPath $hashPath -Force }
    [System.IO.Compression.ZipFile]::CreateFromDirectory(
        $stagingRoot,
        $zipPath,
        [System.IO.Compression.CompressionLevel]::Optimal,
        $false)

    $archive = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
    try {
        $requiredEntries = @(
            'HaumeaVR/HaumeaVR.Launcher.exe',
            'HaumeaVR/uxrb_core.exe',
            'HaumeaVR/openxr_loader.dll',
            'HaumeaVR/SHA256SUMS.txt',
            'HaumeaVR/LEIA-ME-PTBR.txt'
        )
        $entryNames = @($archive.Entries.FullName -replace '\\', '/')
        $missing = @($requiredEntries | Where-Object { $_ -notin $entryNames })
        if ($missing.Count -ne 0) {
            throw "Portable package is missing required entries: $($missing -join ', ')"
        }
    }
    finally {
        $archive.Dispose()
    }

    $zipHash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
    [System.IO.File]::WriteAllText(
        $hashPath,
        "$zipHash  $([System.IO.Path]::GetFileName($zipPath))`n",
        [System.Text.UTF8Encoding]::new($false))
    Write-Output "Packaged HaumeaVR to $zipPath"
    Write-Output "SHA-256: $zipHash"
}
finally {
    if (Test-Path -LiteralPath $stagingRoot) {
        $resolvedStaging = [System.IO.Path]::GetFullPath($stagingRoot)
        if (-not $resolvedStaging.StartsWith($artifactsPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to remove staging directory outside artifacts: $resolvedStaging"
        }
        Remove-Item -LiteralPath $resolvedStaging -Recurse -Force
    }
}
