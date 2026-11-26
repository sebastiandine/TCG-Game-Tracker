#pragma once

// IRotationRepository: persistence port for Rotation entities.

#include "tracker/domain/Rotation.hpp"
#include "tracker/util/Result.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tracker {

class IRotationRepository {
public:
    virtual ~IRotationRepository() = default;

    // Create a rotation under `formatId`. Fails if the name already exists
    // in that format (case-insensitive), the span is rejected by the schema,
    // or the format does not exist.
    [[nodiscard]] virtual Result<Rotation> create(
        std::int64_t formatId,
        const std::string& name,
        int startYear,
        int startMonth,
        int endYear,
        int endMonth) = 0;

    // Rotations for `formatId`, ordered by start year, start month, then id.
    [[nodiscard]] virtual Result<std::vector<Rotation>> listByFormat(
        std::int64_t formatId) = 0;

    // Remember that the expired-rotation prompt was shown for this rotation.
    [[nodiscard]] virtual Result<void> markExpiryPrompted(
        std::int64_t rotationId) = 0;
};

}  // namespace tracker
