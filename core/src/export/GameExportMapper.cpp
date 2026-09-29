#include "tracker/export/GameExportWrite.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace tracker {

Result<std::vector<GameExportRow>> mapGamesForExport(
    const std::vector<Game>& games,
    const std::vector<Deck>& decks,
    const std::vector<GameType>& gameTypes) {
    std::unordered_map<std::int64_t, const Deck*> deckById;
    deckById.reserve(decks.size());
    for (const auto& d : decks) {
        deckById.emplace(d.id, &d);
    }

    std::unordered_map<std::int64_t, const GameType*> typeById;
    typeById.reserve(gameTypes.size());
    for (const auto& t : gameTypes) {
        typeById.emplace(t.id, &t);
    }

    const auto missingRef = [](const Game& g, const char* kind,
                               std::int64_t id) {
        return "Game played on " + g.playedOn + " (id " +
               std::to_string(g.id) + ") references unknown " + kind + " " +
               std::to_string(id) + ".";
    };

    std::vector<GameExportRow> rows;
    rows.reserve(games.size());
    for (auto it = games.rbegin(); it != games.rend(); ++it) {
        const Game& g = *it;

        const auto player = deckById.find(g.deckId);
        if (player == deckById.end()) {
            return Result<std::vector<GameExportRow>>::err(
                missingRef(g, "deck", g.deckId));
        }
        const auto opponent = deckById.find(g.opponentDeckId);
        if (opponent == deckById.end()) {
            return Result<std::vector<GameExportRow>>::err(
                missingRef(g, "opponent deck", g.opponentDeckId));
        }
        const auto event = typeById.find(g.gameTypeId);
        if (event == typeById.end()) {
            return Result<std::vector<GameExportRow>>::err(
                missingRef(g, "event type", g.gameTypeId));
        }

        GameExportRow row;
        row.playedOn = g.playedOn;
        row.deckName = player->second->name;
        row.variant = player->second->variant;
        row.opponentDeckName = opponent->second->name;
        row.score = g.score;
        row.eventName = event->second->name;
        row.opponent = g.opponent;
        row.notes = g.notes;
        rows.push_back(std::move(row));
    }
    return Result<std::vector<GameExportRow>>::ok(std::move(rows));
}

}  // namespace tracker
