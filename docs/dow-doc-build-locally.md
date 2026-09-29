# Build Locally Guide

This guide explains how to build TCG Game Tracker on Windows and Linux, how dependencies are resolved, and how to run tests locally.

## Build Model

The project uses CMake and builds one desktop executable:

- executable target: `tracker` (output binary: `tcg-game-tracker` / `tcg-game-tracker.exe`)
- language standard: C++20
- layered targets: `tracker_core` (logic), `tracker_ui_wx` (wx UI), `tracker` (composition root)

## Dependency Model

Dependencies are managed with CMake `FetchContent` in `cmake/Dependencies.cmake`.

Pinned versions:

- `nlohmann/json` `v3.11.3`
- `miniz` `3.0.2` (ZIP inflate/deflate for XLSX import and export)
- `SQLite` `3.53.4` (amalgamation — `sqlite3.c` compiled as a static lib)
- `wxWidgets` `v3.2.5`
- `doctest` `v2.4.11` (only when tests are enabled)

On first configure, CMake downloads sources. On first full build, heavy dependencies (especially wxWidgets) build locally. Later builds reuse cached dependencies under `build/_deps`.

## Use System wxWidgets

By default, the build fetches wxWidgets. For faster local iteration with an installed wxWidgets, set `-DTRACKER_USE_SYSTEM_WX=ON`.

## Prerequisites

### Windows

Recommended: Clang + Ninja

- LLVM/Clang 14+ on `PATH`
- CMake 3.22+
- Ninja on `PATH`

Verified fallback: MSYS2 UCRT64 + MinGW-w64 GCC

- MSYS2 UCRT64 toolchain (`gcc`, `cmake`, `make` or `ninja`)
- CMake 3.22+

MSVC is intentionally not supported.

### Linux

- GCC 11+ or Clang 14+
- CMake 3.22+
- Ninja (recommended)
- wxWidgets 3.2 dev headers (`libwxgtk3.2-dev` on Ubuntu/Debian) for system-wx builds, or let FetchContent build it from source

## Configure And Build

### Clang + Ninja (any OS)

```bash
cmake -S . -B build -G Ninja
cmake --build build --parallel
```

### MinGW-w64 (Windows MSYS2)

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### System wxWidgets (faster iteration)

```bash
cmake -S . -B build -G Ninja -DTRACKER_USE_SYSTEM_WX=ON
cmake --build build --parallel
```

## Run The App

```bash
./build/bin/tcg-game-tracker
```

On Windows: `.\build\bin\tcg-game-tracker.exe`

## Run Tests

```bash
ctest --test-dir build --output-on-failure
```

Tests are enabled by default (`TRACKER_BUILD_TESTS=ON`).

## CMake Options

| Option | Default | Purpose |
|---|---|---|
| `TRACKER_USE_SYSTEM_WX` | `OFF` | Use installed wxWidgets instead of FetchContent |
| `TRACKER_BUILD_TESTS` | `ON` | Build unit test suite |
| `TRACKER_APP_VERSION` | `${PROJECT_VERSION} (localbuild)` | Embedded version string |

## Troubleshooting

- **Permission denied linking on Windows**: close the running app before rebuilding.
- **Missing DLLs on Windows**: ensure MSYS2 UCRT64 `bin/` is on `PATH` or copy runtime DLLs next to the exe.
- **Long first build**: wxWidgets FetchContent compile is slow once. Use `-DTRACKER_USE_SYSTEM_WX=ON` to skip it if you have system wx.
