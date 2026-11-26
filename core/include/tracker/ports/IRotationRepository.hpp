#pragma once

// IRotationRepository: persistence port for Rotation entities.

#include "tracker/domain/Rotation.hpp"
#include "tracker/util/Result.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tracker {

class IRotationRepository {
public:
    virtual ~IRotationRepository() = default;

    // Create a rotation under `formatId`. Fails if the name already exists
    // in that format (case-insensitive), the span is rejected by the schema,
    // or the format does not exist. An unknown end is both optionals empty.
    [[nodiscard]] virtual Result<Rotation> create(
        std::int64_t formatId,
        const std::string& name,
        int startYear,
        int startMonth,
        std::optional<int> endYear,
        std::optional<int> endMonth) = 0;

    // Replace the name and span of an existing rotation. Fails when the row
    // is missing, the name already exists in that format, or the span is
    // rejected. Changing the end clears expiry_prompted.
    [[nodiscard]] virtual Result<Rotation> update(
        std::int64_t rotationId,
        const std::string& name,
        int startYear,
        int startMonth,
        std::optional<int> endYear,
        std::optional<int> endMonth) = 0;

    // Rotations for `formatId`, ordered by start year, start month, then id.
    [[nodiscard]] virtual Result<std::vector<Rotation>> listByFormat(
        std::int64_t formatId) = 0;

    // Remember that the expired-rotation prompt was shown for this rotation.
    [[nodiscard]] virtual Result<void> markExpiryPrompted(
        std::int64_t rotationId) = 0;
};

}  // namespace tracker
