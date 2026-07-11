# Milestone 0 validation

Milestone 0 is complete only after both automated and physical-headset checks pass.

The latest machine-specific evidence is recorded in [validation-2026-07-10.md](validation-2026-07-10.md).

## Automated checks

- Configure from a clean build directory.
- Compile with MSVC x64.
- Run the runtime-independent CTest suite.
- Run `uxrb_core.exe --self-test` without an OpenXR runtime.
- Run `uxrb_core.exe --diagnose` and verify its failure mode when no runtime is active.

## Headset checks

- Select a conformant OpenXR runtime in the vendor application.
- Connect and wake the headset.
- Run `uxrb_core.exe --diagnose` and record runtime, system, adapter, and recommended view sizes.
- Launch `uxrb_core.exe`.
- Confirm that both eyes show a stable rotating cube.
- Confirm that head rotation and translation change the expected view.
- Confirm that the desktop mirror updates without stalling headset presentation.
- Close the mirror and verify a clean session shutdown.

## Exit criteria

- No direct dependency on a headset vendor SDK.
- Correct D3D11 adapter selected from the runtime-provided LUID.
- Two projection views submitted at runtime cadence.
- No GPU-to-CPU frame copy.
- Runtime loss and missing-runtime conditions produce actionable logs instead of a crash.
