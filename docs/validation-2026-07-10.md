# Validation report — 2026-07-10

## Build and automated checks

- MSVC x64 Debug configure/build: passed.
- MSVC x64 Release configure/build: passed.
- CTest Debug: 7/7 passed.
- CTest Release: 7/7 passed.
- Launcher/.NET tests: 11/11 passed, including a real authenticated IPC
  process integration test.
- NuGet vulnerable-package scan, including transitives: no known vulnerable
  packages reported by the configured source.
- Khronos OpenXR SDK/loader: pinned to `release-1.1.58`.
- Unity native interface input: pinned to NativeRenderingPlugin commit
  `f703c47a140d343c2c863a36d1aa5832586f3aaa`.
- Microsoft Detours: pinned to `v4.0.1`; process-creation/injection helpers are
  not compiled.

The seven native tests cover projection math, both D3D11 render pipelines,
framed/authenticated IPC, versioned Unity shared memory and mapping-collision
refusal, cross-device named D3D11 texture transfer, Depth 3D compute
reconstruction/visualization, Unity native ABI lifecycle and current-process
D3D11 observation with anti-cheat refusal.

## Published bundle

`scripts/Publish.ps1` produced the self-contained Windows x64 launcher and now
runs the **published** `uxrb_core.exe --self-test` before reporting success. The
bundle includes `openxr_loader.dll`; UXRB native binaries link the MSVC runtime
statically and no longer depend on a machine-wide Visual C++ Redistributable.
Publication also writes `SHA256SUMS.txt` for every file in the self-contained
bundle.

Checks executed directly against `artifacts/launcher/win-x64`:

- core help/argument parser: passed;
- runtime-independent D3D11 self-test: passed on AMD Radeon RX 6600;
- Desktop Duplication self-test: passed on `\\.\DISPLAY2`, 1920×1080, with a
  GPU-resident texture;
- invalid/non-local Unity transport name: rejected before OpenXR startup;
- launcher startup without a global .NET 10 runtime: passed because the bundle
  contains .NET Runtime 10.0.9;
- visual QA at 1440×900: passed; game list, runtime selector, diagnostics,
  compatibility column and all four session controls render without clipping.

## Unity Stereo transport

The controlled Unity path now has a live bidirectional transport:

1. launcher generates a random per-session `Local\\UXRB-Unity-<id>` name;
2. core owns a versioned mapping and refuses a name that already exists;
3. core publishes predicted per-eye pose/FOV and recommended dimensions through
   a seqlock;
4. Unity renders script-free eye cameras and copies them on the render thread;
5. plug-in publishes named keyed-mutex D3D11 textures while retaining each NT
   handle for the advertised lifetime;
6. core verifies expected producer PID, resource name, dimensions, format and
   sharing flags before copying into OpenXR swapchains.

The launcher only enables this route for an analyzed Unity Mono player that
already contains both controlled bridge modules. A detected anti-cheat is a hard
block; Virtual Screen remains available externally.

## OpenXR runtime checks

The machine has no global 64-bit or 32-bit `ActiveRuntime` registry value. UXRB
did not change either value.

Registered runtime observations:

- Meta Horizon is registered, but its recorded manifest path is missing;
- Virtual Desktop's manifest exists and loads through a process-local
  `XR_RUNTIME_JSON` override.

The published core loaded VirtualDesktopXR `1.0.10`. `xrGetSystem` then returned
`XR_ERROR_FORM_FACTOR_UNAVAILABLE`: the runtime was active for the process but no
HMD was exposed at validation time.

## Remaining physical/toolchain checks

Unity Editor `6000.3.19f1` and an active HMD were not installed/available on this
machine. Consequently, the source-pinned Unity Compatibility Lab could not be
built and the final photon-to-photon Unity/headset path could not be observed.
No multi-gigabyte Editor or vendor runtime was installed silently, and no global
OpenXR setting was changed.

When those prerequisites are present, the remaining acceptance run is:

```powershell
.\scripts\BuildUnityLab.ps1 -Backend Mono
.\artifacts\launcher\win-x64\UniversalXRBridge.Launcher.exe
```

Add the generated Mono lab executable, analyze it, choose the intended runtime
and start **Unity Stereo**. Confirm stereo geometry, six-degree-of-freedom head
response, HUD readability, mirror output, frame stability and clean shutdown in
the headset.
