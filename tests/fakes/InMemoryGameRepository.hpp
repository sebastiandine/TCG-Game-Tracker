#pragma once

// InMemoryGameRepository: test fake backed by a vector.

#include "tracker/ports/IGameRepository.hpp"

#include <algorithm>
#include <vector>

namespace tracker::testing {

class InMemoryGameRepository final : public IGameRepository {
public:
    Result<Game> create(std::int64_t formatId,
                        const std::string& playedOn,
                        std::int64_t deckId,
                        std::int64_t opponentDeckId,
                        const std::string& opponent,
                        MatchResult result,
                        MatchScore score,
                        std::int64_t gameTypeId,
                        const std::string& notes,
                        std::int64_t rotationId = 0) override {
        Game g;
        g.id = nextId_++;
        g.formatId = formatId;
        g.playedOn = playedOn;
        g.deckId = deckId;
        g.opponentDeckId = opponentDeckId;
        g.opponent = opponent;
        g.result = result;
        g.score = score;
        g.gameTypeId = gameTypeId;
        g.notes = notes;
        g.rotationId = rotationId;
        games_.push_back(g);
        return Result<Game>::ok(g);
    }

    Result<Game> update(std::int64_t gameId,
                        const std::string& playedOn,
                        std::int64_t deckId,
                        std::int64_t opponentDeckId,
                        const std::string& opponent,
                        MatchResult result,
                        MatchScore score,
                        std::int64_t gameTypeId,
                        const std::string& notes) override {
        for (auto& g : games_) {
            if (g.id == gameId) {
                g.playedOn = playedOn;
                g.deckId = deckId;
                g.opponentDeckId = opponentDeckId;
                g.opponent = opponent;
                g.result = result;
                g.score = score;
                g.gameTypeId = gameTypeId;
                g.notes = notes;
                return Result<Game>::ok(g);
            }
        }
        return Result<Game>::err("Game not found.");
    }

    Result<std::vector<Game>> listByFormat(std::int64_t formatId,
                                           std::int64_t rotationId = 0) override {
        std::vector<Game> result;
        for (const auto& g : games_) {
            if (g.formatId == formatId && g.rotationId == rotationId)
                result.push_back(g);
        }
        // Newest first: sort by playedOn desc, then id desc.
        std::sort(result.begin(), result.end(),
                  [](const Game& a, const Game& b) {
                      if (a.playedOn != b.playedOn)
                          return a.playedOn > b.playedOn;
                      return a.id > b.id;
                  });
        return Result<std::vector<Game>>::ok(std::move(result));
    }

    // Remove games that use `deckId` as the player deck or the opponent deck.
    // Returns how many games were removed.
    int eraseReferencing(std::int64_t deckId) {
        const auto before = games_.size();
        games_.erase(std::remove_if(games_.begin(), games_.end(),
                                    [deckId](const Game& game) {
                                        return game.deckId == deckId ||
                                               game.opponentDeckId == deckId;
                                    }),
                     games_.end());
        return static_cast<int>(before - games_.size());
    }

private:
    std::int64_t nextId_{1};
    std::vector<Game> games_;
};

}  // namespace tracker::testing
