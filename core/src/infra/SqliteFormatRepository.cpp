#include "tracker/infra/SqliteFormatRepository.hpp"
#include "tracker/infra/SqliteDatabase.hpp"

#include <sqlite3.h>

namespace tracker {

SqliteFormatRepository::SqliteFormatRepository(SqliteDatabase& db)
    : db_(db) {}

namespace {

Format readFormat(sqlite3_stmt* stmt) {
    Format format;
    format.id = sqlite3_column_int64(stmt, 0);
    format.gameId = sqlite3_column_int64(stmt, 1);
    const auto* text =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    format.name = text ? text : "";
    format.rotating = sqlite3_column_int(stmt, 3) != 0;
    return format;
}

}  // namespace

Result<Format> SqliteFormatRepository::create(
        std::int64_t gameId, const std::string& name, bool rotating) {
    const char* sql =
        "INSERT INTO Formats (game_id, name, is_rotating) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<Format>::err(
            std::string("Failed to prepare insert: ") + sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, gameId);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, rotating ? 1 : 0);
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
    f.rotating = rotating;
    return Result<Format>::ok(std::move(f));
}

Result<Format> SqliteFormatRepository::findById(std::int64_t formatId) {
    const char* sql =
        "SELECT id, game_id, name, is_rotating FROM Formats WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<Format>::err(
            std::string("Failed to prepare query: ") + sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, formatId);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        if (rc == SQLITE_DONE) return Result<Format>::err("Format not found.");
        return Result<Format>::err(
            std::string("Failed to read format: ") + sqlite3_errmsg(db_.handle()));
    }
    Format format = readFormat(stmt);
    sqlite3_finalize(stmt);
    return Result<Format>::ok(std::move(format));
}

Result<void> SqliteFormatRepository::remove(std::int64_t formatId) {
    const char* sql = "DELETE FROM Formats WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<void>::err(
            std::string("Failed to prepare delete: ") + sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, formatId);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        return Result<void>::err(
            std::string("Failed to delete format: ") + sqlite3_errmsg(db_.handle()));
    }
    if (sqlite3_changes(db_.handle()) == 0) {
        return Result<void>::err("Format not found.");
    }
    return Result<void>::ok();
}

Result<std::vector<Format>> SqliteFormatRepository::listByGame(
        std::int64_t gameId) {
    const char* sql =
        "SELECT id, game_id, name, is_rotating FROM Formats"
        " WHERE game_id = ? ORDER BY id ASC;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<std::vector<Format>>::err(
            std::string("Failed to prepare query: ") + sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, gameId);

    std::vector<Format> formats;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        formats.push_back(readFormat(stmt));
    }
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Result<std::vector<Format>>::err(
            std::string("Failed to list formats: ") + sqlite3_errmsg(db_.handle()));
    }

    return Result<std::vector<Format>>::ok(std::move(formats));
}

}  // namespace tracker
