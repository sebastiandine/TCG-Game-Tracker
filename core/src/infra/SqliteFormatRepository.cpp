#include "tracker/infra/SqliteFormatRepository.hpp"
#include "tracker/infra/SqliteDatabase.hpp"

#include <sqlite3.h>

namespace tracker {

SqliteFormatRepository::SqliteFormatRepository(SqliteDatabase& db)
    : db_(db) {}

Result<Format> SqliteFormatRepository::create(
        std::int64_t gameId, const std::string& name) {
    const char* sql = "INSERT INTO Formats (game_id, name) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<Format>::err(
            std::string("Failed to prepare insert: ") + sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, gameId);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        const std::string errMsg = sqlite3_errmsg(db_.handle());
        if (errMsg.find("UNIQUE constraint") != std::string::npos) {
            return Result<Format>::err(
                "A format named \"" + name + "\" already exists.");
        }
        if (errMsg.find("FOREIGN KEY constraint") != std::string::npos) {
            return Result<Format>::err("Invalid game reference.");
        }
        return Result<Format>::err("Failed to create format: " + errMsg);
    }

    Format f;
    f.id = sqlite3_last_insert_rowid(db_.handle());
    f.gameId = gameId;
    f.name = name;
    return Result<Format>::ok(std::move(f));
}

Result<std::vector<Format>> SqliteFormatRepository::listByGame(
        std::int64_t gameId) {
    const char* sql =
        "SELECT id, game_id, name FROM Formats WHERE game_id = ? ORDER BY id ASC;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<std::vector<Format>>::err(
            std::string("Failed to prepare query: ") + sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, gameId);

    std::vector<Format> formats;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Format f;
        f.id = sqlite3_column_int64(stmt, 0);
        f.gameId = sqlite3_column_int64(stmt, 1);
        const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        f.name = text ? text : "";
        formats.push_back(std::move(f));
    }
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Result<std::vector<Format>>::err(
            std::string("Failed to list formats: ") + sqlite3_errmsg(db_.handle()));
    }

    return Result<std::vector<Format>>::ok(std::move(formats));
}

}  // namespace tracker
