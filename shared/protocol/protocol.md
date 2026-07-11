# UXRB local IPC protocol

The launcher and native runtime communicate through a local Windows named pipe. The launcher generates a random 256-bit token for each runtime process and passes it through the child process environment. The token is never stored in a profile or database.

## Transport

- Pipe path: `\\.\pipe\uxrb-<random-session-id>`.
- Local machine only; remote pipe clients are rejected.
- One launcher connection per runtime process.
- Binary frame header followed by UTF-8 JSON.
- Maximum JSON payload: 1 MiB.

## Frame header

All integer fields use little-endian encoding. The header is 24 bytes.

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 4 | ASCII magic `UXRB` |
| 4 | 2 | Protocol version (`1`) |
| 6 | 2 | Message kind |
| 8 | 4 | Payload byte count |
| 12 | 4 | Reserved flags (`0`) |
| 16 | 8 | Request identifier |

## Authentication

The first client frame must be `Handshake` and contain:

```json
{
  "client": "launcher",
  "protocolVersion": 1,
  "token": "64 lowercase hexadecimal characters"
}
```

The runtime compares the token in constant time. It responds with `AuthResult` and closes the pipe immediately when authentication fails.

## Message kinds

| Value | Name | Direction |
| ---: | --- | --- |
| 1 | Handshake | Launcher to runtime |
| 2 | AuthResult | Runtime to launcher |
| 3 | Command | Launcher to runtime |
| 4 | Event | Runtime to launcher |
| 5 | Response | Either direction |
| 6 | Error | Either direction |
| 7 | Shutdown | Launcher to runtime |

The `requestId` correlates commands and responses. Events use `0` unless they are causally tied to a request.

## Commands

- `get-status` reports runtime state, validated foundations and safety properties.
- `start-virtual-screen` accepts a live `windowHandle` plus screen width, distance and curvature.
- `start-unity-stereo` accepts a random `Local\\UXRB-Unity-<id>` transport name and an optional expected Unity process ID. The core owns the mapping and rejects pre-existing mappings.
- `recenter` queues recentering on the render thread.
- `stop-session` stops the active session and closes the per-process core.
