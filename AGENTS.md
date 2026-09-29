# AGENTS.md

C++ desktop application — single wxWidgets binary built with CMake + FetchContent.

## Project structure

- `core/` — `tracker_core` static library. UI-agnostic domain, ports, services, infra adapters. **Never** depends on wxWidgets. See `core/AGENTS.md`.
- `ui_wx/` — `tracker_ui_wx` static library. The only place that touches wxWidgets. See `ui_wx/AGENTS.md`.
- `app/` — `tracker` executable (composition root). Wires concrete adapters into services. See `app/AGENTS.md`.
- `tests/` — `tracker_core_tests` doctest binary. Pure-logic tests against in-memory fakes. See `tests/AGENTS.md`.
- `docs/` — long-form developer documentation. See `docs/AGENTS.md`.
- `.github/workflows/` — GitHub Actions CI/release workflows. See `.github/workflows/AGENTS.md` for orchestrator/reusable workflow rules and CI invariants.
- `cmake/` — `Toolchain.cmake` (Clang first, MinGW-w64 fallback), `Dependencies.cmake` (FetchContent pins), `CompilerWarnings.cmake` (`tracker_warnings` interface target).
- `CMakeLists.txt` — top-level. Defines options `TRACKER_USE_SYSTEM_WX` (default OFF) and `TRACKER_BUILD_TESTS` (default ON).
  - Build metadata option: `TRACKER_APP_VERSION` (defaults to `${PROJECT_VERSION} (localbuild)` for local/manual builds, overridden by CI).

## Architecture rules (do not break)

1. Dependencies point inward only: `app` -> `ui_wx` -> `core`. `core` depends on no other first-party target.
2. `core` must not include any wx header. CI-equivalent: `rg "wx/" core/` must return zero hits.
3. Cross-boundary types are domain types and `tracker::ui::AppContext`. UI code consumes services via the references in `AppContext` — **never** by including a concrete adapter header.
4. Errors cross port boundaries as `tracker::Result<T, E=std::string>` (see `core/include/tracker/util/Result.hpp`). Do not throw across ports; reserve exceptions for genuinely unrecoverable bugs.
5. JSON layout must stay byte-for-byte stable: if you touch a domain type, update the round-trip test in `tests/domain_json_tests.cpp`.

## Toolchain

- **Compilers**: Clang 14+ preferred, MinGW-w64 GCC 11+ fallback on Windows. **Do not** add MSVC support.
- **C++ standard**: C++20 (`CMAKE_CXX_STANDARD 20`, `CXX_EXTENSIONS OFF`).
- **Build system**: CMake 3.22+ with `FetchContent`. Pin every dep by tag in `cmake/Dependencies.cmake`; never use `master`.

## Key dependencies

| Library | Version | Purpose |
|---|---|---|
| nlohmann/json | v3.11.3 | All JSON serde |
| miniz | 3.0.2 | In-memory ZIP inflate for XLSX import (MIT) |
| SQLite | 3.53.4 (amalgamation) | Embedded SQL database for persistent data storage (public domain) |
| wxWidgets | v3.2.5 | UI toolkit (only `ui_wx/` may use it) |
| doctest | v2.4.11 | Tests (only when `TRACKER_BUILD_TESTS=ON`) |

## Commands

Run from the **workspace root**.

- Configure (Clang/Ninja, FetchContent wx):
  `cmake -S . -B build -G Ninja`
- Configure (Windows MinGW-w64 fallback):
  `cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release`
- Configure with system wx for fast iteration:
  `cmake -S . -B build -G Ninja -DTRACKER_USE_SYSTEM_WX=ON`
- Build everything:
  `cmake --build build --parallel`
- Run the app:
  `./build/bin/tcg-game-tracker` (`.\build\bin\tcg-game-tracker.exe` on Windows)
- Run tests (TRACKER_BUILD_TESTS defaults to ON):
  `ctest --test-dir build --output-on-failure`
- Build tests only:
  `cmake --build build --target tracker_core_tests`

> **Windows runtime note**: with MinGW-w64 you need `libgcc_s_seh-1.dll` and `libstdc++-6.dll` from your MSYS2 UCRT64 `bin/` on `PATH` (or copied alongside the exe) to launch from Explorer.
>
> **Windows rebuild note**: linking `tcg-game-tracker.exe` fails with `Permission denied` if the app is still running/locked. Close the exe before rebuilding app targets.

## UI performance guardrails

- Keep first paint responsive: avoid heavy synchronous work in constructors of top-level windows/dialogs.
- For startup, defer non-critical work with `CallAfter(...)` so the frame appears before data loading.
- While constructing/populating dialogs with many controls/choices, wrap with `Freeze()`/`Thaw()` and append choice items in bulk (`wxArrayString`) to reduce layout/repaint churn.

## Windows UI theming guardrails

- `wxWidgets` native dark-mode behavior on Windows is inconsistent across controls and OS builds; prefer explicit app theming in `ui_wx/src/Theme.cpp` plus targeted native hints only where needed.
- Treat UI text from domain/services as UTF-8 and convert explicitly at wx boundaries (`wxString::FromUTF8(...)` for display, `ToStdString(wxConvUTF8)` for write-back); do not rely on implicit `std::string` conversions on Windows.
- For dialogs (`wxDialog`) and frames (`wxFrame`), apply title-bar dark mode through top-level-window handling (not frame-only handling), otherwise modal window headers stay light.
- Do **not** use native `wxMenuBar` on Windows; it stays light in dark mode. Host File/Games/Formats/Data/Help as a themed `wxPanel` strip that opens `PopupMenu` (same approach as CCM3).
- Do **not** apply `Explorer` class theming to `wxTextCtrl` in dark mode; some Windows builds force black typed text. Keep edit controls palette-driven via `applyPaletteToTextCtrl` / `hardenTextCtrlNativeTheme` in `Theme.cpp`.
- Theme modal dialogs explicitly before `ShowModal()` so they don't inherit mismatched defaults from Windows.
- For button hover/pressed contrast fixes in dark theme, prefer explicit state handling in `Theme.cpp`.
- Keep button theming state dynamic across theme switches (Dark <-> Light). Avoid lambdas that permanently capture old theme colors.
- After changing `ui_wx` theming behavior, rebuild the final app target (`cmake --build build --target tracker --parallel`), not just `tracker_ui_wx`, before validating runtime behavior.
- If linker fails with `Permission denied` on `build/bin/tcg-game-tracker.exe`, the app is still running; close it before rebuilding.

## Required follow-ups

- After modifying a domain type's fields or JSON layout you **must** update the matching round-trip test in `tests/domain_json_tests.cpp` and re-run tests.
- After adding a new `.cpp` to `core/` or `ui_wx/` you **must** add it to that package's `CMakeLists.txt`. There is no glob.
- After adding a new dependency you **must** verify its license is compatible with this repository's MIT license before merging.
- For new code, keep duplication to an absolute minimum: prefer extracting shared helpers/components instead of copy/paste so Sonar duplication stays comfortably below the quality gate.
- For new code, add or update unit tests so behavior is covered and overall test coverage remains high.
- **Data management must be documented**: all database tables, columns, keys, relations, and the DB file location **must** be kept up to date in `docs/database/`. Adding or modifying a table without updating `docs/database/schema.md` is not done.

## Agent collaboration (Cursor / AI)

- **Never** `git commit` or `git push` unless the user **explicitly** asked you to commit and/or push (e.g. "commit this", "push to origin"). Preparing diffs and suggesting commands is fine; performing those Git writes without explicit instruction is not.
- **Never** check out another branch **to change it** unless the user **explicitly** asked you to work on that branch.

## Anti-patterns

- Don't include `wx/...` headers from `core/` (breaks layering and tests will refuse to build).
- Don't add tests that hit real network or real disk; use the fake `tracker::testing::InMemoryFileSystem`.
- Don't enable `-Wconversion` / `-Wsign-conversion`; they fight wxWidgets's `int` IDs. They were intentionally removed from `cmake/CompilerWarnings.cmake`.
- Don't use `master` for FetchContent tags. Bump deliberately.
- Don't create multiple top-level triggers for the same CI intent (feature or master). Keep one triggered orchestrator workflow and use `workflow_call` reusable workflows for OS-specific splits.
