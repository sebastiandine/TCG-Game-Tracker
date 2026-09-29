#pragma once

// SqliteDeckRepository: IDeckRepository backed by a SqliteDatabase.

#include "tracker/ports/IDeckRepository.hpp"

namespace tracker {

class SqliteDatabase;

class SqliteDeckRepository final : public IDeckRepository {
public:
    explicit SqliteDeckRepository(SqliteDatabase& db);

    [[nodiscard]] Result<DeckArchetype> createArchetype(
        std::int64_t gameId, const std::string& name) override;
    [[nodiscard]] Result<std::vector<DeckArchetype>> listByGame(
        std::int64_t gameId) override;

    [[nodiscard]] Result<Deck> create(std::int64_t formatId,
                                      std::int64_t archetypeId,
                                      const std::string& name,
                                      const std::string& variant,
                                      const std::string& variantNote) override;

    [[nodiscard]] Result<Deck> update(std::int64_t deckId,
                                      std::int64_t archetypeId,
                                      const std::string& name,
                                      const std::string& variant,
                                      const std::string& variantNote) override;

    [[nodiscard]] Result<std::vector<Deck>> listByFormat(
        std::int64_t formatId) override;

private:
    SqliteDatabase& db_;
};

}  // namespace tracker
