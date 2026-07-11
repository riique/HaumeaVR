[CmdletBinding()]
param(
    [ValidateSet('windows-msvc-debug', 'windows-msvc-release')]
    [string] $Preset = 'windows-msvc-debug',
    [switch] $Fresh
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'DeveloperEnvironment.ps1')
$cmake = Enter-UxrbDeveloperEnvironment
$repositoryRoot = Split-Path -Parent $PSScriptRoot

Push-Location $repositoryRoot
try {
    $arguments = @('--preset', $Preset)
    if ($Fresh) {
        $arguments += '--fresh'
    }

    & $cmake @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}

