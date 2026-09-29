# TCG Game Tracker

TCG Game Tracker is a desktop application for tracking trading-card-game results and statistics across multiple games and formats. Built with C++20 and wxWidgets.


## Technical Overview

This is a C++20 project with a wxWidgets UI:

- `core/`: domain logic, services, and infrastructure adapters
- `ui_wx/`: wxWidgets presentation layer
- `app/`: executable composition root

Key libraries used by the project:

- `nlohmann/json`: JSON serialization/deserialization
- `miniz`: ZIP inflate/deflate for XLSX import and export
- `doctest`: unit testing

For contributor documentation, see the [`docs/`](docs/README.md) directory.

## Quick Start

### Prerequisites

- CMake 3.22+
- Clang 14+ (preferred) or MinGW-w64 GCC 11+
- Ninja (recommended) or Make

### Build

```bash
cmake -S . -B build -G Ninja
cmake --build build --parallel
```

### Run

```bash
./build/bin/tcg-game-tracker
```

On Windows: `.\build\bin\tcg-game-tracker.exe`

### Test

```bash
ctest --test-dir build --output-on-failure
```

### System wxWidgets (faster iteration)

If you have wxWidgets 3.2 installed, skip FetchContent:

```bash
cmake -S . -B build -G Ninja -DTRACKER_USE_SYSTEM_WX=ON
cmake --build build --parallel
```

## License

MIT — see [LICENSE](LICENSE).
