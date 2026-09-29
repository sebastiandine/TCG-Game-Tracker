#pragma once

// GameTypeService: high-level operations on game types.
// Validates input (trim, reject empty) and delegates to IGameTypeRepository.

#include "tracker/domain/GameType.hpp"
#include "tracker/ports/IGameTypeRepository.hpp"
#include "tracker/util/Result.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tracker {

class GameTypeService {
public:
    explicit GameTypeService(IGameTypeRepository& repo);

    // Create a game type after trimming the name and rejecting empty names.
    [[nodiscard]] Result<GameType> create(
        std::int64_t gameId,
        const std::string& name,
        Competitiveness competitiveness,
        PlayMedium medium);

    // Return game types for `gameId` ordered by id ascending.
    [[nodiscard]] Result<std::vector<GameType>> listByGame(std::int64_t gameId);

private:
    IGameTypeRepository& repo_;
};

}  // namespace tracker
