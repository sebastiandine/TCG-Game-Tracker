#pragma once

// SqliteGameTypeRepository: IGameTypeRepository backed by a SqliteDatabase.

#include "tracker/ports/IGameTypeRepository.hpp"

namespace tracker {

class SqliteDatabase;

class SqliteGameTypeRepository final : public IGameTypeRepository {
public:
    explicit SqliteGameTypeRepository(SqliteDatabase& db);

    [[nodiscard]] Result<GameType> create(
        std::int64_t gameId,
        const std::string& name,
        Competitiveness competitiveness,
        PlayMedium medium) override;
    [[nodiscard]] Result<std::vector<GameType>> listByGame(
        std::int64_t gameId) override;

private:
    SqliteDatabase& db_;
};

}  // namespace tracker
