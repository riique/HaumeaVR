# HaumeaVR

HaumeaVR is a Windows-first OpenXR platform for bringing selected
flat-screen games into VR through a staged, safety-conscious compatibility
ladder. The original product plan lives in [Plano.md](Plano.md). The
self-contained implementation roadmap for future work is indexed in
[docs/planos-implementacao/README.md](docs/planos-implementacao/README.md).

## Implemented today

- OpenXR core using the official Khronos loader, D3D11 adapter LUID matching,
  stereo swapchains, predicted eye poses, mirror window and diagnostics;
- self-contained Avalonia launcher with Steam/Epic/manual discovery, engine,
  scripting backend, graphics API and anti-cheat detection, SQLite profiles and
  authenticated local named-pipe IPC;
- universal Virtual Screen mode with DXGI Desktop Duplication, window crop,
  flat/curved GPU mesh, recentering and OpenXR controller actions;
- opt-in Unity 6000.3 Mono bridge with asymmetric per-eye cameras, a native
  render-thread plug-in and live cross-process OpenXR pose/texture transport;
- reproducible Unity Compatibility Lab source for Built-in/Mono and IL2CPP;
- opt-in, current-process-only D3D11 `Present`/depth observer with a hard
  anti-cheat veto and no remote-process injector;
- Depth 3D visualization and stereo compute reconstruction foundation.

Virtual Screen is the universal end-user path. Unity Stereo is exposed only for
controlled Unity Mono players that already contain both HaumeaVR bridge modules; the
launcher verifies that deployment, binds the random transport name to the game
PID and blocks the path when anti-cheat is detected. Generic Depth 3D and
Geometry Stereo remain foundations rather than advertised game capabilities.

## Build, test and publish

```powershell
.\scripts\Configure.ps1 -Fresh
.\scripts\Build.ps1 -Preset windows-msvc-debug
.\scripts\Test.ps1 -Preset windows-msvc-debug
.\scripts\TestLauncher.ps1
.\scripts\Publish.ps1
.\scripts\Package.ps1
```

The self-contained launcher is published to
`artifacts\launcher\win-x64\HaumeaVR.Launcher.exe` and does not require
a machine-wide .NET or Visual C++ runtime installation. The official OpenXR
loader DLL is shipped beside the core, and publication runs the packaged native
self-test before succeeding. `SHA256SUMS.txt` records every published file hash.
See [the validation report](docs/validation-2026-07-10.md)
for the exact test matrix and remaining physical-device check.

`Package.ps1` creates a versioned portable ZIP under `artifacts\distribution`.
It contains no source code and needs no compiler on the destination computer;
the recipient extracts the whole folder and runs the launcher executable. See
[the distribution and MiSide status](docs/distribution-and-miside-2026-07-11.md)
for the clean-extraction test and the exact current compatibility limit.

Useful native checks:

```powershell
.\build\windows-msvc-debug\bin\uxrb_core.exe --self-test
.\build\windows-msvc-debug\bin\uxrb_core.exe --capture-self-test
.\build\windows-msvc-debug\bin\uxrb_core.exe --diagnose
```

The Unity lab is pinned to Unity `6000.3.19f1`:

```powershell
.\scripts\BuildUnityLab.ps1 -Backend Mono
.\scripts\BuildUnityLab.ps1 -Backend IL2CPP
```

After building the Mono lab, add its executable to the launcher, analyze it and
use **Unity Stereo**. The launcher starts it with `-force-d3d11 -uxrb-enable`,
passes a random versioned shared-memory name through the child environment and
asks the core to accept textures only from that exact process ID. The Unity
package is a controlled integration for games/projects where the bridge is
deployed during the build; it is not a general commercial-game injector.

## Runtime selection and safety

HaumeaVR never writes the global OpenXR runtime registration. A runtime selected in
the launcher is applied to the child core process only through `XR_RUNTIME_JSON`.

The IPC server rejects remote clients and authenticates every session with a
random 256-bit token. No kernel driver, service, unsigned community code or
anti-cheat bypass is present. The D3D11 observer requires
`UXRB_D3D11_HOOK_ENABLE=1`, only attaches inside its current process, and refuses
to run when a known anti-cheat module is loaded. Microsoft Detours' process
creation/injection source is intentionally excluded from the build.

The Unity transport is process-local, randomly named and versioned. Its core
owner refuses a pre-existing mapping, the texture consumer validates producer
PID/name/format/dimensions, and named D3D11 resources remain guarded by keyed
mutexes for their entire published lifetime.

Logs are stored under `%LOCALAPPDATA%\HaumeaVR\logs`. Existing profile data from
the legacy `UniversalXRBridge` folder is copied on first launch.
