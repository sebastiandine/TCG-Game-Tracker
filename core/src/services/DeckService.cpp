#include "tracker/services/DeckService.hpp"

namespace tracker {

DeckService::DeckService(IDeckRepository& repo)
    : repo_(repo) {}

static std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

Result<DeckArchetype> DeckService::createArchetype(std::int64_t gameId,
                                                   const std::string& name) {
    if (gameId <= 0) {
        return Result<DeckArchetype>::err("No game selected.");
    }
    const std::string trimmed = trim(name);
    if (trimmed.empty()) {
        return Result<DeckArchetype>::err("Archetype name must not be empty.");
    }
    return repo_.createArchetype(gameId, trimmed);
}

Result<std::vector<DeckArchetype>> DeckService::listByGame(
        std::int64_t gameId) {
    return repo_.listByGame(gameId);
}

Result<Deck> DeckService::create(std::int64_t formatId,
                                 std::int64_t archetypeId,
                                 const std::string& name,
                                 const std::string& variant,
                                 const std::string& variantNote) {
    if (formatId <= 0) {
        return Result<Deck>::err("No format selected.");
    }
    if (archetypeId <= 0) {
        return Result<Deck>::err("An archetype must be selected.");
    }

    const std::string trimmedName = trim(name);
    if (trimmedName.empty()) {
        return Result<Deck>::err("Deck name must not be empty.");
    }

    const std::string trimmedVariant = trim(variant);
    const std::string trimmedNote = trim(variantNote);

    return repo_.create(formatId, archetypeId, trimmedName, trimmedVariant,
                        trimmedNote);
}

Result<Deck> DeckService::update(std::int64_t deckId,
                                 std::int64_t archetypeId,
                                 const std::string& name,
                                 const std::string& variant,
                                 const std::string& variantNote) {
    if (deckId <= 0) {
        return Result<Deck>::err("No deck selected.");
    }
    if (archetypeId <= 0) {
        return Result<Deck>::err("An archetype must be selected.");
    }

    const std::string trimmedName = trim(name);
    if (trimmedName.empty()) {
        return Result<Deck>::err("Deck name must not be empty.");
    }

    const std::string trimmedVariant = trim(variant);
    const std::string trimmedNote = trim(variantNote);

    return repo_.update(deckId, archetypeId, trimmedName, trimmedVariant,
                        trimmedNote);
}

Result<std::vector<Deck>> DeckService::listByFormat(std::int64_t formatId) {
    return repo_.listByFormat(formatId);
}

}  // namespace tracker
