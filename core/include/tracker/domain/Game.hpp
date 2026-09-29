#pragma once

// Game: a recorded match result scoped to a format. Persisted in the Games
// SQLite table; no JSON serde needed.

#include "tracker/domain/Enums.hpp"

#include <cstdint>
#include <string>

namespace tracker {

struct Game {
    std::int64_t id{0};
    std::int64_t formatId{0};
    std::string  playedOn;          // YYYY-MM-DD
    std::int64_t deckId{0};         // player's deck (variant row)
    std::int64_t opponentDeckId{0}; // opponent deck (main deck row)
    std::string  opponent;          // free-text opponent name
    MatchResult  result{MatchResult::Win};
    MatchScore   score{MatchScore::TwoOne};
    std::int64_t gameTypeId{0};     // FK to GameTypes
    std::string  notes;

    friend bool operator==(const Game&, const Game&) = default;
};

}  // namespace tracker
