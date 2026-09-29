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
