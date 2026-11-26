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
    // `rotationId` 0 stores a null rotation (non-rotating format).
    [[nodiscard]] virtual Result<Deck> create(std::int64_t formatId,
                                              std::int64_t archetypeId,
                                              const std::string& name,
                                              const std::string& variant,
                                              const std::string& variantNote,
                                              std::int64_t rotationId = 0) = 0;

    // Update an existing deck record. Returns the updated Deck.
    // Fails if the (format_id, name, variant) triple conflicts with another
    // record, or if a foreign key is invalid.
    [[nodiscard]] virtual Result<Deck> update(std::int64_t deckId,
                                              std::int64_t archetypeId,
                                              const std::string& name,
                                              const std::string& variant,
                                              const std::string& variantNote) = 0;

    // Return the deck with this id, or an error if it does not exist.
    [[nodiscard]] virtual Result<Deck> findById(std::int64_t deckId) = 0;

    // Set `toName` on every deck in `formatId` whose name matches `fromName`
    // (case-insensitive). Variant labels are left unchanged. Fails, changing
    // nothing, when the new name would duplicate an existing deck+variant.
    // Rename only the family in this rotation. `rotationId` 0 is the
    // non-rotating scope (rotation_id IS NULL).
    [[nodiscard]] virtual Result<void> renameFamily(
        std::int64_t formatId,
        const std::string& fromName,
        const std::string& toName,
        std::int64_t rotationId = 0) = 0;

    // Delete games that reference `deckId` (as the player deck or the
    // opponent deck), then delete the deck, in one transaction. Returns how
    // many games were removed.
    [[nodiscard]] virtual Result<int> remove(std::int64_t deckId) = 0;

    // Decks in the format. `rotationId` 0 returns only non-rotating rows
    // (rotation_id IS NULL); any other id returns that rotation.
    [[nodiscard]] virtual Result<std::vector<Deck>> listByFormat(
        std::int64_t formatId, std::int64_t rotationId = 0) = 0;
};

}  // namespace tracker
