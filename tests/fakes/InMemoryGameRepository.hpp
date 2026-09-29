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
                        const std::string& notes) override {
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

    Result<std::vector<Game>> listByFormat(std::int64_t formatId) override {
        std::vector<Game> result;
        for (const auto& g : games_) {
            if (g.formatId == formatId)
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

private:
    std::int64_t nextId_{1};
    std::vector<Game> games_;
};

}  // namespace tracker::testing
