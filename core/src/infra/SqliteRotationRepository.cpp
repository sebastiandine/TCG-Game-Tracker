#include "tracker/infra/SqliteRotationRepository.hpp"
#include "tracker/infra/SqliteDatabase.hpp"

#include <sqlite3.h>

namespace tracker {

namespace {

const char* safeText(sqlite3_stmt* stmt, int col) {
    const auto* text =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
    return text ? text : "";
}

Rotation readRotation(sqlite3_stmt* stmt) {
    Rotation rotation;
    rotation.id = sqlite3_column_int64(stmt, 0);
    rotation.formatId = sqlite3_column_int64(stmt, 1);
    rotation.name = safeText(stmt, 2);
    rotation.startYear = sqlite3_column_int(stmt, 3);
    rotation.startMonth = sqlite3_column_int(stmt, 4);
    rotation.endYear = sqlite3_column_int(stmt, 5);
    rotation.endMonth = sqlite3_column_int(stmt, 6);
    rotation.expiryPrompted = sqlite3_column_int(stmt, 7) != 0;
    return rotation;
}

}  // namespace

SqliteRotationRepository::SqliteRotationRepository(SqliteDatabase& db)
    : db_(db) {}

Result<Rotation> SqliteRotationRepository::create(std::int64_t formatId,
                                                  const std::string& name,
                                                  int startYear,
                                                  int startMonth,
                                                  int endYear,
                                                  int endMonth) {
    const char* sql =
        "INSERT INTO Rotations (format_id, name, start_year, start_month,"
        " end_year, end_month) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<Rotation>::err(
            std::string("Failed to prepare insert: ") +
            sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, formatId);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, startYear);
    sqlite3_bind_int(stmt, 4, startMonth);
    sqlite3_bind_int(stmt, 5, endYear);
    sqlite3_bind_int(stmt, 6, endMonth);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        const std::string errMsg = sqlite3_errmsg(db_.handle());
        if (errMsg.find("UNIQUE constraint") != std::string::npos) {
            return Result<Rotation>::err(
                "A rotation named \"" + name + "\" already exists.");
        }
        if (errMsg.find("CHECK constraint") != std::string::npos) {
            return Result<Rotation>::err("Rotation end is before the start.");
        }
        if (errMsg.find("FOREIGN KEY constraint") != std::string::npos) {
            return Result<Rotation>::err("Invalid format reference.");
        }
        return Result<Rotation>::err("Failed to create rotation: " + errMsg);
    }

    Rotation rotation;
    rotation.id = sqlite3_last_insert_rowid(db_.handle());
    rotation.formatId = formatId;
    rotation.name = name;
    rotation.startYear = startYear;
    rotation.startMonth = startMonth;
    rotation.endYear = endYear;
    rotation.endMonth = endMonth;
    return Result<Rotation>::ok(std::move(rotation));
}

Result<std::vector<Rotation>> SqliteRotationRepository::listByFormat(
        std::int64_t formatId) {
    const char* sql =
        "SELECT id, format_id, name, start_year, start_month, end_year,"
        " end_month, expiry_prompted FROM Rotations"
        " WHERE format_id = ?"
        " ORDER BY start_year ASC, start_month ASC, id ASC;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<std::vector<Rotation>>::err(
            std::string("Failed to prepare query: ") +
            sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, formatId);

    std::vector<Rotation> rotations;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        rotations.push_back(readRotation(stmt));
    }
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Result<std::vector<Rotation>>::err(
            std::string("Failed to list rotations: ") +
            sqlite3_errmsg(db_.handle()));
    }
    return Result<std::vector<Rotation>>::ok(std::move(rotations));
}

Result<void> SqliteRotationRepository::markExpiryPrompted(
        std::int64_t rotationId) {
    const char* sql =
        "UPDATE Rotations SET expiry_prompted = 1 WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<void>::err(
            std::string("Failed to prepare update: ") +
            sqlite3_errmsg(db_.handle()));
    }

    sqlite3_bind_int64(stmt, 1, rotationId);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Result<void>::err(
            std::string("Failed to update rotation: ") +
            sqlite3_errmsg(db_.handle()));
    }
    if (sqlite3_changes(db_.handle()) == 0) {
        return Result<void>::err("Rotation not found.");
    }
    return Result<void>::ok();
}

}  // namespace tracker
