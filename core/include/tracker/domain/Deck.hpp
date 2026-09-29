#pragma once

// Deck: a named deck definition scoped to a format, with archetype and
// optional variant. Persisted in the Decks SQLite table; no JSON serde needed.

#include <cstdint>
#include <string>

namespace tracker {

struct Deck {
    std::int64_t id{0};
    std::int64_t formatId{0};
    std::int64_t archetypeId{0};
    std::string  name;
    std::string  variant;
    std::string  variantNote;

    friend bool operator==(const Deck&, const Deck&) = default;
};

}  // namespace tracker
