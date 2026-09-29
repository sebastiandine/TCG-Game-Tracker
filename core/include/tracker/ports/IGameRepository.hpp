#pragma once

// IGameRepository: persistence port for Game entities.

#include "tracker/domain/Game.hpp"
#include "tracker/util/Result.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tracker {

class IGameRepository {
public:
    virtual ~IGameRepository() = default;

    // Create a game record. Returns the created Game (with its assigned id).
    [[nodiscard]] virtual Result<Game> create(std::int64_t formatId,
                                              const std::string& playedOn,
                                              std::int64_t deckId,
                                              std::int64_t opponentDeckId,
                                              const std::string& opponent,
                                              MatchResult result,
                                              MatchScore score,
                                              std::int64_t gameTypeId,
                                              const std::string& notes) = 0;

    // Update an existing game record. Returns the updated Game.
    [[nodiscard]] virtual Result<Game> update(std::int64_t gameId,
                                              const std::string& playedOn,
                                              std::int64_t deckId,
                                              std::int64_t opponentDeckId,
                                              const std::string& opponent,
                                              MatchResult result,
                                              MatchScore score,
                                              std::int64_t gameTypeId,
                                              const std::string& notes) = 0;

    // Return all games belonging to the given format, ordered by played_on
    // descending, then id descending (newest first).
    [[nodiscard]] virtual Result<std::vector<Game>> listByFormat(
        std::int64_t formatId) = 0;
};

}  // namespace tracker
