# app/AGENTS.md

The `tracker` executable — composition root only. The single place where concrete adapter types are mentioned. Read the root `AGENTS.md` first.

## File pointers

- `main.cpp` — the entire app. Defines `TrackerApp : public wxApp`, builds the dependency graph in `OnInit()`, then hands an `AppContext` to `MainFrame`.
- `CMakeLists.txt` — declares the `tracker` target. Sets `WIN32_EXECUTABLE TRUE` on Windows so no console window appears. Links `tracker_core`, `tracker_ui_wx`, `tracker_warnings`.

## Conventions

1. **Composition root is the only place** that names concrete adapters: `StdFileSystem`, `SqliteDatabase`, `SqliteGameTitleRepository`, `SqliteFormatRepository`, `SqliteDeckRepository`, `SqliteGameTypeRepository`, `SqliteGameRepository`, `ConfigService`, `DataDirectoryService`, `GameTitleService`, `FormatService`, `DeckService`, `GameTypeService`, `GameService`, `GameImportService`. If a concrete adapter type appears anywhere else in the codebase, move the wiring here.
2. **Member declaration order in `TrackerApp` matters** — destruction is reverse, so a member that depends on another must be declared **after** its deps. Do not reorder casually.
3. **Use `std::unique_ptr` for everything owned** by `TrackerApp`. The `AppContext` then holds plain references into those owned objects.
4. **`config.json` location** is the executable's parent directory, resolved via `wxStandardPaths::Get().GetExecutablePath()`. Do not change this.
5. **Image format handlers** must be registered via `wxImage::AddHandler(new wxPNGHandler)` and `new wxJPEGHandler` before any image is loaded. They are added in `OnInit()` first thing.

## Required follow-ups

- After adding a new core service you **must** add a `unique_ptr<...>` member, construct it in `OnInit()` after its deps, and add a reference field to `AppContext`.
- After adding a new dependency edge you **must** verify destruction order is still correct: deps **before** dependents in the member list.

## Anti-patterns

- Don't add business logic here. If something is more than `std::make_unique` and a `Bind` call, it belongs in `core/`.
- Don't construct services on the stack inside `OnInit()` — they must outlive the `MainFrame`, so they live as `TrackerApp` members.

## Commands

- Build the binary: `cmake --build build --target tracker`
- Run on Windows / MinGW-w64: `.\build\bin\tcg-game-tracker.exe`. The MSYS2 UCRT64 runtime (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`) needs to be on `PATH`.
