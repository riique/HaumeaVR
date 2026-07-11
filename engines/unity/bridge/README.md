# UniversalXR Bridge for Unity

This package is the managed half of the controlled Unity compatibility adapter.
It never starts itself in an arbitrary game: activation requires the exact
`-uxrb-enable` command-line switch or `UXRB_UNITY_ENABLE=1` environment variable.

The package locates a gameplay camera, creates script-free per-eye cameras,
applies asymmetric projection matrices, renders into two D3D11 textures and asks
the native plug-in to copy them into named keyed-mutex shared textures. The main
OpenXR session remains owned by `uxrb_core`.

In live mode the core publishes predicted eye poses and recommended dimensions
through the versioned `UXRB_UNITY_TRANSPORT` mapping. The native plug-in keeps
each NT shared handle alive for as long as its texture is advertised, publishes
resource metadata from Unity's render thread, and becomes idle when the core
requests shutdown.

The first supported target is Unity 6000.3, Windows x64, Mono, D3D11 and the
Built-in Render Pipeline. Unsupported renderers fail closed without changing the
game camera.
