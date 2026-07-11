# Architecture

```text
Avalonia launcher
  | authenticated local IPC (random per-session token)
  v
uxrb_core
  | OpenXR timing, poses and swapchains
  | D3D11 renderer + mirror
  +-- Virtual Screen: DXGI Desktop Duplication -> curved/flat mesh
  +-- Unity Stereo: predicted views <-> shared memory <-> named eye textures
  +-- Depth foundation: normalized depth -> compute stereo pair

Controlled engine/graphics modules
  +-- Unity managed bridge -> native render event -> named shared D3D11 textures
  +-- D3D11 observer -> Present/back-buffer/depth metadata (explicit opt-in)
```

The OpenXR core is the sole owner of runtime selection, frame timing and headset
poses. Engine adapters never create a competing OpenXR session. The D3D11 device
uses the adapter LUID returned by `xrGetD3D11GraphicsRequirementsKHR`.

## Process boundaries

The launcher starts `uxrb_core` with a process-local runtime manifest and a
single-use IPC token. The token is not persisted in SQLite or profiles. The
native server accepts one local client and caps framed JSON payloads at 1 MiB.

The Unity plug-in executes texture registration on the host side and GPU copies
on Unity's render thread. Per-eye color textures are copied into named D3D11
shared resources guarded by a keyed mutex. The managed cameras are bare Camera
components; gameplay scripts, audio listeners and particle updates are not
cloned.

For each Unity session, the launcher creates a random `Local\\UXRB-Unity-<id>`
name and launches the controlled player with that name in its environment. The
core creates and owns a versioned mapping, publishes predicted OpenXR poses/FOV
and recommended dimensions through a seqlock, and accepts plug-in state only
from the expected process. The plug-in publishes per-eye resource names and
generations; the core validates PID, naming convention, dimensions, format and
sharing flags before performing a GPU-to-GPU copy into OpenXR swapchains.

## D3D11 observation boundary

The generic observer is a DLL API, not an injector. A controlled host must load
it and call `UxrbD3D11Hook_InstallForCurrentProcess` after setting the exact
opt-in environment value. Installation enumerates loaded modules and hard-fails
on recognized anti-cheat components. It observes `IDXGISwapChain::Present`, the
back buffer and the depth-stencil view bound at presentation. The next transport
phase will move compatible color/depth resources to the OpenXR core; until then,
Depth 3D is not advertised as an end-user capability.

## Fallback principle

Failures in Unity, Depth 3D or future geometry adapters must not prevent normal
game launch. Stopping the core makes the controlled Unity producer idle, while
the game remains open. Profiles retain the ordered fallback ladder, with
Virtual Screen as the validated universal path.
