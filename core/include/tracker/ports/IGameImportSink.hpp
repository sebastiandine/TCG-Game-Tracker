#pragma once

// IGameImportSink: UI (or test) callbacks while importing games.
// Used to choose details for newly created core decks and game types, and to
// report row progress. Cancel is expressed as Result::err / false.

#include "tracker/domain/Enums.hpp"
#include "tracker/util/Result.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace tracker {

struct GameTypeChoice {
    Competitiveness competitiveness{Competitiveness::NonCompetitive};
    PlayMedium      medium{PlayMedium::Online};
};

class IGameImportSink {
public:
    virtual ~IGameImportSink() = default;

    // Called once per new core deck name. Return the archetype id to use, or
    // err if the user skipped/cancelled.
    virtual Result<std::int64_t> chooseArchetype(const std::string& deckName) = 0;

    // Called once per new game type name. Return competitiveness + medium, or
    // err if the user skipped/cancelled.
    virtual Result<GameTypeChoice> chooseGameType(const std::string& eventName) = 0;

    // Called before each row (current is 0-based). Return false to stop the
    // remaining rows; already-imported rows are kept.
    virtual bool onProgress(std::size_t current, std::size_t total) = 0;
};

}  // namespace tracker
