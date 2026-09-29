#pragma once

// IDatabaseSession: persistence port for opening a SQLite database file.
// Concrete adapter is SqliteDatabase.

#include "tracker/util/Result.hpp"

#include <filesystem>

namespace tracker {

class IDatabaseSession {
public:
    virtual ~IDatabaseSession() = default;

    // Open (or create) a database file at the given path and apply migrations.
    [[nodiscard]] virtual Result<void> open(const std::filesystem::path& dbPath) = 0;
};

}  // namespace tracker
