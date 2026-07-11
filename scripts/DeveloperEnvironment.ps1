Set-StrictMode -Version Latest

function Enter-UxrbDeveloperEnvironment {
    [CmdletBinding()]
    param()

    $vsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vsWhere)) {
        throw 'Visual Studio Build Tools were not found (vswhere.exe is missing).'
    }

    $installations = @(& $vsWhere -all -products '*' -property installationPath)
    $installation = $installations |
        Where-Object { Test-Path -LiteralPath (Join-Path $_ 'VC\Auxiliary\Build\vcvars64.bat') } |
        Select-Object -First 1

    if (-not $installation) {
        throw 'No Visual Studio installation with the x64 MSVC toolchain was found.'
    }

    $devCommand = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
    $environmentDump = & $env:ComSpec /d /s /c "`"$devCommand`" -no_logo -arch=x64 -host_arch=x64 >nul && set"
    if ($LASTEXITCODE -ne 0) {
        throw "VsDevCmd failed with exit code $LASTEXITCODE."
    }

    foreach ($line in $environmentDump) {
        if ($line -match '^([^=]+)=(.*)$') {
            [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
        }
    }

    # Keep MSVC's /showIncludes prefix ASCII and deterministic so Ninja can
    # parse dependencies even when Windows uses a non-English display language.
    $env:VSLANG = '1033'

    $bundledCMake = Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if (Test-Path -LiteralPath $bundledCMake) {
        return $bundledCMake
    }

    $cmake = Get-Command cmake.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $cmake) {
        throw 'CMake was not found in Visual Studio or PATH.'
    }

    return $cmake.Source
}
