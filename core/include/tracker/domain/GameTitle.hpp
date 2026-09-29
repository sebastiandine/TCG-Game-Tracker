#pragma once

// GameTitle: a user-defined game (e.g. "Magic: The Gathering", "Lorcana").
// Formats and game types belong to a title. Distinct from `Game`, which is a
// recorded match. Persisted in the GameTitles SQLite table; no JSON serde.

#include <cstdint>
#include <string>

namespace tracker {

struct GameTitle {
    std::int64_t id{0};
    std::string  name;

    friend bool operator==(const GameTitle&, const GameTitle&) = default;
};

}  // namespace tracker
