#pragma once

// SqliteFormatRepository: IFormatRepository backed by a SqliteDatabase.

#include "tracker/ports/IFormatRepository.hpp"

namespace tracker {

class SqliteDatabase;

class SqliteFormatRepository final : public IFormatRepository {
public:
    explicit SqliteFormatRepository(SqliteDatabase& db);

    [[nodiscard]] Result<Format> create(
        std::int64_t gameId, const std::string& name) override;
    [[nodiscard]] Result<std::vector<Format>> listByGame(
        std::int64_t gameId) override;

private:
    SqliteDatabase& db_;
};

}  // namespace tracker
