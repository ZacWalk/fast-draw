# AGENTS.md

fast-draw is a Win32 SDK drawing application, originally written in 2005 and
brought back to a modern CMake build. It is 64-bit and Windows-only: the CMake
configuration fails fast on any other platform or toolchain.

## Layout

| Path | Purpose |
| --- | --- |
| `src/` | Application sources |
| `tests/` | `fast-draw-tests` CTest executable plus the dd smoke test |
| `cmake/` | Dependency pins (`dd-dependencies.json`) |
| `dd.psd1` | Project manifest: targets, presets and output paths |
| `CMakePresets.json` | `x64-debug` and `x64-release` configure/build/test presets |

## dd build system

This repo uses the vendored dd build system. dd has two modes.

**CLI mode** is the default; each verb runs once and exits:

```pwsh
.\dd.ps1 test                  # build both configs and run the suite
.\dd.ps1 build debug
.\dd.ps1 doctor --json
```

**MCP mode** — `.\dd.ps1 mcp` turns the process into a stdio JSON-RPC server for an
MCP client, adapting typed requests onto CLI mode. It owns stdout for protocol
messages, so it prints no result envelope and rejects `--json`. Register it with
`.\dd.ps1 ide --mcp`.

Project settings live in `dd.psd1`; dependency pins live in
`cmake/dd-dependencies.json` with `dependencies.owner = 'dd'`. Vendored dd file
hashes are recorded in `docs/dd-upstream.json`.

## Conventions

- The app target is `fast-draw`; the CTest label is also `fast-draw`.
- libpng and zlib are acquired by CMake, not vendored into the tree.
- Keep the Win32 entry point and message loop in `src/`; do not introduce a
  cross-platform abstraction unless the app is actually being ported.
