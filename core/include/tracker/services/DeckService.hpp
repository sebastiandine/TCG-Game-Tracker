#pragma once

// DeckService: high-level operations on decks and archetypes.
// Validates input (trim, reject empty name) and delegates to IDeckRepository.

#include "tracker/domain/Deck.hpp"
#include "tracker/domain/DeckArchetype.hpp"
#include "tracker/ports/IDeckRepository.hpp"
#include "tracker/util/Result.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tracker {

class DeckService {
public:
    explicit DeckService(IDeckRepository& repo);

    // Create an archetype after trimming whitespace and rejecting empty names.
    [[nodiscard]] Result<DeckArchetype> createArchetype(
        std::int64_t gameId, const std::string& name);

    // Return archetypes for `gameId` ordered by id ascending.
    [[nodiscard]] Result<std::vector<DeckArchetype>> listByGame(
        std::int64_t gameId);

    // Create a deck after trimming name/variant/note and validating inputs.
    [[nodiscard]] Result<Deck> create(std::int64_t formatId,
                                      std::int64_t archetypeId,
                                      const std::string& name,
                                      const std::string& variant,
                                      const std::string& variantNote);

    // Update an existing deck after trimming and validating inputs.
    [[nodiscard]] Result<Deck> update(std::int64_t deckId,
                                      std::int64_t archetypeId,
                                      const std::string& name,
                                      const std::string& variant,
                                      const std::string& variantNote);

    // Return all decks belonging to the given format.
    [[nodiscard]] Result<std::vector<Deck>> listByFormat(
        std::int64_t formatId);

private:
    IDeckRepository& repo_;
};

}  // namespace tracker
