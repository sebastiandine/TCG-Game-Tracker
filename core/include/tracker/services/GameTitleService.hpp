#pragma once

// GameTitleService: high-level operations on game titles.
// Validates input (trim, reject empty) and delegates to IGameTitleRepository.

#include "tracker/domain/GameTitle.hpp"
#include "tracker/ports/IGameTitleRepository.hpp"
#include "tracker/util/Result.hpp"

#include <string>
#include <vector>

namespace tracker {

class GameTitleService {
public:
    explicit GameTitleService(IGameTitleRepository& repo);

    // Create a game title after trimming whitespace and rejecting empty names.
    [[nodiscard]] Result<GameTitle> create(const std::string& name);

    // Return all game titles ordered by id ascending.
    [[nodiscard]] Result<std::vector<GameTitle>> listAll();

private:
    IGameTitleRepository& repo_;
};

}  // namespace tracker
