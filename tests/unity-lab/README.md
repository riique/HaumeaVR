# Unity Compatibility Lab

Pinned Editor: **Unity 6000.3.19f1**. The initial matrix targets Windows x64,
D3D11 and the Built-in Render Pipeline with both Mono and IL2CPP builds.

The scene is generated from source so large binary Unity assets do not need to
be committed. It covers multiple cameras, camera switching/cutscenes,
transparent objects, particles, water-like transparency, a reflective material,
portal geometry, Screen Space Overlay UI and World Space UI.

Build from the repository root:

```powershell
.\scripts\BuildUnityLab.ps1 -Backend Mono
.\scripts\BuildUnityLab.ps1 -Backend IL2CPP
```

Run the generated player with the bridge's lab-only pose source:

```powershell
UniversalXRCompatibilityLab.exe -force-d3d11 -uxrb-enable -uxrb-simulated-pose
```

For the live OpenXR path, add the generated Mono executable to the HaumeaVR launcher,
analyze it and press **Unity Stereo**. The launcher supplies the transport name,
expected process ID and D3D11/activation switches. The plug-in retries its
connection while the core creates the mapping, so launch order is deterministic.

Without `-uxrb-enable`, the managed bridge does not create an object, register a
texture or change a camera.
