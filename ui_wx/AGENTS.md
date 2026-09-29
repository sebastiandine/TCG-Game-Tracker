# ui_wx/AGENTS.md

`tracker_ui_wx` static library — wxWidgets adapter. The **only** target that may include `wx/...` headers. Read the root `AGENTS.md` first.

## Layer pointers

- `include/tracker/ui/AppContext.hpp` — the boundary type. A struct of references to shared core services (`ConfigService`, `DataDirectoryService`, `GameTitleService`, `FormatService`, `DeckService`, `GameTypeService`, `GameService`, `GameImportService`). UI code talks to core only through this struct, never by including a concrete adapter header.
- `include/tracker/ui/MainFrame.hpp` + `src/MainFrame.cpp` — top-level window, CCM3-style themed menu strip (File / Games / Formats / Data / Help popup labels, not a native `wxMenuBar`), and content area. When no format is selected a centered label is shown; when a format is selected a `FormatWorkspace` replaces the label. Theme is applied on load and after Settings. `restoreSelection` reopens the saved game/format, or the only game/format when that is all the database has. Changing the data directory in Settings calls `DataDirectoryService::activate` immediately.
- `include/tracker/ui/FormatWorkspace.hpp` + `src/FormatWorkspace.cpp` — per-format workspace with a CCM3-style custom tab bar over a `wxSimplebook`. Hosts tabs: **Games**, **Statistics**, and **Decks**, plus closeable per-deck statistics tabs opened from the Statistics tree.
- `include/tracker/ui/DecksPanel.hpp` + `src/DecksPanel.cpp` — inline form for creating decks (name, archetype, variante, variante note) and a grouped `wxTreeCtrl` listing decks by name, scoped to the selected format. Archetypes are loaded for the selected game.
- `include/tracker/ui/GamesPanel.hpp` + `src/GamesPanel.cpp` — CCM3-style toolbar (plus button + filter input) over a `wxListCtrl` listing recorded games sorted newest-first. Supports column sort and case-insensitive filter. Add/edit via `GameDialog`. Columns: Date, Deck, Opponent Deck, Result, Game Type, Opponent, Notes.
- `include/tracker/ui/StatisticsPanel.hpp` + `src/StatisticsPanel.cpp` — per-format statistics dashboard: KPI cards (Games/W/L/D/Win %), extra cards (Decks played, unique opponents, current streak), ranked best/worst/most-played decks, and a filterable Games + W/L/D + win % tree list (`wxTreeListCtrl`). Parent rows aggregate a main deck (core + variants) and stay collapsed; expand shows `Name (default)` then `Variante (variante)`. Clicking a parent or core row opens a family-aggregate tab; clicking a named variant opens a variant-only tab.
- `include/tracker/ui/StatsWidgets.hpp` + `src/StatsWidgets.cpp` — shared KPI cards and ranked lists used by Statistics and DeckStatistics.
- `include/tracker/ui/DeckStatisticsPanel.hpp` + `src/DeckStatisticsPanel.cpp` — closeable per-deck statistics dashboard: KPI cards (Games/W/L/D/Win %), side-by-side ranked best/worst matchups, Open Notes (opponent deck + note, Approve clears `Game::notes`), and one internally scrolling vs-archetype / vs-deck table switched by toggle (filterable, column-sortable, default Win %).
- `include/tracker/ui/GameDialog.hpp` + `src/GameDialog.cpp` — modal dialog for creating or editing a game record: date, deck, variante, opponent deck, result (Win/Loss/Draw), score (2-1, 2-0, 1-2, 0-2, 1-1), game type, opponent (free-text name), notes. Score auto-sets result.
- `include/tracker/ui/SettingsDialog.hpp` + `src/SettingsDialog.cpp` — edits `Configuration` via `ConfigService::store`. Data directory + Light/Dark theme choice.
- `include/tracker/ui/CreateGameTitleDialog.hpp` + `src/CreateGameTitleDialog.cpp` — modal dialog for creating a game. Validates via `GameTitleService::create`.
- `include/tracker/ui/CreateFormatDialog.hpp` + `src/CreateFormatDialog.cpp` — modal dialog for creating a format under the selected game. Validates via `FormatService::create`.
- `include/tracker/ui/CreateGameTypeDialog.hpp` + `src/CreateGameTypeDialog.cpp` — modal dialog for creating a game type under the selected game (name, competitiveness, medium). Validates via `GameTypeService::create`.
- `include/tracker/ui/CreateArchetypeDialog.hpp` + `src/CreateArchetypeDialog.cpp` — modal dialog for creating an archetype under the selected game. Validates via `DeckService::createArchetype`.
- `include/tracker/ui/ImportGamesDialog.hpp` + `src/ImportGamesDialog.cpp` — modal dialog for importing game records from CSV/XLSX into a format of the selected game. A progress dialog reports row status; new core decks prompt for archetype unless the name contains one of that game's archetype labels, and new events prompt for competitiveness and paper/online.
- `include/tracker/ui/Theme.hpp` + `src/Theme.cpp` — shared theme helpers: `paletteForTheme`, `applyThemeToWindowTree`, `themeModalDialog`, `showThemedMessageDialog`, `showThemedConfirmDialog`, dark-mode button painting, MSW title-bar dark mode, text-ctrl hardening, placeholder painting.
- `include/tracker/ui/AppVersion.hpp.in` — `kAppVersion` from `TRACKER_APP_VERSION` CMake variable.

## Conventions

1. **Only consume core through `AppContext`.** Do not include any header from `tracker/infra/` here.
2. **Ownership**: dialogs and panels are heap-allocated and parented to a `wxWindow`. wxWidgets owns the lifetime — do **not** wrap them in `unique_ptr`.
3. **wxFont modifications** mutate in place: `font.MakeBold().MakeLarger()` — do not call `Scale` (it does not exist on wxFont 3.2).
4. **No `tracker_warnings`.** This target intentionally does **not** link the strict warning interface — wxWidgets headers trip `-Wpedantic` / `-Wshadow`. Keep it that way.
5. **String encoding on Windows (avoid mojibake):**
   - Domain/service strings are UTF-8 `std::string`. Do not rely on implicit `std::string <-> wxString` conversions on Windows.
   - UI display path (`std::string` -> wx control): always convert with `wxString::FromUTF8(str.c_str())`.
   - UI write-back path (wx control -> `std::string`): always convert with `ToStdString(wxConvUTF8)`.
6. **Theme consistency rules (Windows):**
   - Theme dialogs before `ShowModal()` with `themeModalDialog(...)`.
   - Do not use native `wxMessageBox` / `wxAboutBox` for app-facing flows that must match dark mode. Use themed popup helpers.
   - Center popup dialogs on the app window (`CentreOnParent()`).
   - Do not call `applyNativeClassTheme(..., "DarkMode_Explorer", "Explorer")` for `wxTextCtrl`.
   - When validating UI theming changes, rebuild and run `tracker` (the executable), not just `tracker_ui_wx`.
   - Do not use native `wxMenuBar` on Windows; it stays light in dark mode. Use a themed `wxPanel` strip with `PopupMenu` like CCM3.

## Required follow-ups

- After adding a new dialog/panel `.cpp` you **must** add it to `ui_wx/CMakeLists.txt`.
- After changing `AppContext` you **must** update `app/main.cpp` so the composition root populates the new field.

## Commands

Build UI only: `cmake --build build --target tracker_ui_wx`
