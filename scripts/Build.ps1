[CmdletBinding()]
param(
    [ValidateSet('windows-msvc-debug', 'windows-msvc-release')]
    [string] $Preset = 'windows-msvc-debug'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'DeveloperEnvironment.ps1')
$cmake = Enter-UxrbDeveloperEnvironment
$repositoryRoot = Split-Path -Parent $PSScriptRoot

Push-Location $repositoryRoot
try {
    & $cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed with exit code $LASTEXITCODE."
    }

    & $cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}
