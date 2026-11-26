#pragma once

// Rotation: a named month-span inside a rotating format. Decks and games of
// that format belong to one rotation. Persisted in the Rotations table.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tracker {

struct Rotation {
    std::int64_t id{0};
    std::int64_t formatId{0};
    std::string  name;
    int          startYear{0};
    int          startMonth{0};  // 1-12
    int          endYear{0};
    int          endMonth{0};    // 1-12
    bool         expiryPrompted{false};

    friend bool operator==(const Rotation&, const Rotation&) = default;
};

// nullptr when the span is usable (months 1-12, years 1-9999, end not before start).
[[nodiscard]] inline const char* rotationSpanError(int startYear, int startMonth,
                                                   int endYear, int endMonth) {
    if (startMonth < 1 || startMonth > 12 || endMonth < 1 || endMonth > 12) {
        return "Rotation month must be from 1 to 12.";
    }
    if (startYear < 1 || startYear > 9999 || endYear < 1 || endYear > 9999) {
        return "Rotation year is out of range.";
    }
    if ((endYear * 12 + endMonth) < (startYear * 12 + startMonth)) {
        return "Rotation end is before the start.";
    }
    return nullptr;
}

[[nodiscard]] inline bool rotationSpanValid(int startYear, int startMonth,
                                            int endYear, int endMonth) {
    return rotationSpanError(startYear, startMonth, endYear, endMonth) == nullptr;
}

// True once `year`/`month` is strictly after the rotation's end month.
[[nodiscard]] inline bool rotationEnded(const Rotation& rotation, int year,
                                        int month) {
    return (year * 12 + month) >
           (rotation.endYear * 12 + rotation.endMonth);
}

// The month after `year`/`month`. December wraps to January of the next year.
inline void monthAfter(int year, int month, int& outYear, int& outMonth) {
    if (month >= 12) {
        outYear = year + 1;
        outMonth = 1;
    } else {
        outYear = year;
        outMonth = month + 1;
    }
}

// Latest rotation by start year, then start month, then id.
[[nodiscard]] inline std::optional<Rotation> latestRotation(
        const std::vector<Rotation>& rotations) {
    if (rotations.empty()) return std::nullopt;
    const Rotation* best = &rotations.front();
    for (const auto& rotation : rotations) {
        const int key = rotation.startYear * 12 + rotation.startMonth;
        const int bestKey = best->startYear * 12 + best->startMonth;
        if (key > bestKey || (key == bestKey && rotation.id > best->id)) {
            best = &rotation;
        }
    }
    return *best;
}

}  // namespace tracker
