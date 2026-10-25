# AGENTS.md

fast-draw is a Win32 SDK drawing application, originally written in 2005 and
brought back to a modern CMake build. It is 64-bit and Windows-only: the CMake
configuration fails fast on any other platform or toolchain.

## Layout

| Path | Purpose |
| --- | --- |
| `src/` | Application sources |
| `tests/` | `fast-draw-tests` CTest executable plus the dd smoke test |
| `cmake/` | dd's dependency manifest (`dd-dependencies.json`), which pins zlib 1.3.1 and libpng 1.6.44 as release archives by SHA-256, with method `application`: `CMakeLists.txt` builds them |
| `dd.psd1` | Project manifest: targets, presets and output paths |
| `CMakePresets.json` | `x64-debug` and `x64-release` configure/build/test presets |

## Build and verify

```pwsh
.\dd.ps1 build                   # configure + build both configs
.\dd.ps1 launch                  # build Release, then start the app
.\dd.ps1 test --label fast-draw  # build both, run fast-draw-tests
```

**`.\dd.ps1 test --label fast-draw` is the gate.** It runs the `fast-draw-tests`
CTest target in both configurations.

This repository vendors [dd](https://github.com/ZacWalk/dd) without local
changes: `dd.ps1` and `.dd/`, with hashes tracked by
[docs/dd-upstream.json](docs/dd-upstream.json). The app-specific settings are in
`dd.psd1`; zlib and libpng are pinned by SHA-256 in `cmake/dd-dependencies.json`,
which the `FetchContent` declarations in `CMakeLists.txt` read; and
`.\dd.ps1 commands --json` prints this project's commands. Upstream dd documents
its [two modes](https://github.com/ZacWalk/dd/blob/main/docs/mcp.md#two-modes)
and [command set](https://github.com/ZacWalk/dd/blob/main/docs/cli.md#commands).

## Conventions

- The app target is `fast-draw`; the CTest label is also `fast-draw`.
- libpng and zlib are acquired by CMake, not vendored into the tree.
- Keep the Win32 entry point and message loop in `src/`; do not introduce a
  cross-platform abstraction of the app's own. If the app moves off raw Win32,
  it moves onto the shared [libdesktop](https://github.com/ZacWalk/libdesktop),
  whose [todo](https://github.com/ZacWalk/libdesktop/blob/main/docs/todo.md)
  lists it as a candidate adopter.
