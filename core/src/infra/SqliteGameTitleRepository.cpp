#include "tracker/infra/SqliteGameTitleRepository.hpp"
#include "tracker/infra/SqliteDatabase.hpp"

#include <sqlite3.h>

namespace tracker {

SqliteGameTitleRepository::SqliteGameTitleRepository(SqliteDatabase& db)
    : db_(db) {}

Result<GameTitle> SqliteGameTitleRepository::create(const std::string& name) {
    const char* sql = "INSERT INTO GameTitles (name) VALUES (?);";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<GameTitle>::err(
            std::string("Failed to prepare insert: ") + sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        const std::string errMsg = sqlite3_errmsg(db_.handle());
        if (errMsg.find("UNIQUE constraint") != std::string::npos) {
            return Result<GameTitle>::err(
                "A game named \"" + name + "\" already exists.");
        }
        return Result<GameTitle>::err("Failed to create game: " + errMsg);
    }

    GameTitle g;
    g.id = sqlite3_last_insert_rowid(db_.handle());
    g.name = name;
    return Result<GameTitle>::ok(std::move(g));
}

Result<std::vector<GameTitle>> SqliteGameTitleRepository::listAll() {
    const char* sql = "SELECT id, name FROM GameTitles ORDER BY id ASC;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<std::vector<GameTitle>>::err(
            std::string("Failed to prepare query: ") + sqlite3_errmsg(db_.handle()));
    }

    std::vector<GameTitle> titles;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        GameTitle g;
        g.id = sqlite3_column_int64(stmt, 0);
        const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        g.name = text ? text : "";
        titles.push_back(std::move(g));
    }
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Result<std::vector<GameTitle>>::err(
            std::string("Failed to list games: ") + sqlite3_errmsg(db_.handle()));
    }

    return Result<std::vector<GameTitle>>::ok(std::move(titles));
}

}  // namespace tracker
