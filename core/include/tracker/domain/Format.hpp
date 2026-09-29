#pragma once

// Format: a named game format (e.g. "Standard", "Commander") scoped to a
// GameTitle. Persisted in the Formats SQLite table; no JSON serde needed.

#include <cstdint>
#include <string>

namespace tracker {

struct Format {
    std::int64_t id{0};
    std::int64_t gameId{0};
    std::string  name;

    friend bool operator==(const Format&, const Format&) = default;
};

}  // namespace tracker
