#pragma once

// IGameTypeRepository: persistence port for GameType entities.

#include "tracker/domain/GameType.hpp"
#include "tracker/util/Result.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tracker {

class IGameTypeRepository {
public:
    virtual ~IGameTypeRepository() = default;

    // Create a game type with the given fields under `gameId`. Returns the
    // created GameType (with its assigned id). Fails if a game type with the
    // same name already exists in that game (case-insensitive).
    [[nodiscard]] virtual Result<GameType> create(
        std::int64_t gameId,
        const std::string& name,
        Competitiveness competitiveness,
        PlayMedium medium) = 0;

    // Return game types for `gameId` ordered by id ascending.
    [[nodiscard]] virtual Result<std::vector<GameType>> listByGame(
        std::int64_t gameId) = 0;
};

}  // namespace tracker
