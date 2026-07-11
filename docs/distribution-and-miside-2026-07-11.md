# Distribution and MiSide status — 2026-07-11

## Portable distribution

`scripts/Package.ps1` generated:

```text
artifacts/distribution/UniversalXRBridge-0.1.0-win-x64.zip
artifacts/distribution/UniversalXRBridge-0.1.0-win-x64.zip.sha256
```

Validation performed from a clean extraction directory:

- ZIP size: 81,315,632 bytes;
- ZIP SHA-256:
  `b46e47b25517a592a10478414ec022970899ca177800b7c6b505db980650f46c`;
- 236 payload files;
- zero C/C++/C# project or source files;
- launcher, core, OpenXR loader, per-file checksums and Portuguese instructions
  present;
- extracted `uxrb_core.exe --self-test`: passed on AMD Radeon RX 6600.

The recipient does not need Visual Studio, a compiler, a machine-wide .NET
runtime or a Visual C++ Redistributable. They must extract the complete folder
before launching `UniversalXRBridge.Launcher.exe`.

This is a portable release, not a signed installer. A future one-click MSI/EXE
should be code-signed before general distribution to avoid avoidable Windows
SmartScreen friction.

## MiSide inspection

Installed target inspected:

```text
C:\Program Files (x86)\Steam\steamapps\common\MiSide\MiSideFull.exe
```

Observed files:

- `UnityPlayer.dll`: present;
- `MiSideFull_Data`: present;
- `GameAssembly.dll`: present;
- `MonoBleedingEdge`: absent;
- `MiSideFull_Data\Managed\Assembly-CSharp.dll`: absent;
- `uxrb_unity_bridge.dll`: absent;
- `Haumea.UniversalXRBridge.Runtime.dll`: absent.

This identifies the installed build as a Unity IL2CPP player, not the controlled
Unity Mono target currently supported by Unity Stereo. Consequently:

- **available now:** external Virtual Screen, flat/curved presentation,
  headset-driven view of the panel and recentering;
- **not available now:** direct stereo cameras inside MiSide, six-degree-of-
  freedom movement of the game camera, tracked hands, controller-to-game
  interaction, object grabbing or room-scale collision.

Reaching native-feeling MiSide VR would require an authorized/safe IL2CPP
adapter or game-specific mod, validated camera and HUD integration, OpenXR
controller/hand input mapping and game-specific interaction bindings. The
current project deliberately does not inject an unfinished bridge into the
commercial game or bypass integrity/anti-cheat systems.
