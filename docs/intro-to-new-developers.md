# Intro For New Developers

Welcome to TCG Game Tracker. This document orients you in the repository and points to deeper references.

## Architecture

The project is a single C++20 desktop executable built with CMake and wxWidgets.

Three first-party targets:

1. **`tracker_core`** (`core/`) — domain types, services, ports, infra adapters. UI-free. Tested by `tracker_core_tests`.
2. **`tracker_ui_wx`** (`ui_wx/`) — wxWidgets adapter. The only place that touches `wx/...` headers.
3. **`tracker`** (`app/`) — composition root. Wires concrete adapters into services and launches the UI.

Dependencies point inward: `app -> ui_wx -> core`. `core` must never include a wx header.

## Key Boundaries

- **Domain vs UI**: all domain types live in `core/include/tracker/domain/`. The UI consumes them through `AppContext` references.
- **Ports vs Infra**: `core/include/tracker/ports/` defines interfaces; `core/include/tracker/infra/` + `core/src/infra/` implements them. Tests use fakes from `tests/fakes/`.
- **Result<T>**: fallible operations return `tracker::Result<T>`, not exceptions.

## First Steps

1. Build: `cmake -S . -B build -G Ninja && cmake --build build --parallel`
2. Run tests: `ctest --test-dir build --output-on-failure`
3. Launch the app: `./build/bin/tcg-game-tracker`
4. Read `AGENTS.md` at the repo root for commands, toolchain rules, and anti-patterns.

## Common Pitfalls

- Including `wx/...` from `core/` breaks layering and tests.
- Implicit `std::string <-> wxString` on Windows causes mojibake. Use `FromUTF8` / `ToStdString(wxConvUTF8)`.
- Don't use `-Wconversion` — it fights wxWidgets's int IDs.
- Theme dialogs before `ShowModal()` or they stay light in dark mode.

## Deeper Docs

- [Build Locally](dow-doc-build-locally.md)
- [Testing Guide](testing-and-test-code-of-conduct.md)
- [CI/CD Guide](ci-cd-guide.md)
