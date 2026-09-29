#include "tracker/infra/SqliteDatabase.hpp"

#include <sqlite3.h>

#include <cstring>
#include <string>

namespace tracker {

namespace {

bool columnExists(sqlite3* db, const char* table, const char* column) {
    const std::string sql = std::string("PRAGMA table_info(") + table + ");";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (name != nullptr && std::strcmp(name, column) == 0) {
            found = true;
            break;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

bool tableExists(sqlite3* db, const char* table) {
    const char* sql =
        "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, table, -1, SQLITE_TRANSIENT);
    const bool found = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return found;
}

Result<void> execSql(sqlite3* db, const char* sql, const char* prefix) {
    char* errMsg = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        return Result<void>::err(std::string(prefix) + msg);
    }
    return Result<void>::ok();
}

// Existing databases stored Formats/GameTypes with a global UNIQUE(name).
// SQLite cannot rewrite that constraint in place, so copy into new tables
// scoped to a seeded "Magic: The Gathering" title, preserving row ids.
Result<void> attachLegacyRowsToMagic(sqlite3* db) {
    auto fkOff = execSql(db, "PRAGMA foreign_keys = OFF;",
                         "Failed to disable foreign keys: ");
    if (!fkOff) return fkOff;

    auto begin = execSql(db, "BEGIN;", "Database migration failed: ");
    if (!begin) {
        execSql(db, "PRAGMA foreign_keys = ON;", "");
        return begin;
    }

    auto insertTitle = execSql(
        db,
        "INSERT INTO GameTitles (name) VALUES ('Magic: The Gathering');",
        "Database migration failed: ");
    if (!insertTitle) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        execSql(db, "PRAGMA foreign_keys = ON;", "");
        return insertTitle;
    }
    const auto magicId = sqlite3_last_insert_rowid(db);
    const std::string magicIdSql = std::to_string(magicId);

    const std::string rebuild =
        "CREATE TABLE Formats_new ("
        "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  game_id INTEGER NOT NULL,"
        "  name    TEXT    NOT NULL COLLATE NOCASE,"
        "  FOREIGN KEY (game_id) REFERENCES GameTitles(id),"
        "  UNIQUE (game_id, name)"
        ");"
        "INSERT INTO Formats_new (id, game_id, name) "
        "SELECT id, " + magicIdSql + ", name FROM Formats;"
        "DROP TABLE Formats;"
        "ALTER TABLE Formats_new RENAME TO Formats;"

        "CREATE TABLE GameTypes_new ("
        "  id              INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  game_id         INTEGER NOT NULL,"
        "  name            TEXT    NOT NULL COLLATE NOCASE,"
        "  competitiveness TEXT    NOT NULL,"
        "  medium          TEXT    NOT NULL,"
        "  FOREIGN KEY (game_id) REFERENCES GameTitles(id),"
        "  UNIQUE (game_id, name)"
        ");"
        "INSERT INTO GameTypes_new (id, game_id, name, competitiveness, medium) "
        "SELECT id, " + magicIdSql + ", name, competitiveness, medium FROM GameTypes;"
        "DROP TABLE GameTypes;"
        "ALTER TABLE GameTypes_new RENAME TO GameTypes;"

        "CREATE TABLE DeckArchetypes_new ("
        "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  game_id INTEGER NOT NULL,"
        "  name    TEXT    NOT NULL COLLATE NOCASE,"
        "  FOREIGN KEY (game_id) REFERENCES GameTitles(id),"
        "  UNIQUE (game_id, name)"
        ");"
        "INSERT INTO DeckArchetypes_new (id, game_id, name) "
        "SELECT id, " + magicIdSql + ", name FROM DeckArchetypes;"
        "DROP TABLE DeckArchetypes;"
        "ALTER TABLE DeckArchetypes_new RENAME TO DeckArchetypes;";

    auto copied = execSql(db, rebuild.c_str(), "Database migration failed: ");
    if (!copied) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        execSql(db, "PRAGMA foreign_keys = ON;", "");
        return copied;
    }

    auto commit = execSql(db, "COMMIT;", "Database migration failed: ");
    if (!commit) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        execSql(db, "PRAGMA foreign_keys = ON;", "");
        return commit;
    }

    return execSql(db, "PRAGMA foreign_keys = ON;",
                   "Failed to enable foreign keys: ");
}

}  // namespace

SqliteDatabase::~SqliteDatabase() {
    close();
}

void SqliteDatabase::close() noexcept {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

Result<void> SqliteDatabase::open(const std::filesystem::path& dbPath) {
    close();
    const std::string pathStr = dbPath.generic_string();
    const int rc = sqlite3_open(pathStr.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "unknown error";
        close();
        return Result<void>::err("Failed to open database: " + msg);
    }
    return migrate();
}

Result<void> SqliteDatabase::openMemory() {
    close();
    const int rc = sqlite3_open(":memory:", &db_);
    if (rc != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "unknown error";
        close();
        return Result<void>::err("Failed to open in-memory database: " + msg);
    }
    return migrate();
}

Result<void> SqliteDatabase::reopen(const std::filesystem::path& dbPath) {
    return open(dbPath);
}

Result<void> SqliteDatabase::migrate() {
    auto fkOn = execSql(db_, "PRAGMA foreign_keys = ON;",
                        "Failed to enable foreign keys: ");
    if (!fkOn) return fkOn;

    const char* sql =
        "CREATE TABLE IF NOT EXISTS GameTitles ("
        "  id   INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT    NOT NULL UNIQUE COLLATE NOCASE"
        ");"

        "CREATE TABLE IF NOT EXISTS Formats ("
        "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  game_id INTEGER NOT NULL,"
        "  name    TEXT    NOT NULL COLLATE NOCASE,"
        "  FOREIGN KEY (game_id) REFERENCES GameTitles(id),"
        "  UNIQUE (game_id, name)"
        ");"

        "CREATE TABLE IF NOT EXISTS DeckArchetypes ("
        "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  game_id INTEGER NOT NULL,"
        "  name    TEXT    NOT NULL COLLATE NOCASE,"
        "  FOREIGN KEY (game_id) REFERENCES GameTitles(id),"
        "  UNIQUE (game_id, name)"
        ");"

        "CREATE TABLE IF NOT EXISTS GameTypes ("
        "  id              INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  game_id         INTEGER NOT NULL,"
        "  name            TEXT    NOT NULL COLLATE NOCASE,"
        "  competitiveness TEXT    NOT NULL,"
        "  medium          TEXT    NOT NULL,"
        "  FOREIGN KEY (game_id) REFERENCES GameTitles(id),"
        "  UNIQUE (game_id, name)"
        ");"

        "CREATE TABLE IF NOT EXISTS Decks ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  format_id     INTEGER NOT NULL,"
        "  archetype_id  INTEGER NOT NULL,"
        "  name          TEXT    NOT NULL COLLATE NOCASE,"
        "  variant       TEXT    NOT NULL DEFAULT '' COLLATE NOCASE,"
        "  variant_note  TEXT    NOT NULL DEFAULT '',"
        "  FOREIGN KEY (format_id)    REFERENCES Formats(id),"
        "  FOREIGN KEY (archetype_id) REFERENCES DeckArchetypes(id),"
        "  UNIQUE (format_id, name, variant)"
        ");"

        "CREATE TABLE IF NOT EXISTS Games ("
        "  id               INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  format_id        INTEGER NOT NULL,"
        "  played_on        TEXT    NOT NULL,"
        "  deck_id          INTEGER NOT NULL,"
        "  opponent_deck_id INTEGER NOT NULL,"
        "  opponent         TEXT    NOT NULL DEFAULT '',"
        "  result           TEXT    NOT NULL,"
        "  score            TEXT    NOT NULL,"
        "  game_type_id     INTEGER NOT NULL,"
        "  notes            TEXT    NOT NULL DEFAULT '',"
        "  FOREIGN KEY (format_id)        REFERENCES Formats(id),"
        "  FOREIGN KEY (deck_id)          REFERENCES Decks(id),"
        "  FOREIGN KEY (opponent_deck_id) REFERENCES Decks(id),"
        "  FOREIGN KEY (game_type_id)     REFERENCES GameTypes(id)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_games_format_date"
        "  ON Games (format_id, played_on DESC, id DESC);";

    auto created = execSql(db_, sql, "Database migration failed: ");
    if (!created) return created;

    // Existing databases created before this column still have a Games table
    // without it; CREATE TABLE IF NOT EXISTS will not alter them.
    if (tableExists(db_, "Games") && !columnExists(db_, "Games", "opponent")) {
        auto altered = execSql(
            db_,
            "ALTER TABLE Games ADD COLUMN opponent TEXT NOT NULL DEFAULT '';",
            "Database migration failed: ");
        if (!altered) return altered;
    }

    // Legacy Formats/GameTypes/DeckArchetypes lack game_id.
    if (tableExists(db_, "Formats") && !columnExists(db_, "Formats", "game_id")) {
        return attachLegacyRowsToMagic(db_);
    }

    // Incomplete mid-migration DBs may already have Formats.game_id but still
    // have a global DeckArchetypes table. Rebuild it empty when no titles
    // exist; otherwise attach existing rows to the first title.
    if (tableExists(db_, "DeckArchetypes") &&
        !columnExists(db_, "DeckArchetypes", "game_id")) {
        auto fkOff = execSql(db_, "PRAGMA foreign_keys = OFF;",
                             "Failed to disable foreign keys: ");
        if (!fkOff) return fkOff;

        sqlite3_stmt* stmt = nullptr;
        std::int64_t titleId = 0;
        if (sqlite3_prepare_v2(
                db_, "SELECT id FROM GameTitles ORDER BY id ASC LIMIT 1;",
                -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                titleId = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }

        std::string rebuild;
        if (titleId > 0) {
            const std::string idSql = std::to_string(titleId);
            rebuild =
                "CREATE TABLE DeckArchetypes_new ("
                "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
                "  game_id INTEGER NOT NULL,"
                "  name    TEXT    NOT NULL COLLATE NOCASE,"
                "  FOREIGN KEY (game_id) REFERENCES GameTitles(id),"
                "  UNIQUE (game_id, name)"
                ");"
                "INSERT INTO DeckArchetypes_new (id, game_id, name) "
                "SELECT id, " + idSql + ", name FROM DeckArchetypes;"
                "DROP TABLE DeckArchetypes;"
                "ALTER TABLE DeckArchetypes_new RENAME TO DeckArchetypes;";
        } else {
            rebuild =
                "DROP TABLE DeckArchetypes;"
                "CREATE TABLE DeckArchetypes ("
                "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
                "  game_id INTEGER NOT NULL,"
                "  name    TEXT    NOT NULL COLLATE NOCASE,"
                "  FOREIGN KEY (game_id) REFERENCES GameTitles(id),"
                "  UNIQUE (game_id, name)"
                ");";
        }

        auto copied = execSql(db_, rebuild.c_str(),
                              "Database migration failed: ");
        auto fkOn2 = execSql(db_, "PRAGMA foreign_keys = ON;",
                             "Failed to enable foreign keys: ");
        if (!copied) return copied;
        if (!fkOn2) return fkOn2;
    }

    return Result<void>::ok();
}

}  // namespace tracker
