#include "tracker/infra/SqliteGameRepository.hpp"
#include "tracker/infra/SqliteDatabase.hpp"

#include <sqlite3.h>

#include <string>

namespace tracker {

namespace {

const char* safeText(sqlite3_stmt* stmt, int col) {
    const auto* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
    return t ? t : "";
}

Game rowToGame(sqlite3_stmt* stmt) {
    Game g;
    g.id             = sqlite3_column_int64(stmt, 0);
    g.formatId       = sqlite3_column_int64(stmt, 1);
    g.playedOn       = safeText(stmt, 2);
    g.deckId         = sqlite3_column_int64(stmt, 3);
    g.opponentDeckId = sqlite3_column_int64(stmt, 4);
    g.opponent       = safeText(stmt, 5);
    auto r = matchResultFromString(safeText(stmt, 6));
    g.result = r.value_or(MatchResult::Win);
    auto s = matchScoreFromString(safeText(stmt, 7));
    g.score = s.value_or(MatchScore::TwoOne);
    g.gameTypeId     = sqlite3_column_int64(stmt, 8);
    g.notes          = safeText(stmt, 9);
    return g;
}

}  // namespace

SqliteGameRepository::SqliteGameRepository(SqliteDatabase& db)
    : db_(db) {}

Result<Game> SqliteGameRepository::create(std::int64_t formatId,
                                          const std::string& playedOn,
                                          std::int64_t deckId,
                                          std::int64_t opponentDeckId,
                                          const std::string& opponent,
                                          MatchResult result,
                                          MatchScore score,
                                          std::int64_t gameTypeId,
                                          const std::string& notes) {
    const char* sql =
        "INSERT INTO Games (format_id, played_on, deck_id, opponent_deck_id,"
        " opponent, result, score, game_type_id, notes)"
        " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<Game>::err(std::string("Failed to prepare insert: ") +
                                 sqlite3_errmsg(db_.handle()));
    }

    const std::string resultStr(to_string(result));
    const std::string scoreStr(to_string(score));

    sqlite3_bind_int64(stmt, 1, formatId);
    sqlite3_bind_text(stmt, 2, playedOn.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, deckId);
    sqlite3_bind_int64(stmt, 4, opponentDeckId);
    sqlite3_bind_text(stmt, 5, opponent.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, resultStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, scoreStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 8, gameTypeId);
    sqlite3_bind_text(stmt, 9, notes.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        const std::string errMsg = sqlite3_errmsg(db_.handle());
        if (errMsg.find("FOREIGN KEY constraint") != std::string::npos) {
            return Result<Game>::err(
                "Invalid format, deck, or game type reference.");
        }
        return Result<Game>::err("Failed to create game: " + errMsg);
    }

    Game g;
    g.id = sqlite3_last_insert_rowid(db_.handle());
    g.formatId = formatId;
    g.playedOn = playedOn;
    g.deckId = deckId;
    g.opponentDeckId = opponentDeckId;
    g.opponent = opponent;
    g.result = result;
    g.score = score;
    g.gameTypeId = gameTypeId;
    g.notes = notes;
    return Result<Game>::ok(std::move(g));
}

Result<Game> SqliteGameRepository::update(std::int64_t gameId,
                                          const std::string& playedOn,
                                          std::int64_t deckId,
                                          std::int64_t opponentDeckId,
                                          const std::string& opponent,
                                          MatchResult result,
                                          MatchScore score,
                                          std::int64_t gameTypeId,
                                          const std::string& notes) {
    const char* sql =
        "UPDATE Games SET played_on = ?, deck_id = ?, opponent_deck_id = ?,"
        " opponent = ?, result = ?, score = ?, game_type_id = ?, notes = ?"
        " WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<Game>::err(std::string("Failed to prepare update: ") +
                                 sqlite3_errmsg(db_.handle()));
    }

    const std::string resultStr(to_string(result));
    const std::string scoreStr(to_string(score));

    sqlite3_bind_text(stmt, 1, playedOn.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, deckId);
    sqlite3_bind_int64(stmt, 3, opponentDeckId);
    sqlite3_bind_text(stmt, 4, opponent.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, resultStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, scoreStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 7, gameTypeId);
    sqlite3_bind_text(stmt, 8, notes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 9, gameId);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        const std::string errMsg = sqlite3_errmsg(db_.handle());
        if (errMsg.find("FOREIGN KEY constraint") != std::string::npos) {
            return Result<Game>::err("Invalid deck or game type reference.");
        }
        return Result<Game>::err("Failed to update game: " + errMsg);
    }

    if (sqlite3_changes(db_.handle()) == 0) {
        return Result<Game>::err("Game not found.");
    }

    // Read back the full row.
    const char* selectSql =
        "SELECT id, format_id, played_on, deck_id, opponent_deck_id, opponent,"
        " result, score, game_type_id, notes FROM Games WHERE id = ?;";
    sqlite3_stmt* selStmt = nullptr;
    rc = sqlite3_prepare_v2(db_.handle(), selectSql, -1, &selStmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<Game>::err(std::string("Failed to read back game: ") +
                                 sqlite3_errmsg(db_.handle()));
    }
    sqlite3_bind_int64(selStmt, 1, gameId);
    rc = sqlite3_step(selStmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(selStmt);
        return Result<Game>::err("Game not found after update.");
    }

    Game g = rowToGame(selStmt);
    sqlite3_finalize(selStmt);
    return Result<Game>::ok(std::move(g));
}

Result<std::vector<Game>> SqliteGameRepository::listByFormat(
    std::int64_t formatId) {
    const char* sql =
        "SELECT id, format_id, played_on, deck_id, opponent_deck_id, opponent,"
        " result, score, game_type_id, notes FROM Games"
        " WHERE format_id = ?"
        " ORDER BY played_on DESC, id DESC;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<std::vector<Game>>::err(
            std::string("Failed to prepare query: ") +
            sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, formatId);

    std::vector<Game> games;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        games.push_back(rowToGame(stmt));
    }
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Result<std::vector<Game>>::err(
            std::string("Failed to list games: ") +
            sqlite3_errmsg(db_.handle()));
    }

    return Result<std::vector<Game>>::ok(std::move(games));
}

}  // namespace tracker
