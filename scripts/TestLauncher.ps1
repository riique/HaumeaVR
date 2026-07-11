[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Debug'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'DotnetEnvironment.ps1')
$dotnet = Get-UxrbDotnet
$root = Split-Path -Parent $PSScriptRoot

Push-Location $root
try {
    & $dotnet test 'apps\launcher.tests\UniversalXRBridge.Launcher.Tests.csproj' --configuration $Configuration
    if ($LASTEXITCODE -ne 0) { throw "Launcher tests failed with exit code $LASTEXITCODE." }
}
finally {
    Pop-Location
}

