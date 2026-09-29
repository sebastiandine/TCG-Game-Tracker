#pragma once

// SqliteGameRepository: IGameRepository backed by a SqliteDatabase.

#include "tracker/ports/IGameRepository.hpp"

namespace tracker {

class SqliteDatabase;

class SqliteGameRepository final : public IGameRepository {
public:
    explicit SqliteGameRepository(SqliteDatabase& db);

    [[nodiscard]] Result<Game> create(std::int64_t formatId,
                                      const std::string& playedOn,
                                      std::int64_t deckId,
                                      std::int64_t opponentDeckId,
                                      const std::string& opponent,
                                      MatchResult result,
                                      MatchScore score,
                                      std::int64_t gameTypeId,
                                      const std::string& notes) override;

    [[nodiscard]] Result<Game> update(std::int64_t gameId,
                                      const std::string& playedOn,
                                      std::int64_t deckId,
                                      std::int64_t opponentDeckId,
                                      const std::string& opponent,
                                      MatchResult result,
                                      MatchScore score,
                                      std::int64_t gameTypeId,
                                      const std::string& notes) override;

    [[nodiscard]] Result<std::vector<Game>> listByFormat(
        std::int64_t formatId) override;

private:
    SqliteDatabase& db_;
};

}  // namespace tracker
