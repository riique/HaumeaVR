Set-StrictMode -Version Latest

function Get-UxrbDotnet {
    [CmdletBinding()]
    param()

    $managedInstall = Join-Path $env:LOCALAPPDATA 'UniversalXRBridge\dotnet\dotnet.exe'
    if (Test-Path -LiteralPath $managedInstall) {
        return $managedInstall
    }

    $dotnet = Get-Command dotnet.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $dotnet) {
        throw 'The .NET SDK was not found. Install the version pinned by global.json.'
    }
    return $dotnet.Source
}

