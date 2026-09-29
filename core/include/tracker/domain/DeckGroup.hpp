#pragma once

// DeckGroup: UI-friendly grouping of Deck records that share the same name
// within a format. Produced by groupDecksByName().

#include "tracker/domain/Deck.hpp"

#include <string>
#include <vector>

namespace tracker {

struct DeckGroup {
    std::string        name;
    std::vector<Deck>  variants;

    friend bool operator==(const DeckGroup&, const DeckGroup&) = default;
};

// Group a flat list of decks by name (case-insensitive), sorted by name
// then by variant within each group. Input order does not matter.
std::vector<DeckGroup> groupDecksByName(const std::vector<Deck>& decks);

}  // namespace tracker
