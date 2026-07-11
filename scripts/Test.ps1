[CmdletBinding()]
param(
    [string] $Preset = 'windows-msvc-debug'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'DeveloperEnvironment.ps1')
$cmake = Enter-UxrbDeveloperEnvironment
$ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
$repositoryRoot = Split-Path -Parent $PSScriptRoot

if (-not (Test-Path -LiteralPath $ctest)) {
    throw "CTest was not found next to CMake: $ctest"
}

Push-Location $repositoryRoot
try {
    & $ctest --preset $Preset
    if ($LASTEXITCODE -ne 0) {
        throw "CTest failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}

