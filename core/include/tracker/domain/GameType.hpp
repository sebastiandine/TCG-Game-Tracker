#pragma once

// GameType: a named event type (competitiveness + play medium) scoped to a
// GameTitle. Persisted in the GameTypes SQLite table; no JSON serde needed.

#include "tracker/domain/Enums.hpp"

#include <cstdint>
#include <string>

namespace tracker {

struct GameType {
    std::int64_t    id{0};
    std::int64_t    gameId{0};
    std::string     name;
    Competitiveness competitiveness{Competitiveness::NonCompetitive};
    PlayMedium      medium{PlayMedium::Paper};

    friend bool operator==(const GameType&, const GameType&) = default;
};

}  // namespace tracker
