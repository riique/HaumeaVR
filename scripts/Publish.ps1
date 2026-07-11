[CmdletBinding()]
param(
    [ValidateSet('win-x64')]
    [string] $Runtime = 'win-x64'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'DotnetEnvironment.ps1')
$dotnet = Get-UxrbDotnet
$root = Split-Path -Parent $PSScriptRoot
$output = Join-Path $root "artifacts\launcher\$Runtime"

& (Join-Path $PSScriptRoot 'Build.ps1') -Preset windows-msvc-release
if ($LASTEXITCODE -ne 0) { throw "Native Release build failed with exit code $LASTEXITCODE." }

Push-Location $root
try {
    & $dotnet publish .\apps\launcher\UniversalXRBridge.Launcher.csproj `
        --configuration Release `
        --runtime $Runtime `
        --self-contained true `
        --output $output `
        -p:DebugType=None `
        -p:DebugSymbols=false
    if ($LASTEXITCODE -ne 0) { throw "Launcher publish failed with exit code $LASTEXITCODE." }

    Copy-Item `
        -LiteralPath (Join-Path $root 'build\windows-msvc-release\bin\uxrb_core.exe') `
        -Destination (Join-Path $output 'uxrb_core.exe') `
        -Force
    Copy-Item `
        -LiteralPath (Join-Path $root 'build\windows-msvc-release\bin\openxr_loader.dll') `
        -Destination (Join-Path $output 'openxr_loader.dll') `
        -Force
    Copy-Item `
        -LiteralPath (Join-Path $root 'build\windows-msvc-release\bin\uxrb_unity_bridge.dll') `
        -Destination (Join-Path $output 'uxrb_unity_bridge.dll') `
        -Force
    Copy-Item `
        -LiteralPath (Join-Path $root 'build\windows-msvc-release\bin\uxrb_d3d11_intercept.dll') `
        -Destination (Join-Path $output 'uxrb_d3d11_intercept.dll') `
        -Force
    Copy-Item `
        -LiteralPath (Join-Path $root 'THIRD_PARTY_NOTICES.md') `
        -Destination (Join-Path $output 'THIRD_PARTY_NOTICES.md') `
        -Force

    & (Join-Path $output 'uxrb_core.exe') --self-test
    if ($LASTEXITCODE -ne 0) {
        throw "Published native core self-test failed with exit code $LASTEXITCODE."
    }

    $checksumFiles = Get-ChildItem -LiteralPath $output -File |
        Where-Object Name -ne 'SHA256SUMS.txt' |
        Sort-Object Name
    $checksumLines = foreach ($file in $checksumFiles) {
        $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        "$hash  $($file.Name)"
    }
    [System.IO.File]::WriteAllLines(
        (Join-Path $output 'SHA256SUMS.txt'),
        $checksumLines,
        [System.Text.UTF8Encoding]::new($false))
    Write-Output "Published HaumeaVR to $output"
}
finally {
    Pop-Location
}
