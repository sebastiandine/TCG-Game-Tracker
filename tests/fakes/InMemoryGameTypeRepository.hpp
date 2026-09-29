#pragma once

// InMemoryGameTypeRepository: test fake backed by a std::vector.

#include "tracker/ports/IGameTypeRepository.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace tracker::testing {

class InMemoryGameTypeRepository final : public IGameTypeRepository {
public:
    Result<GameType> create(
            std::int64_t gameId,
            const std::string& name,
            Competitiveness competitiveness,
            PlayMedium medium) override {
        auto it = std::find_if(gameTypes_.begin(), gameTypes_.end(),
            [&](const GameType& gt) {
                if (gt.gameId != gameId) return false;
                if (gt.name.size() != name.size()) return false;
                for (std::size_t i = 0; i < name.size(); ++i) {
                    if (std::tolower(static_cast<unsigned char>(gt.name[i])) !=
                        std::tolower(static_cast<unsigned char>(name[i])))
                        return false;
                }
                return true;
            });
        if (it != gameTypes_.end()) {
            return Result<GameType>::err(
                "A game type named \"" + name + "\" already exists.");
        }
        GameType gt;
        gt.id = nextId_++;
        gt.gameId = gameId;
        gt.name = name;
        gt.competitiveness = competitiveness;
        gt.medium = medium;
        gameTypes_.push_back(gt);
        return Result<GameType>::ok(gt);
    }

    Result<std::vector<GameType>> listByGame(std::int64_t gameId) override {
        std::vector<GameType> out;
        for (const auto& gt : gameTypes_) {
            if (gt.gameId == gameId) out.push_back(gt);
        }
        return Result<std::vector<GameType>>::ok(std::move(out));
    }

private:
    std::int64_t nextId_{1};
    std::vector<GameType> gameTypes_;
};

}  // namespace tracker::testing
