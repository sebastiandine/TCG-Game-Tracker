#include "tracker/services/DeckService.hpp"

#include <cctype>
#include <vector>

namespace tracker {

DeckService::DeckService(IDeckRepository& repo)
    : repo_(repo) {}

static std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static bool sameText(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// The name/variant pairs that would exist after renaming `current`'s family
// to `newName` and setting this row's variant to `newVariant`.
static Result<void> projectedNamesAreUnique(const std::vector<Deck>& decks,
                                            const Deck& current,
                                            const std::string& newName,
                                            const std::string& newVariant) {
    struct Row {
        std::string name;
        std::string variant;
    };
    std::vector<Row> rows;
    for (const auto& deck : decks) {
        if (deck.formatId != current.formatId) continue;
        if (deck.rotationId != current.rotationId) continue;
        Row row;
        row.name = sameText(deck.name, current.name) ? newName : deck.name;
        row.variant = deck.id == current.id ? newVariant : deck.variant;
        for (const auto& existing : rows) {
            if (sameText(existing.name, row.name) &&
                sameText(existing.variant, row.variant)) {
                return Result<void>::err(
                    "A deck named \"" + row.name + "\" with variant \"" +
                    row.variant + "\" already exists in this format.");
            }
        }
        rows.push_back(std::move(row));
    }
    return Result<void>::ok();
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
                                 const std::string& variantNote,
                                 std::int64_t rotationId) {
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
                        trimmedNote, rotationId);
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

    auto current = repo_.findById(deckId);
    if (!current) return Result<Deck>::err(current.error());

    if (trimmedName != current.value().name) {
        auto listed = repo_.listByFormat(current.value().formatId,
                                         current.value().rotationId);
        if (!listed) return Result<Deck>::err(listed.error());
        auto unique = projectedNamesAreUnique(
            listed.value(), current.value(), trimmedName, trimmedVariant);
        if (!unique) return Result<Deck>::err(unique.error());

        auto renamed = repo_.renameFamily(
            current.value().formatId, current.value().name, trimmedName,
            current.value().rotationId);
        if (!renamed) return Result<Deck>::err(renamed.error());

        auto updated = repo_.update(deckId, archetypeId, trimmedName,
                                    trimmedVariant, trimmedNote);
        if (!updated) {
            const auto reverted = repo_.renameFamily(
                current.value().formatId, trimmedName, current.value().name,
                current.value().rotationId);
            (void)reverted;
            return Result<Deck>::err(updated.error());
        }
        return updated;
    }

    return repo_.update(deckId, archetypeId, trimmedName, trimmedVariant,
                        trimmedNote);
}

Result<void> DeckService::renameFamily(std::int64_t formatId,
                                       const std::string& fromName,
                                       const std::string& toName,
                                       std::int64_t rotationId) {
    if (formatId <= 0) {
        return Result<void>::err("No format selected.");
    }
    const std::string from = trim(fromName);
    const std::string to = trim(toName);
    if (from.empty() || to.empty()) {
        return Result<void>::err("Deck name must not be empty.");
    }
    if (from == to) return Result<void>::ok();
    return repo_.renameFamily(formatId, from, to, rotationId);
}

Result<int> DeckService::deleteVariant(std::int64_t deckId) {
    if (deckId <= 0) return Result<int>::err("No deck selected.");

    auto deck = repo_.findById(deckId);
    if (!deck) return Result<int>::err(deck.error());
    if (deck.value().variant.empty()) {
        return Result<int>::err(
            "This is the main deck. Delete its variants first, then delete "
            "the deck.");
    }
    return repo_.remove(deckId);
}

Result<int> DeckService::deleteDeck(std::int64_t deckId) {
    if (deckId <= 0) return Result<int>::err("No deck selected.");

    auto deck = repo_.findById(deckId);
    if (!deck) return Result<int>::err(deck.error());
    if (!deck.value().variant.empty()) {
        return Result<int>::err(
            "This row is a variante. Delete the variante instead.");
    }

    auto listed = repo_.listByFormat(deck.value().formatId,
                                     deck.value().rotationId);
    if (!listed) return Result<int>::err(listed.error());
    for (const auto& other : listed.value()) {
        if (sameText(other.name, deck.value().name) &&
            !other.variant.empty()) {
            return Result<int>::err(
                "Delete the variants of \"" + deck.value().name +
                "\" before deleting the deck.");
        }
    }
    return repo_.remove(deckId);
}

Result<std::vector<Deck>> DeckService::listByFormat(std::int64_t formatId,
                                                    std::int64_t rotationId) {
    return repo_.listByFormat(formatId, rotationId);
}

}  // namespace tracker
