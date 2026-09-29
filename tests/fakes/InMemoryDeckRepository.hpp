#pragma once

// InMemoryDeckRepository: test fake backed by vectors.

#include "tracker/ports/IDeckRepository.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace tracker::testing {

class InMemoryDeckRepository final : public IDeckRepository {
public:
    Result<DeckArchetype> createArchetype(
            std::int64_t gameId, const std::string& name) override {
        for (const auto& a : archetypes_) {
            if (a.gameId == gameId && equalsIgnoreCase(a.name, name)) {
                return Result<DeckArchetype>::err(
                    "An archetype named \"" + name + "\" already exists.");
            }
        }
        DeckArchetype a;
        a.id = nextArchetypeId_++;
        a.gameId = gameId;
        a.name = name;
        archetypes_.push_back(a);
        return Result<DeckArchetype>::ok(a);
    }

    Result<std::vector<DeckArchetype>> listByGame(std::int64_t gameId) override {
        std::vector<DeckArchetype> out;
        for (const auto& a : archetypes_) {
            if (a.gameId == gameId) out.push_back(a);
        }
        return Result<std::vector<DeckArchetype>>::ok(std::move(out));
    }

    Result<Deck> create(std::int64_t formatId,
                        std::int64_t archetypeId,
                        const std::string& name,
                        const std::string& variant,
                        const std::string& variantNote) override {
        bool archetypeFound = false;
        for (const auto& a : archetypes_) {
            if (a.id == archetypeId) { archetypeFound = true; break; }
        }
        if (!archetypeFound) {
            return Result<Deck>::err("Invalid format or archetype reference.");
        }

        for (const auto& d : decks_) {
            if (d.formatId == formatId &&
                equalsIgnoreCase(d.name, name) &&
                equalsIgnoreCase(d.variant, variant)) {
                return Result<Deck>::err(
                    "A deck named \"" + name + "\" with variant \"" + variant +
                    "\" already exists in this format.");
            }
        }

        Deck d;
        d.id = nextId_++;
        d.formatId = formatId;
        d.archetypeId = archetypeId;
        d.name = name;
        d.variant = variant;
        d.variantNote = variantNote;
        decks_.push_back(d);
        return Result<Deck>::ok(d);
    }

    Result<Deck> update(std::int64_t deckId,
                        std::int64_t archetypeId,
                        const std::string& name,
                        const std::string& variant,
                        const std::string& variantNote) override {
        bool archetypeFound = false;
        for (const auto& a : archetypes_) {
            if (a.id == archetypeId) { archetypeFound = true; break; }
        }
        if (!archetypeFound) {
            return Result<Deck>::err("Invalid archetype reference.");
        }

        Deck* target = nullptr;
        for (auto& d : decks_) {
            if (d.id == deckId) { target = &d; break; }
        }
        if (target == nullptr) {
            return Result<Deck>::err("Deck not found.");
        }

        for (const auto& d : decks_) {
            if (d.id != deckId &&
                d.formatId == target->formatId &&
                equalsIgnoreCase(d.name, name) &&
                equalsIgnoreCase(d.variant, variant)) {
                return Result<Deck>::err(
                    "A deck named \"" + name + "\" with variant \"" + variant +
                    "\" already exists in this format.");
            }
        }

        target->archetypeId = archetypeId;
        target->name = name;
        target->variant = variant;
        target->variantNote = variantNote;
        return Result<Deck>::ok(*target);
    }

    Result<std::vector<Deck>> listByFormat(std::int64_t formatId) override {
        std::vector<Deck> result;
        for (const auto& d : decks_) {
            if (d.formatId == formatId)
                result.push_back(d);
        }
        return Result<std::vector<Deck>>::ok(result);
    }

    void seedArchetypes(std::int64_t gameId = 1) {
        archetypes_ = {
            {1, gameId, "Aggro"},
            {2, gameId, "Midrange"},
            {3, gameId, "Control"},
            {4, gameId, "Combo"},
        };
        nextArchetypeId_ = 5;
    }

private:
    static bool equalsIgnoreCase(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i])))
                return false;
        }
        return true;
    }

    std::int64_t nextId_{1};
    std::int64_t nextArchetypeId_{1};
    std::vector<DeckArchetype> archetypes_;
    std::vector<Deck> decks_;
};

}  // namespace tracker::testing
