[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'DotnetEnvironment.ps1')
$dotnet = Get-UxrbDotnet
$root = Split-Path -Parent $PSScriptRoot

Push-Location $root
try {
    & $dotnet run --project 'apps\launcher\UniversalXRBridge.Launcher.csproj'
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}

