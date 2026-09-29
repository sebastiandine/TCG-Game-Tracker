#pragma once

// SqliteDatabase: thin RAII wrapper around a sqlite3* handle.
// Owns the connection, runs migrations on open.

#include "tracker/ports/IDatabaseSession.hpp"
#include "tracker/util/Result.hpp"

#include <filesystem>
#include <string>

struct sqlite3;  // forward-declare; sqlite3.h is included only in the .cpp

namespace tracker {

class SqliteDatabase final : public IDatabaseSession {
public:
    SqliteDatabase() = default;
    ~SqliteDatabase() override;

    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;

    // Open (or create) a database file at the given path.
    [[nodiscard]] Result<void> open(const std::filesystem::path& dbPath) override;

    // Open an in-memory database (useful for tests).
    [[nodiscard]] Result<void> openMemory();

    // Close the current connection and open a new one at `dbPath`.
    [[nodiscard]] Result<void> reopen(const std::filesystem::path& dbPath);

    // Apply pending schema migrations. Called automatically on open; tests
    // may call it again after installing a legacy schema on the same handle.
    [[nodiscard]] Result<void> migrate();

    // Raw handle for repository adapters. Never null after a successful open.
    [[nodiscard]] sqlite3* handle() const noexcept { return db_; }

private:
    void close() noexcept;

    sqlite3* db_{nullptr};
};

}  // namespace tracker
