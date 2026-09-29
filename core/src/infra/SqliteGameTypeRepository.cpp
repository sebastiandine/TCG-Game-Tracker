#include "tracker/infra/SqliteGameTypeRepository.hpp"
#include "tracker/infra/SqliteDatabase.hpp"

#include <sqlite3.h>

namespace tracker {

SqliteGameTypeRepository::SqliteGameTypeRepository(SqliteDatabase& db)
    : db_(db) {}

Result<GameType> SqliteGameTypeRepository::create(
        std::int64_t gameId,
        const std::string& name,
        Competitiveness competitiveness,
        PlayMedium medium) {
    const char* sql =
        "INSERT INTO GameTypes (game_id, name, competitiveness, medium) "
        "VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<GameType>::err(
            std::string("Failed to prepare insert: ") + sqlite3_errmsg(db_.handle()));
    }

    const std::string compStr(to_string(competitiveness));
    const std::string medStr(to_string(medium));

    sqlite3_bind_int64(stmt, 1, gameId);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, compStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, medStr.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        const std::string errMsg = sqlite3_errmsg(db_.handle());
        if (errMsg.find("UNIQUE constraint") != std::string::npos) {
            return Result<GameType>::err(
                "A game type named \"" + name + "\" already exists.");
        }
        if (errMsg.find("FOREIGN KEY constraint") != std::string::npos) {
            return Result<GameType>::err("Invalid game reference.");
        }
        return Result<GameType>::err("Failed to create game type: " + errMsg);
    }

    GameType gt;
    gt.id = sqlite3_last_insert_rowid(db_.handle());
    gt.gameId = gameId;
    gt.name = name;
    gt.competitiveness = competitiveness;
    gt.medium = medium;
    return Result<GameType>::ok(std::move(gt));
}

Result<std::vector<GameType>> SqliteGameTypeRepository::listByGame(
        std::int64_t gameId) {
    const char* sql =
        "SELECT id, game_id, name, competitiveness, medium FROM GameTypes "
        "WHERE game_id = ? ORDER BY id ASC;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<std::vector<GameType>>::err(
            std::string("Failed to prepare query: ") + sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, gameId);

    std::vector<GameType> gameTypes;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        GameType gt;
        gt.id = sqlite3_column_int64(stmt, 0);
        gt.gameId = sqlite3_column_int64(stmt, 1);

        const auto* nameText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        gt.name = nameText ? nameText : "";

        const auto* compText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const std::string compStr = compText ? compText : "";
        auto comp = competitivenessFromString(compStr);
        if (!comp) {
            sqlite3_finalize(stmt);
            return Result<std::vector<GameType>>::err(
                "Unknown competitiveness value in database: " + compStr);
        }
        gt.competitiveness = *comp;

        const auto* medText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const std::string medStr = medText ? medText : "";
        auto med = playMediumFromString(medStr);
        if (!med) {
            sqlite3_finalize(stmt);
            return Result<std::vector<GameType>>::err(
                "Unknown medium value in database: " + medStr);
        }
        gt.medium = *med;

        gameTypes.push_back(std::move(gt));
    }
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Result<std::vector<GameType>>::err(
            std::string("Failed to list game types: ") + sqlite3_errmsg(db_.handle()));
    }

    return Result<std::vector<GameType>>::ok(std::move(gameTypes));
}

}  // namespace tracker
