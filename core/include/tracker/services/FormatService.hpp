#pragma once

// FormatService: high-level operations on game formats and their rotations.
// Validates input (trim, reject empty) and delegates to the repositories.

#include "tracker/domain/Format.hpp"
#include "tracker/domain/Rotation.hpp"
#include "tracker/ports/IFormatRepository.hpp"
#include "tracker/ports/IRotationRepository.hpp"
#include "tracker/util/Result.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tracker {

struct CreatedRotatingFormat {
    Format   format;
    Rotation rotation;
};

class FormatService {
public:
    FormatService(IFormatRepository& formats, IRotationRepository& rotations);

    // Create a non-rotating format after trimming whitespace.
    [[nodiscard]] Result<Format> create(std::int64_t gameId,
                                        const std::string& name);

    // Create a rotating format and its first rotation together.
    // An unknown end is both optionals empty.
    [[nodiscard]] Result<CreatedRotatingFormat> createRotating(
        std::int64_t gameId,
        const std::string& formatName,
        const std::string& rotationName,
        int startYear,
        int startMonth,
        std::optional<int> endYear,
        std::optional<int> endMonth);

    // Add a rotation to an existing rotating format.
    [[nodiscard]] Result<Rotation> createRotation(
        std::int64_t formatId,
        const std::string& name,
        int startYear,
        int startMonth,
        std::optional<int> endYear,
        std::optional<int> endMonth);

    // Change the name and span of a rotation on a rotating format.
    // Changing the end clears the expiry prompt.
    [[nodiscard]] Result<Rotation> updateRotation(
        std::int64_t formatId,
        std::int64_t rotationId,
        const std::string& name,
        int startYear,
        int startMonth,
        std::optional<int> endYear,
        std::optional<int> endMonth);

    // Return formats for `gameId` ordered by id ascending.
    [[nodiscard]] Result<std::vector<Format>> listByGame(std::int64_t gameId);

    // Rotations for `formatId`, ordered by start date then id.
    [[nodiscard]] Result<std::vector<Rotation>> listRotations(
        std::int64_t formatId);

    // Remember that the expired-rotation prompt was shown.
    [[nodiscard]] Result<void> markExpiryPrompted(std::int64_t rotationId);

private:
    [[nodiscard]] Result<std::string> trimmedName(const std::string& name,
                                                  const char* emptyMessage) const;

    IFormatRepository&   formats_;
    IRotationRepository& rotations_;
};

}  // namespace tracker
