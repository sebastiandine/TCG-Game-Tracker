#pragma once

// IGameTitleRepository: persistence port for GameTitle entities.

#include "tracker/domain/GameTitle.hpp"
#include "tracker/util/Result.hpp"

#include <string>
#include <vector>

namespace tracker {

class IGameTitleRepository {
public:
    virtual ~IGameTitleRepository() = default;

    // Create a game title with the given name. Returns the created GameTitle
    // (with its assigned id). Fails if a title with the same name already
    // exists (case-insensitive).
    [[nodiscard]] virtual Result<GameTitle> create(const std::string& name) = 0;

    // Return all game titles ordered by id ascending.
    [[nodiscard]] virtual Result<std::vector<GameTitle>> listAll() = 0;
};

}  // namespace tracker
