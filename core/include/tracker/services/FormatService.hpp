#pragma once

// FormatService: high-level operations on game formats.
// Validates input (trim, reject empty) and delegates to IFormatRepository.

#include "tracker/domain/Format.hpp"
#include "tracker/ports/IFormatRepository.hpp"
#include "tracker/util/Result.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tracker {

class FormatService {
public:
    explicit FormatService(IFormatRepository& repo);

    // Create a format after trimming whitespace and rejecting empty names.
    [[nodiscard]] Result<Format> create(std::int64_t gameId,
                                        const std::string& name);

    // Return formats for `gameId` ordered by id ascending.
    [[nodiscard]] Result<std::vector<Format>> listByGame(std::int64_t gameId);

private:
    IFormatRepository& repo_;
};

}  // namespace tracker
