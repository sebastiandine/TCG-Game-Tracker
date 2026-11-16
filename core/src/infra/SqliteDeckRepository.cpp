#include "tracker/infra/SqliteDeckRepository.hpp"
#include "tracker/infra/SqliteDatabase.hpp"

#include <sqlite3.h>

namespace tracker {

SqliteDeckRepository::SqliteDeckRepository(SqliteDatabase& db)
    : db_(db) {}

Result<DeckArchetype> SqliteDeckRepository::createArchetype(
        std::int64_t gameId, const std::string& name) {
    const char* sql =
        "INSERT INTO DeckArchetypes (game_id, name) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<DeckArchetype>::err(
            std::string("Failed to prepare insert: ") +
            sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, gameId);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        const std::string errMsg = sqlite3_errmsg(db_.handle());
        if (errMsg.find("UNIQUE constraint") != std::string::npos) {
            return Result<DeckArchetype>::err(
                "An archetype named \"" + name + "\" already exists.");
        }
        if (errMsg.find("FOREIGN KEY constraint") != std::string::npos) {
            return Result<DeckArchetype>::err("Invalid game reference.");
        }
        return Result<DeckArchetype>::err(
            "Failed to create archetype: " + errMsg);
    }

    DeckArchetype a;
    a.id = sqlite3_last_insert_rowid(db_.handle());
    a.gameId = gameId;
    a.name = name;
    return Result<DeckArchetype>::ok(std::move(a));
}

Result<std::vector<DeckArchetype>> SqliteDeckRepository::listByGame(
        std::int64_t gameId) {
    const char* sql =
        "SELECT id, game_id, name FROM DeckArchetypes "
        "WHERE game_id = ? ORDER BY id ASC;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<std::vector<DeckArchetype>>::err(
            std::string("Failed to prepare query: ") +
            sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, gameId);

    std::vector<DeckArchetype> archetypes;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        DeckArchetype a;
        a.id = sqlite3_column_int64(stmt, 0);
        a.gameId = sqlite3_column_int64(stmt, 1);
        const auto* text =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        a.name = text ? text : "";
        archetypes.push_back(std::move(a));
    }
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Result<std::vector<DeckArchetype>>::err(
            std::string("Failed to list archetypes: ") +
            sqlite3_errmsg(db_.handle()));
    }

    return Result<std::vector<DeckArchetype>>::ok(std::move(archetypes));
}

Result<Deck> SqliteDeckRepository::create(std::int64_t formatId,
                                          std::int64_t archetypeId,
                                          const std::string& name,
                                          const std::string& variant,
                                          const std::string& variantNote) {
    const char* sql =
        "INSERT INTO Decks (format_id, archetype_id, name, variant, variant_note)"
        " VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<Deck>::err(std::string("Failed to prepare insert: ") +
                                 sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, formatId);
    sqlite3_bind_int64(stmt, 2, archetypeId);
    sqlite3_bind_text(stmt, 3, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, variant.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, variantNote.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        const std::string errMsg = sqlite3_errmsg(db_.handle());
        if (errMsg.find("UNIQUE constraint") != std::string::npos) {
            return Result<Deck>::err(
                "A deck named \"" + name + "\" with variant \"" + variant +
                "\" already exists in this format.");
        }
        if (errMsg.find("FOREIGN KEY constraint") != std::string::npos) {
            return Result<Deck>::err(
                "Invalid format or archetype reference.");
        }
        return Result<Deck>::err("Failed to create deck: " + errMsg);
    }

    Deck d;
    d.id = sqlite3_last_insert_rowid(db_.handle());
    d.formatId = formatId;
    d.archetypeId = archetypeId;
    d.name = name;
    d.variant = variant;
    d.variantNote = variantNote;
    return Result<Deck>::ok(std::move(d));
}

Result<Deck> SqliteDeckRepository::update(std::int64_t deckId,
                                          std::int64_t archetypeId,
                                          const std::string& name,
                                          const std::string& variant,
                                          const std::string& variantNote) {
    const char* sql =
        "UPDATE Decks SET archetype_id = ?, name = ?, variant = ?,"
        " variant_note = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<Deck>::err(std::string("Failed to prepare update: ") +
                                 sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, archetypeId);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, variant.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, variantNote.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, deckId);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        const std::string errMsg = sqlite3_errmsg(db_.handle());
        if (errMsg.find("UNIQUE constraint") != std::string::npos) {
            return Result<Deck>::err(
                "A deck named \"" + name + "\" with variant \"" + variant +
                "\" already exists in this format.");
        }
        if (errMsg.find("FOREIGN KEY constraint") != std::string::npos) {
            return Result<Deck>::err(
                "Invalid archetype reference.");
        }
        return Result<Deck>::err("Failed to update deck: " + errMsg);
    }

    if (sqlite3_changes(db_.handle()) == 0) {
        return Result<Deck>::err("Deck not found.");
    }

    // Read back the full row to get format_id.
    const char* selectSql =
        "SELECT id, format_id, archetype_id, name, variant, variant_note"
        " FROM Decks WHERE id = ?;";
    sqlite3_stmt* selStmt = nullptr;
    rc = sqlite3_prepare_v2(db_.handle(), selectSql, -1, &selStmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<Deck>::err(std::string("Failed to read back deck: ") +
                                 sqlite3_errmsg(db_.handle()));
    }
    sqlite3_bind_int64(selStmt, 1, deckId);
    rc = sqlite3_step(selStmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(selStmt);
        return Result<Deck>::err("Deck not found after update.");
    }

    Deck d;
    d.id = sqlite3_column_int64(selStmt, 0);
    d.formatId = sqlite3_column_int64(selStmt, 1);
    d.archetypeId = sqlite3_column_int64(selStmt, 2);
    const auto* nt = reinterpret_cast<const char*>(sqlite3_column_text(selStmt, 3));
    d.name = nt ? nt : "";
    const auto* vt = reinterpret_cast<const char*>(sqlite3_column_text(selStmt, 4));
    d.variant = vt ? vt : "";
    const auto* ntt = reinterpret_cast<const char*>(sqlite3_column_text(selStmt, 5));
    d.variantNote = ntt ? ntt : "";
    sqlite3_finalize(selStmt);

    return Result<Deck>::ok(std::move(d));
}

Result<Deck> SqliteDeckRepository::findById(std::int64_t deckId) {
    const char* sql =
        "SELECT id, format_id, archetype_id, name, variant, variant_note"
        " FROM Decks WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<Deck>::err(std::string("Failed to prepare query: ") +
                                 sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, deckId);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        if (rc == SQLITE_DONE) return Result<Deck>::err("Deck not found.");
        return Result<Deck>::err(std::string("Failed to read deck: ") +
                                 sqlite3_errmsg(db_.handle()));
    }

    Deck d;
    d.id = sqlite3_column_int64(stmt, 0);
    d.formatId = sqlite3_column_int64(stmt, 1);
    d.archetypeId = sqlite3_column_int64(stmt, 2);
    const auto* nameText =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    d.name = nameText ? nameText : "";
    const auto* variantText =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    d.variant = variantText ? variantText : "";
    const auto* noteText =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    d.variantNote = noteText ? noteText : "";
    sqlite3_finalize(stmt);
    return Result<Deck>::ok(std::move(d));
}

Result<void> SqliteDeckRepository::renameFamily(std::int64_t formatId,
                                                const std::string& fromName,
                                                const std::string& toName) {
    const char* conflictSql =
        "SELECT d.variant FROM Decks d "
        "WHERE d.format_id = ?1 AND d.name = ?2 COLLATE NOCASE "
        "AND EXISTS ("
        "  SELECT 1 FROM Decks o "
        "  WHERE o.format_id = d.format_id "
        "    AND o.id != d.id "
        "    AND o.name = ?3 COLLATE NOCASE "
        "    AND o.variant = d.variant "
        "    AND o.name != ?2 COLLATE NOCASE"
        ");";
    sqlite3_stmt* conflictStmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), conflictSql, -1, &conflictStmt,
                                nullptr);
    if (rc != SQLITE_OK) {
        return Result<void>::err(std::string("Failed to prepare rename: ") +
                                 sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(conflictStmt, 1, formatId);
    sqlite3_bind_text(conflictStmt, 2, fromName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(conflictStmt, 3, toName.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(conflictStmt);
    if (rc == SQLITE_ROW) {
        const auto* variantText = reinterpret_cast<const char*>(
            sqlite3_column_text(conflictStmt, 0));
        const std::string variant = variantText ? variantText : "";
        sqlite3_finalize(conflictStmt);
        return Result<void>::err(
            "A deck named \"" + toName + "\" with variant \"" + variant +
            "\" already exists in this format.");
    }
    sqlite3_finalize(conflictStmt);
    if (rc != SQLITE_DONE) {
        return Result<void>::err(std::string("Failed to rename deck: ") +
                                 sqlite3_errmsg(db_.handle()));
    }

    const char* sql =
        "UPDATE Decks SET name = ?1 "
        "WHERE format_id = ?2 AND name = ?3 COLLATE NOCASE;";
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<void>::err(std::string("Failed to prepare rename: ") +
                                 sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_text(stmt, 1, toName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, formatId);
    sqlite3_bind_text(stmt, 3, fromName.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        const std::string errMsg = sqlite3_errmsg(db_.handle());
        if (errMsg.find("UNIQUE constraint") != std::string::npos) {
            return Result<void>::err(
                "A deck named \"" + toName +
                "\" already exists in this format.");
        }
        return Result<void>::err("Failed to rename deck: " + errMsg);
    }
    return Result<void>::ok();
}

Result<int> SqliteDeckRepository::remove(std::int64_t deckId) {
    auto existing = findById(deckId);
    if (!existing) return Result<int>::err(existing.error());

    auto rollback = [this]() {
        sqlite3_exec(db_.handle(), "ROLLBACK;", nullptr, nullptr, nullptr);
    };

    char* errMsg = nullptr;
    if (sqlite3_exec(db_.handle(), "BEGIN;", nullptr, nullptr, &errMsg) !=
        SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "Failed to start delete.";
        sqlite3_free(errMsg);
        return Result<int>::err(std::move(msg));
    }

    const char* gamesSql =
        "DELETE FROM Games WHERE deck_id = ? OR opponent_deck_id = ?;";
    sqlite3_stmt* gamesStmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), gamesSql, -1, &gamesStmt,
                                nullptr);
    if (rc != SQLITE_OK) {
        const std::string msg = std::string("Failed to prepare delete: ") +
                                sqlite3_errmsg(db_.handle());
        rollback();
        return Result<int>::err(msg);
    }
    sqlite3_bind_int64(gamesStmt, 1, deckId);
    sqlite3_bind_int64(gamesStmt, 2, deckId);
    rc = sqlite3_step(gamesStmt);
    sqlite3_finalize(gamesStmt);
    if (rc != SQLITE_DONE) {
        const std::string msg = std::string("Failed to delete games: ") +
                                sqlite3_errmsg(db_.handle());
        rollback();
        return Result<int>::err(msg);
    }
    const int gamesRemoved = sqlite3_changes(db_.handle());

    const char* deckSql = "DELETE FROM Decks WHERE id = ?;";
    sqlite3_stmt* deckStmt = nullptr;
    rc = sqlite3_prepare_v2(db_.handle(), deckSql, -1, &deckStmt, nullptr);
    if (rc != SQLITE_OK) {
        const std::string msg = std::string("Failed to prepare delete: ") +
                                sqlite3_errmsg(db_.handle());
        rollback();
        return Result<int>::err(msg);
    }
    sqlite3_bind_int64(deckStmt, 1, deckId);
    rc = sqlite3_step(deckStmt);
    sqlite3_finalize(deckStmt);
    if (rc != SQLITE_DONE) {
        const std::string msg = std::string("Failed to delete deck: ") +
                                sqlite3_errmsg(db_.handle());
        rollback();
        return Result<int>::err(msg);
    }
    if (sqlite3_changes(db_.handle()) == 0) {
        rollback();
        return Result<int>::err("Deck not found.");
    }

    errMsg = nullptr;
    if (sqlite3_exec(db_.handle(), "COMMIT;", nullptr, nullptr, &errMsg) !=
        SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "Failed to commit delete.";
        sqlite3_free(errMsg);
        rollback();
        return Result<int>::err(std::move(msg));
    }
    return Result<int>::ok(gamesRemoved);
}

Result<std::vector<Deck>> SqliteDeckRepository::listByFormat(
    std::int64_t formatId) {
    const char* sql =
        "SELECT id, format_id, archetype_id, name, variant, variant_note"
        " FROM Decks WHERE format_id = ?"
        " ORDER BY name COLLATE NOCASE ASC, variant COLLATE NOCASE ASC;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<std::vector<Deck>>::err(
            std::string("Failed to prepare query: ") +
            sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, formatId);

    std::vector<Deck> decks;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Deck d;
        d.id = sqlite3_column_int64(stmt, 0);
        d.formatId = sqlite3_column_int64(stmt, 1);
        d.archetypeId = sqlite3_column_int64(stmt, 2);
        const auto* nameText =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        d.name = nameText ? nameText : "";
        const auto* variantText =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        d.variant = variantText ? variantText : "";
        const auto* noteText =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        d.variantNote = noteText ? noteText : "";
        decks.push_back(std::move(d));
    }
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Result<std::vector<Deck>>::err(
            std::string("Failed to list decks: ") +
            sqlite3_errmsg(db_.handle()));
    }

    return Result<std::vector<Deck>>::ok(std::move(decks));
}

}  // namespace tracker
