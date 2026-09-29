#pragma once

// IFormatRepository: persistence port for Format entities.

#include "tracker/domain/Format.hpp"
#include "tracker/util/Result.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tracker {

class IFormatRepository {
public:
    virtual ~IFormatRepository() = default;

    // Create a format with the given name under `gameId`. Returns the created
    // Format (with its assigned id). Fails if a format with the same name
    // already exists in that game (case-insensitive).
    [[nodiscard]] virtual Result<Format> create(
        std::int64_t gameId, const std::string& name) = 0;

    // Return formats for `gameId` ordered by id ascending.
    [[nodiscard]] virtual Result<std::vector<Format>> listByGame(
        std::int64_t gameId) = 0;
};

}  // namespace tracker
