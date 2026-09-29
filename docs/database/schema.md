# Database Schema

This document describes all tables in `tracker.db`. Keep it up to date whenever a table is added, modified, or removed.

## GameTitles

Stores user-defined games (e.g. "Magic: The Gathering", "Lorcana"). Formats, game types, and archetypes belong to a title.

```sql
CREATE TABLE GameTitles (
  id   INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT    NOT NULL UNIQUE COLLATE NOCASE
);
```

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `id` | `INTEGER` | `PRIMARY KEY AUTOINCREMENT` | Consecutive numeric identifier. |
| `name` | `TEXT` | `NOT NULL UNIQUE COLLATE NOCASE` | Human-readable game name. Uniqueness is case-insensitive. |

### Notes

- `AUTOINCREMENT` guarantees that ids are never reused, even after deletion.
- New databases start with no titles; the user creates them.
- Existing databases that predate this table get one title, **Magic: The Gathering**, and existing formats, game types, and archetypes are attached to it.
- The application trims whitespace and rejects empty names before insertion.

## Formats

Stores format definitions scoped to a game title (e.g. "Standard", "Commander").

```sql
CREATE TABLE Formats (
  id      INTEGER PRIMARY KEY AUTOINCREMENT,
  game_id INTEGER NOT NULL,
  name    TEXT    NOT NULL COLLATE NOCASE,
  FOREIGN KEY (game_id) REFERENCES GameTitles(id),
  UNIQUE (game_id, name)
);
```

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `id` | `INTEGER` | `PRIMARY KEY AUTOINCREMENT` | Consecutive numeric identifier. |
| `game_id` | `INTEGER` | `NOT NULL, FK → GameTitles(id)` | The game this format belongs to. |
| `name` | `TEXT` | `NOT NULL COLLATE NOCASE` | Human-readable format name. Unique per game, case-insensitive. |

### Notes

- Two games may both have a format named "Standard".
- The application trims whitespace and rejects empty names before insertion.

## DeckArchetypes

Lookup table of deck archetype categories scoped to a game title. Users create archetypes; none are seeded.

```sql
CREATE TABLE DeckArchetypes (
  id      INTEGER PRIMARY KEY AUTOINCREMENT,
  game_id INTEGER NOT NULL,
  name    TEXT    NOT NULL COLLATE NOCASE,
  FOREIGN KEY (game_id) REFERENCES GameTitles(id),
  UNIQUE (game_id, name)
);
```

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `id` | `INTEGER` | `PRIMARY KEY AUTOINCREMENT` | Consecutive numeric identifier. |
| `game_id` | `INTEGER` | `NOT NULL, FK → GameTitles(id)` | The game this archetype belongs to. |
| `name` | `TEXT` | `NOT NULL COLLATE NOCASE` | Archetype label (e.g. "Aggro", "Control"). Unique per game, case-insensitive. |

### Notes

- Existing databases that predate `game_id` keep their previous global rows (typically Aggro, Midrange, Control, Combo) attached to **Magic: The Gathering**.
- The application trims whitespace and rejects empty names before insertion.

## GameTypes

Lookup table of event type definitions scoped to a game title. Users create game types; none are seeded.

```sql
CREATE TABLE GameTypes (
  id              INTEGER PRIMARY KEY AUTOINCREMENT,
  game_id         INTEGER NOT NULL,
  name            TEXT    NOT NULL COLLATE NOCASE,
  competitiveness TEXT    NOT NULL,
  medium          TEXT    NOT NULL,
  FOREIGN KEY (game_id) REFERENCES GameTitles(id),
  UNIQUE (game_id, name)
);
```

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `id` | `INTEGER` | `PRIMARY KEY AUTOINCREMENT` | Consecutive numeric identifier. |
| `game_id` | `INTEGER` | `NOT NULL, FK → GameTitles(id)` | The game this event type belongs to. |
| `name` | `TEXT` | `NOT NULL COLLATE NOCASE` | Human-readable game type name. Unique per game, case-insensitive. |
| `competitiveness` | `TEXT` | `NOT NULL` | `"Competitive"` or `"Non-Competitive"`. |
| `medium` | `TEXT` | `NOT NULL` | `"Paper"` or `"Online"`. |

### Notes

- Existing databases that predate `game_id` keep their previous global rows (typically MTGO Friendly, MTGO League, FNM, Tournament) attached to **Magic: The Gathering**.
- The application trims whitespace and rejects empty names before insertion.

## Decks

Stores deck definitions scoped to a format, with an archetype and optional variant.

```sql
CREATE TABLE Decks (
  id            INTEGER PRIMARY KEY AUTOINCREMENT,
  format_id     INTEGER NOT NULL,
  archetype_id  INTEGER NOT NULL,
  name          TEXT    NOT NULL COLLATE NOCASE,
  variant       TEXT    NOT NULL DEFAULT '' COLLATE NOCASE,
  variant_note  TEXT    NOT NULL DEFAULT '',
  FOREIGN KEY (format_id)    REFERENCES Formats(id),
  FOREIGN KEY (archetype_id) REFERENCES DeckArchetypes(id),
  UNIQUE (format_id, name, variant)
);
```

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `id` | `INTEGER` | `PRIMARY KEY AUTOINCREMENT` | Consecutive numeric identifier. |
| `format_id` | `INTEGER` | `NOT NULL, FK → Formats(id)` | The format this deck belongs to. |
| `archetype_id` | `INTEGER` | `NOT NULL, FK → DeckArchetypes(id)` | The deck's archetype category. |
| `name` | `TEXT` | `NOT NULL COLLATE NOCASE` | Deck name (e.g. "Landstill"). |
| `variant` | `TEXT` | `NOT NULL DEFAULT '' COLLATE NOCASE` | Variant label (e.g. "Glaciers"). Empty string for base deck. |
| `variant_note` | `TEXT` | `NOT NULL DEFAULT ''` | Free-text notes about this variant. |

### Notes

- `UNIQUE (format_id, name, variant)` prevents duplicate deck+variant combinations within a format (case-insensitive via `COLLATE NOCASE` on both `name` and `variant`).
- An empty `variant` is allowed and represents the "base" version of a deck.
- Foreign key enforcement is enabled via `PRAGMA foreign_keys = ON` on every database open.

## Games

Stores recorded match results scoped to a format.

```sql
CREATE TABLE Games (
  id               INTEGER PRIMARY KEY AUTOINCREMENT,
  format_id        INTEGER NOT NULL,
  played_on        TEXT    NOT NULL,
  deck_id          INTEGER NOT NULL,
  opponent_deck_id INTEGER NOT NULL,
  opponent         TEXT    NOT NULL DEFAULT '',
  result           TEXT    NOT NULL,
  score            TEXT    NOT NULL,
  game_type_id     INTEGER NOT NULL,
  notes            TEXT    NOT NULL DEFAULT '',
  FOREIGN KEY (format_id)        REFERENCES Formats(id),
  FOREIGN KEY (deck_id)          REFERENCES Decks(id),
  FOREIGN KEY (opponent_deck_id) REFERENCES Decks(id),
  FOREIGN KEY (game_type_id)     REFERENCES GameTypes(id)
);
CREATE INDEX idx_games_format_date
  ON Games (format_id, played_on DESC, id DESC);
```

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `id` | `INTEGER` | `PRIMARY KEY AUTOINCREMENT` | Consecutive numeric identifier. |
| `format_id` | `INTEGER` | `NOT NULL, FK → Formats(id)` | The format this game was played in. |
| `played_on` | `TEXT` | `NOT NULL` | Date the game was played (`YYYY-MM-DD`). |
| `deck_id` | `INTEGER` | `NOT NULL, FK → Decks(id)` | The player's deck (specific variant row). |
| `opponent_deck_id` | `INTEGER` | `NOT NULL, FK → Decks(id)` | The opponent's deck (main deck row, no variant). |
| `opponent` | `TEXT` | `NOT NULL DEFAULT ''` | Free-text opponent name. Empty string when unknown. |
| `result` | `TEXT` | `NOT NULL` | `"Win"`, `"Loss"`, or `"Draw"`. |
| `score` | `TEXT` | `NOT NULL` | Game score: `"2-1"`, `"2-0"`, `"1-2"`, `"0-2"`, or `"1-1"`. |
| `game_type_id` | `INTEGER` | `NOT NULL, FK → GameTypes(id)` | The game type (e.g. MTGO League, FNM). |
| `notes` | `TEXT` | `NOT NULL DEFAULT ''` | Free-text notes about the game. |

### Notes

- `played_on` is stored as an ISO 8601 date string for sortability.
- `deck_id` references the specific deck variant the player used.
- `opponent_deck_id` references the opponent's main deck (empty-variant row or lowest-id row for that name).
- `opponent` is a free-text name (not a foreign key). Existing databases created before this column get it via `ALTER TABLE ... ADD COLUMN` on open.
- `game_type_id` references the game type (see GameTypes table).
- The composite index `idx_games_format_date` speeds up the default descending date+id query.
- Foreign key enforcement is enabled via `PRAGMA foreign_keys = ON` on every database open.

## Relations

- `Formats.game_id` → `GameTitles.id`
- `DeckArchetypes.game_id` → `GameTitles.id`
- `GameTypes.game_id` → `GameTitles.id`
- `Decks.format_id` → `Formats.id`
- `Decks.archetype_id` → `DeckArchetypes.id`
- `Games.format_id` → `Formats.id`
- `Games.deck_id` → `Decks.id`
- `Games.opponent_deck_id` → `Decks.id`
- `Games.game_type_id` → `GameTypes.id`
