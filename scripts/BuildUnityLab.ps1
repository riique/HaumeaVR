[CmdletBinding()]
param(
    [ValidateSet('Mono', 'IL2CPP')]
    [string]$Backend = 'Mono',
    [string]$UnityEditorPath = $env:UNITY_EDITOR_PATH
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$requiredVersion = '6000.3.19f1'

if ([string]::IsNullOrWhiteSpace($UnityEditorPath)) {
    $UnityEditorPath = Join-Path ${env:ProgramFiles} "Unity\Hub\Editor\$requiredVersion\Editor\Unity.exe"
}
if (-not (Test-Path -LiteralPath $UnityEditorPath -PathType Leaf)) {
    throw "Unity Editor $requiredVersion was not found. Install that exact Unity Hub version or set UNITY_EDITOR_PATH. No global Unity setting was changed."
}

& (Join-Path $PSScriptRoot 'Build.ps1') -Preset windows-msvc-release

$nativePlugin = Join-Path $root 'build\windows-msvc-release\bin\uxrb_unity_bridge.dll'
if (-not (Test-Path -LiteralPath $nativePlugin -PathType Leaf)) {
    throw "The native Unity bridge was not produced at $nativePlugin."
}

$pluginFolder = Join-Path $root 'tests\unity-lab\Assets\Plugins\x86_64'
New-Item -ItemType Directory -Path $pluginFolder -Force | Out-Null
Copy-Item -LiteralPath $nativePlugin -Destination (Join-Path $pluginFolder 'uxrb_unity_bridge.dll') -Force

$projectPath = Join-Path $root 'tests\unity-lab'
$outputPath = Join-Path $root "artifacts\unity-lab\$Backend"
$logPath = Join-Path $root "artifacts\unity-lab\unity-$($Backend.ToLowerInvariant()).log"
New-Item -ItemType Directory -Path (Split-Path -Parent $logPath) -Force | Out-Null
$method = if ($Backend -eq 'Mono') {
    'Haumea.UniversalXRBridge.Lab.CompatibilityLabBuilder.BuildMonoPlayer'
} else {
    'Haumea.UniversalXRBridge.Lab.CompatibilityLabBuilder.BuildIl2CppPlayer'
}

& $UnityEditorPath `
    -batchmode `
    -nographics `
    -quit `
    -projectPath $projectPath `
    -executeMethod $method `
    -uxrb-output $outputPath `
    -logFile $logPath

if ($LASTEXITCODE -ne 0) {
    throw "Unity lab build failed with exit code $LASTEXITCODE. See $logPath."
}

Write-Host "Unity $Backend compatibility lab built at $outputPath"
