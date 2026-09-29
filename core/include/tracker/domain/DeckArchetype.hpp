#pragma once

// DeckArchetype: a named archetype category scoped to a GameTitle
// (e.g. "Aggro", "Control"). Persisted in the DeckArchetypes SQLite table;
// no JSON serde needed.

#include <cstdint>
#include <string>

namespace tracker {

struct DeckArchetype {
    std::int64_t id{0};
    std::int64_t gameId{0};
    std::string  name;

    friend bool operator==(const DeckArchetype&,
                           const DeckArchetype&) = default;
};

}  // namespace tracker
