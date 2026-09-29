#pragma once

// Selection: pick a saved id when it is still present, otherwise the sole
// remaining item, otherwise 0. Used for game and format restore on startup.

#include <cstdint>
#include <vector>

namespace tracker {

// `Item` must have a `std::int64_t id` member.
template <typename Item>
std::int64_t resolveSavedOrSoleId(const std::vector<Item>& items,
                                  std::int64_t savedId) {
    if (savedId != 0) {
        for (const auto& item : items) {
            if (item.id == savedId) return savedId;
        }
    }
    if (items.size() == 1) return items.front().id;
    return 0;
}

}  // namespace tracker
