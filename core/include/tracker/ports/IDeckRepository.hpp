#pragma once

// IDeckRepository: persistence port for Deck and DeckArchetype entities.

#include "tracker/domain/Deck.hpp"
#include "tracker/domain/DeckArchetype.hpp"
#include "tracker/util/Result.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tracker {

class IDeckRepository {
public:
    virtual ~IDeckRepository() = default;

    // Create an archetype under `gameId`. Fails if the name already exists
    // in that game (case-insensitive).
    [[nodiscard]] virtual Result<DeckArchetype> createArchetype(
        std::int64_t gameId, const std::string& name) = 0;

    // Return archetypes for `gameId` ordered by id ascending.
    [[nodiscard]] virtual Result<std::vector<DeckArchetype>> listByGame(
        std::int64_t gameId) = 0;

    // Create a deck record. Returns the created Deck (with its assigned id).
    // Fails if the (format_id, name, variant) triple already exists, or if a
    // foreign key is invalid.
    [[nodiscard]] virtual Result<Deck> create(std::int64_t formatId,
                                              std::int64_t archetypeId,
                                              const std::string& name,
                                              const std::string& variant,
                                              const std::string& variantNote) = 0;

    // Update an existing deck record. Returns the updated Deck.
    // Fails if the (format_id, name, variant) triple conflicts with another
    // record, or if a foreign key is invalid.
    [[nodiscard]] virtual Result<Deck> update(std::int64_t deckId,
                                              std::int64_t archetypeId,
                                              const std::string& name,
                                              const std::string& variant,
                                              const std::string& variantNote) = 0;

    // Return all decks belonging to the given format, ordered by name then
    // variant ascending.
    [[nodiscard]] virtual Result<std::vector<Deck>> listByFormat(
        std::int64_t formatId) = 0;
};

}  // namespace tracker
