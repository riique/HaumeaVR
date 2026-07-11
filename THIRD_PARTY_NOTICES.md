# Third-party notices

UniversalXR Bridge fetches the following source dependencies at configure time.
Their original license files remain in each CMake FetchContent source directory.

## Khronos OpenXR-SDK — `release-1.1.58`

Repository: https://github.com/KhronosGroup/OpenXR-SDK

Licensed under Apache License 2.0. Copyright The Khronos Group Inc.

## nlohmann/json — `v3.12.0`

Repository: https://github.com/nlohmann/json

Licensed under the MIT License. Copyright Niels Lohmann and contributors.

## Unity NativeRenderingPlugin — `f703c47a140d343c2c863a36d1aa5832586f3aaa`

Repository: https://github.com/Unity-Technologies/NativeRenderingPlugin

Repository source is licensed under the MIT License, copyright 2016 Unity
Technologies. Unity interface headers retain their upstream license notices.

## Microsoft Detours — `v4.0.1`

Repository: https://github.com/microsoft/Detours

Licensed under the MIT License. Copyright Microsoft Corporation. UXRB compiles
only the detour/decoder/image/module library sources; the upstream
process-creation and DLL-injection helper source is intentionally excluded.

## Published launcher/runtime components

The Windows self-contained bundle also contains the following restored runtime
and NuGet components. Package metadata and upstream license texts remain in the
NuGet package cache used to produce the build.

- .NET Runtime `10.0.9` — MIT; copyright .NET Foundation and contributors;
- Avalonia UI `12.1.0` (including Win32, Skia and HarfBuzz integration) and
  Avalonia BuildServices `11.3.2` — MIT; copyright Avalonia contributors;
- Avalonia ANGLE Windows natives `2.1.27548.20260419` — upstream ANGLE/Chromium
  license notices: https://github.com/AvaloniaUI/angle;
- SkiaSharp `3.119.4` and HarfBuzzSharp `8.3.1.3` — MIT; copyright Microsoft
  Corporation and contributors, with their bundled upstream native notices;
- Microsoft.Data.Sqlite `10.0.9` — MIT; copyright .NET Foundation and
  contributors;
- SQLitePCLRaw `3.0.3` — Apache License 2.0; copyright Eric Sink;
- SQLite/SourceGear.sqlite3 `3.50.4.5` — SQLite public-domain dedication;
- MicroCom.Runtime `0.11.6` and Tmds.DBus.Protocol `0.94.1` — MIT.

License identifiers and texts: https://licenses.nuget.org/. The SQLite public
domain dedication is at https://sqlite.org/copyright.html.
