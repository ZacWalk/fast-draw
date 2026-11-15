# AGENTS.md

fast-draw is a vector drawing application for 64-bit Windows, built on
libdesktop's `ui` library and libfiles' PNG codec; CMake refuses to configure
for any other platform. [README.md](README.md) covers its features and
building.

## Build and verify

**`.\dd.ps1 test --label fast-draw` is the gate.** It builds both
configurations and runs the `fast-draw-tests` CTest target, which uses the
small harness in `tests/test_framework.h`.

- Everything except `src/main.cpp` builds into the `fast-draw-lib` static
  library, which both the app and the tests link; add new sources there.
- `dd.ps1` and `.dd/` are vendored [dd](https://github.com/ZacWalk/dd) without
  local changes, with hashes in [docs/dd-upstream.json](docs/dd-upstream.json).
  App settings are in `dd.psd1`. libdesktop and libfiles are pinned by
  `FAST_DRAW_LIBDESKTOP_REVISION` and `FAST_DRAW_LIBFILES_REVISION` in
  `CMakeLists.txt`; sibling `..\libdesktop` and `..\libfiles` checkouts replace
  the pins. dd documents its
  [two modes](https://github.com/ZacWalk/dd/blob/main/docs/mcp.md#two-modes)
  and [command set](https://github.com/ZacWalk/dd/blob/main/docs/cli.md#commands).

## Conventions

- The app target and the CTest label are both `fast-draw`.
- `main.cpp` owns `main` and returns `platform::run`. `MainWindow` is a
  `ui::shell`: libdesktop provides the window, the menu bar over the content,
  accelerators, placement, input, dialogs, DPI and presentation. Below the menu
  bar, one content element lays out the tab strip, sidebar and active view and
  hands each its pointer input.
- Everything is drawn with the window's `render::draw_context`: each closed
  contour of a shape is a polygon with its fill, linear gradient and border,
  arrow heads are filled polygons, and text goes through the context. Curves
  are flattened in `src/path.cpp`, whose paths also answer hit tests. Open lines
  are a round-ended polygon a segment (`ui::DrawPath`), cheaper than
  `draw_polyline` for a long path of many segments.
- PNG export and printing draw the same code into a software surface
  (`RenderDocument` in `src/view.cpp`).
- Reading a style ignores keys and values it does not know: a document that
  carries dashes, round joins, bold or italic opens without them, and a
  diagonal gradient runs down.
- `src/print_backend.cpp` holds the only Windows SDK code, the printer dialog
  and printer DC.
- libfiles owns PNG encoding and the zlib/libpng recipe. Do not add a codec
  recipe or a second PNG implementation.
- `.clang-format` is the layout: tabs, Allman braces, 120 columns.
  `.\dd.ps1 fmt` applies it to `include`, `src` and `tests` only (all of
  fast-draw's C++), and finds clang-format only on PATH; Visual Studio
  installs one in `VC\Tools\Llvm\x64\bin`. Data tables keep their hand layout
  between `// clang-format off` and `// clang-format on`.
- Every target builds at `/W4` with no warnings in Debug and Release, and
  `.clang-tidy` names the bug-finding and performance checks the code is kept
  clean against; a deliberate exception is a `// NOLINT(check): reason` on its
  line.
