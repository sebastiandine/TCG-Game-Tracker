#pragma once

// GameImportService: creates missing decks/variants/game types, then records
// games from a parsed spreadsheet.

#include "tracker/import/GameImportParse.hpp"
#include "tracker/ports/IGameImportSink.hpp"
#include "tracker/services/DeckService.hpp"
#include "tracker/services/GameService.hpp"
#include "tracker/services/GameTypeService.hpp"
#include "tracker/util/Result.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tracker {

struct GameImportReport {
    std::size_t gamesImported{0};
    std::size_t decksCreated{0};
    std::size_t gameTypesCreated{0};
    bool aborted{false};
    std::vector<GameImportIssue> skipped;
};

class GameImportService {
public:
    GameImportService(DeckService& decks,
                      GameTypeService& gameTypes,
                      GameService& games);

    // Import already-mapped rows into the given format. Game types and
    // inferred archetypes are resolved under `gameId`. The sink is asked for
    // an archetype on each new core deck unless the name already contains one
    // of that game's archetype labels, and for competitiveness/medium on each
    // new game type.
    [[nodiscard]] Result<GameImportReport> import(
        std::int64_t formatId,
        std::int64_t gameId,
        const std::vector<GameImportRow>& rows,
        IGameImportSink& sink);

private:
    Result<Deck> ensureCoreDeck(std::int64_t formatId,
                                const std::string& name,
                                IGameImportSink& sink,
                                GameImportReport& report);

    Result<Deck> ensurePlayerDeck(std::int64_t formatId,
                                  const std::string& name,
                                  const std::string& variant,
                                  IGameImportSink& sink,
                                  GameImportReport& report);

    Result<GameType> ensureGameType(std::int64_t gameId,
                                    const std::string& name,
                                    IGameImportSink& sink,
                                    GameImportReport& report);

    static std::string deckKey(const std::string& name,
                               const std::string& variant);

    std::optional<std::int64_t> inferArchetypeFromName(
        const std::string& name) const;

    DeckService&     decks_;
    GameTypeService& gameTypes_;
    GameService&     games_;

    std::vector<DeckArchetype>                archetypes_;

    std::unordered_map<std::string, Deck>     deckByKey_;
    std::unordered_map<std::string, GameType> gameTypeByName_;
    std::unordered_set<std::string>           declinedCores_;
    std::unordered_set<std::string>           declinedGameTypes_;
};

}  // namespace tracker
