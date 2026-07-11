[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Debug',
    [switch] $NoRestore
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'DotnetEnvironment.ps1')
$dotnet = Get-UxrbDotnet
$root = Split-Path -Parent $PSScriptRoot

Push-Location $root
try {
    $arguments = @('build', 'UniversalXRBridge.slnx', '--configuration', $Configuration)
    if ($NoRestore) { $arguments += '--no-restore' }
    & $dotnet @arguments
    if ($LASTEXITCODE -ne 0) { throw "Launcher build failed with exit code $LASTEXITCODE." }
}
finally {
    Pop-Location
}

