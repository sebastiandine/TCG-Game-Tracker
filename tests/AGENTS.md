# tests/AGENTS.md

`tracker_core_tests` — doctest unit tests for `tracker_core`. Hermetic, fast, no real network or disk. Read the root `AGENTS.md` first.

## File pointers

- `main.cpp` — doctest entry point with `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`. Do not put tests here.
- `fakes/InMemoryFileSystem.{hpp,cpp}` — `IFileSystem` implementation backed by `std::map`. Normalizes paths via `lexically_normal().generic_string()` (always `/` separators).
- `fakes/InMemoryFormatRepository.hpp` — `IFormatRepository` test fake backed by `std::vector`.
- `fakes/InMemoryGameTitleRepository.hpp` — `IGameTitleRepository` test fake backed by `std::vector`.
- `fakes/InMemoryDeckRepository.hpp` — `IDeckRepository` test fake backed by `std::vector` with optional `seedArchetypes()`.
- `fakes/InMemoryGameTypeRepository.hpp` — `IGameTypeRepository` test fake backed by `std::vector`.
- `fakes/InMemoryGameRepository.hpp` — `IGameRepository` test fake backed by `std::vector`.
- `result_tests.cpp` — `Result<T>` ok/err/void semantics.
- `domain_json_tests.cpp` — JSON round-trip tests for every domain type. **Update this file whenever a domain type changes.**
- `config_service_tests.cpp` — `ConfigService` against `InMemoryFileSystem`.
- `data_directory_service_tests.cpp` — `DataDirectoryService` against `InMemoryFileSystem` and a fake `IDatabaseSession`.
- `selection_tests.cpp` — `resolveSavedOrSoleId` (saved id, sole item, empty/ambiguous lists).
- `game_title_service_tests.cpp` — `GameTitleService` against `InMemoryGameTitleRepository`.
- `sqlite_game_title_repository_tests.cpp` — `SqliteGameTitleRepository` against SQLite `:memory:` databases.
- `sqlite_schema_migration_tests.cpp` — legacy Formats/GameTypes/DeckArchetypes (no `game_id`) attach to Magic: The Gathering with stable ids.
- `format_service_tests.cpp` — `FormatService` against `InMemoryFormatRepository`.
- `game_type_service_tests.cpp` — `GameTypeService` against `InMemoryGameTypeRepository`.
- `deck_service_tests.cpp` — `DeckService` against `InMemoryDeckRepository`.
- `game_service_tests.cpp` — `GameService` against `InMemoryGameRepository` (create/update/list, `clearNotes`).
- `game_import_parse_tests.cpp` — CSV/XLSX parsers and header mapping for game import.
- `game_import_service_tests.cpp` — `GameImportService` against in-memory deck/game-type/game fakes and a scripted `IGameImportSink`.
- `game_export_tests.cpp` — `mapGamesForExport` plus CSV/XLSX writers, including import round-trips.
- `sqlite_format_repository_tests.cpp` — `SqliteFormatRepository` against SQLite `:memory:` databases.
- `sqlite_game_type_repository_tests.cpp` — `SqliteGameTypeRepository` against SQLite `:memory:` databases (per-game uniqueness, no seed).
- `sqlite_deck_repository_tests.cpp` — `SqliteDeckRepository` against SQLite `:memory:` databases (per-game archetypes, FK enforcement, UNIQUE constraint).
- `sqlite_game_repository_tests.cpp` — `SqliteGameRepository` against SQLite `:memory:` databases (FK enforcement, newest-first ordering).
- `deck_grouping_tests.cpp` — `groupDecksByName` pure-function tests (collapse, sort, case-insensitive).
- `deck_statistics_tests.cpp` — `computeDeckStatistics` / `computeDeckDetailStatistics` / `computeFormatStatistics` / `MatchRecord` tests (aggregation, omitted unplayed names, opponent games ignored, win %, vs-archetype/vs-deck matchups, top/bottom 5, open notes, format overview/streak/opponents/most-played).
- `CMakeLists.txt` — explicit list of every `.cpp` (no glob).

## Conventions

1. **Framework**: doctest. Each test file `#include <doctest/doctest.h>` and uses `TEST_SUITE("...")` + `TEST_CASE("...")`. Asserts: `CHECK`, `REQUIRE`, `CHECK_THROWS`.
2. **No real I/O.** Everything goes through `tracker::testing::InMemoryFileSystem` or an inline test-local fake. The one exception is SQLite `:memory:` databases — these are in-process only and touch no host files or network.
3. **Fakes for narrow concerns stay in the test file** as anonymous-namespace classes. Promote a fake to `tests/fakes/` only when more than one test file needs it.
4. **Path strings** in expectations must use forward slashes. The fake normalizes everything to `generic_string()`. Do not hard-code `\` separators.
5. **Test names** describe behavior, not implementation. Prefer "missing file is created with defaults" over "test_init_no_file".
6. **Add a `.cpp` to the `add_executable` call in `tests/CMakeLists.txt`.** No glob.

## Required follow-ups

- After modifying any domain type field or alias you **must** extend the matching test in `domain_json_tests.cpp`.
- After adding a new service in `core/` you **must** add a corresponding test file with at least the happy-path and one error-path test.

## Commands

- Configure with tests on:
  `cmake -S . -B build -DTRACKER_BUILD_TESTS=ON`
- Build the suite:
  `cmake --build build --target tracker_core_tests`
- Run all tests:
  `ctest --test-dir build --output-on-failure`
- Run a single test by name pattern:
  `./build/bin/tracker_core_tests --test-case="*missing file*"`

## Anti-patterns

- Don't depend on `tracker_ui_wx` from tests. UI is out of scope here.
- Don't rely on file paths existing on the host. Use `InMemoryFileSystem`.
- Don't add tests that require network access.
