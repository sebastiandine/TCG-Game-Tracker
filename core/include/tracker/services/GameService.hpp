#pragma once

// GameService: high-level operations on recorded games.
// Validates input (trim notes, reject invalid date/enums) and delegates to
// IGameRepository.

#include "tracker/domain/Game.hpp"
#include "tracker/ports/IGameRepository.hpp"
#include "tracker/util/Result.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tracker {

class GameService {
public:
    explicit GameService(IGameRepository& repo);

    // Create a game after trimming notes and validating inputs.
    [[nodiscard]] Result<Game> create(std::int64_t formatId,
                                      const std::string& playedOn,
                                      std::int64_t deckId,
                                      std::int64_t opponentDeckId,
                                      const std::string& opponent,
                                      MatchResult result,
                                      MatchScore score,
                                      std::int64_t gameTypeId,
                                      const std::string& notes);

    // Update an existing game after trimming notes and validating inputs.
    [[nodiscard]] Result<Game> update(std::int64_t gameId,
                                      const std::string& playedOn,
                                      std::int64_t deckId,
                                      std::int64_t opponentDeckId,
                                      const std::string& opponent,
                                      MatchResult result,
                                      MatchScore score,
                                      std::int64_t gameTypeId,
                                      const std::string& notes);

    // Clear notes on an existing game; all other fields stay unchanged.
    [[nodiscard]] Result<Game> clearNotes(const Game& game);

    // Return all games belonging to the given format (newest first).
    [[nodiscard]] Result<std::vector<Game>> listByFormat(
        std::int64_t formatId);

private:
    IGameRepository& repo_;
};

}  // namespace tracker
