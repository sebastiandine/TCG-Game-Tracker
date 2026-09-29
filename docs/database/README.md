# Database

TCG Game Tracker uses an embedded SQLite database for persistent data storage.

## Approach

SQLite is integrated via the **amalgamation** method: the official `sqlite3.c` and `sqlite3.h` files are fetched at configure time by CMake `FetchContent` and compiled directly into `tracker_core` as a static library. No system SQLite installation is required.

- **Version**: SQLite 3.53.4 (amalgamation)
- **License**: Public domain / blessing (compatible with MIT)

## Database File Location

The database file is named `tracker.db` and lives under the configured data directory:

```
{Configuration.dataStorage}/tracker.db
```

By default this resolves to `<exe directory>/data/tracker.db`. The data directory is user-configurable through Settings.

## Schema Migrations

All schema migrations run on database open via `SqliteDatabase::migrate()`. Tables use `CREATE TABLE IF NOT EXISTS` so migrations are idempotent and safe to re-run.

See [schema.md](schema.md) for the current table definitions.

## Adding a New Table

1. Add the `CREATE TABLE IF NOT EXISTS` statement to `SqliteDatabase::migrate()` in `core/src/infra/SqliteDatabase.cpp`.
2. Create the corresponding domain type under `core/include/tracker/domain/`.
3. Create a repository port under `core/include/tracker/ports/` and a SQLite adapter under `core/include/tracker/infra/` + `core/src/infra/`.
4. Create a service under `core/include/tracker/services/`.
5. Register new `.cpp` files in `core/CMakeLists.txt`.
6. Add tests using SQLite `:memory:` databases.
7. **Update `docs/database/schema.md`** with the new table definition.
