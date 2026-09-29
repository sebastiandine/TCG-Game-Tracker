# core/AGENTS.md

`tracker_core` static library — domain types, ports, services, infra adapters. Hard rule: **no UI dependencies, ever**. Read the root `AGENTS.md` first.

## Layer pointers

- `include/tracker/domain/` — POD value types: `Enums` (`Theme`, `Competitiveness`, `PlayMedium`, `MatchResult`, `MatchScore`), `Configuration`, `GameTitle`, `Format`, `GameType`, `Deck`, `DeckArchetype`, `DeckGroup`, `DeckStatistics`, `Game`. Types with JSON serde have `to_json` / `from_json` in the matching `src/domain/*.cpp`. `DeckGroup` has a grouping helper (`groupDecksByName`) implemented in `src/domain/DeckGroup.cpp`. `DeckStatistics` has `computeDeckStatistics` (player-side W/L/D + win %), `computeDeckDetailStatistics` (per-deck overview, vs-archetype / vs-deck matchups, top/bottom 5, open notes), and `computeFormatStatistics` (format-wide overview, unique opponents, streak, best/worst/most-played decks) in `src/domain/DeckStatistics.cpp`. `Selection.hpp` has `resolveSavedOrSoleId` (keep a saved id if present, otherwise the only remaining item).
- `include/tracker/ports/` — interfaces: `IFileSystem`, `IDatabaseSession`, `IGameTitleRepository`, `IFormatRepository`, `IDeckRepository`, `IGameTypeRepository`, `IGameRepository`, `IGameImportSink`. All seams the services depend on. Add new ports here when adding new external concerns.
- `include/tracker/infra/` — concrete adapters: `StdFileSystem`, `SqliteDatabase` (implements `IDatabaseSession`), `SqliteGameTitleRepository`, `SqliteFormatRepository`, `SqliteDeckRepository`, `SqliteGameTypeRepository`, `SqliteGameRepository`.
- `include/tracker/services/` — high-level operations: `ConfigService`, `DataDirectoryService` (`activate` ensures a data directory and opens `{dir}/tracker.db`), `GameTitleService`, `FormatService`, `DeckService` (includes `createArchetype` / `listByGame`), `GameTypeService`, `GameService` (includes `clearNotes`), `GameImportService`. They depend only on ports / domain.
- `include/tracker/import/` — CSV/XLSX spreadsheet parsers and header mapping used by game import (`GameImportParse.hpp`).
- `include/tracker/export/` — CSV/XLSX spreadsheet writers and game-to-row mapping used by game export (`GameExportWrite.hpp`).
- `include/tracker/util/` — `Result.hpp` (the sum type).
- `src/` mirrors `include/tracker/` for non-template implementations.

SQLite headers (`sqlite3.h`) are included only in `src/infra/` translation units, never in public headers. The `SqliteDatabase.hpp` header forward-declares `struct sqlite3` to keep the dependency private. miniz (`miniz.h`) is included only in `src/import/Xlsx.cpp` and `src/export/Xlsx.cpp`.

## Conventions

1. **No throw across ports.** Return `tracker::Result<T>::ok(...)` / `Result<T>::err("msg")`. The caller propagates with `if (!r) return Result<T>::err(r.error());`.
2. **JSON serde stays byte-for-byte stable.** When the C++ field name differs from the JSON key, write hand-rolled `to_json` / `from_json`. Round-trip tests in `tests/domain_json_tests.cpp` enforce this.
3. **Path strings** that get persisted (e.g. `Configuration::dataStorage`) use `std::filesystem::path::generic_string()`, never `string()` — keeps `/` separators on Windows so JSON round-trips and tests stay portable.
4. **Compiler warnings**: every target in this package links `tracker_warnings` `PRIVATE`. Treat warnings as errors locally during dev.
5. **No `wx/...` includes** in headers or sources here. Verify with `rg "wx/" core/` — must be empty.

## Adding a new port

1. Add the interface header under `include/tracker/ports/` with `virtual ~IFoo() = default;`.
2. Implement the adapter under `include/tracker/infra/` + `src/infra/`. Mark it `final`.
3. Update `core/CMakeLists.txt`. Wire it into the relevant service's constructor.
4. Add a fake under `tests/fakes/` modeled on `InMemoryFileSystem` and write service-level tests against it.

## Commands

Build core only: `cmake --build build --target tracker_core`
