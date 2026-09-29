#pragma once

// Spreadsheet writers for exporting game records. CSV and XLSX use the same
// column layout as game import so a file can be imported again.

#include "tracker/domain/Deck.hpp"
#include "tracker/domain/Enums.hpp"
#include "tracker/domain/Game.hpp"
#include "tracker/domain/GameType.hpp"
#include "tracker/util/Result.hpp"

#include <string>
#include <vector>

namespace tracker {

struct GameExportRow {
    std::string playedOn;          // YYYY-MM-DD
    std::string deckName;
    std::string variant;           // empty = core
    std::string opponentDeckName;
    MatchScore  score{MatchScore::TwoZero};
    std::string eventName;
    std::string opponent;
    std::string notes;
};

// Newest-first games become oldest-first rows. Missing deck or game type
// references are a hard error. An empty games list yields an empty vector
// (writers still emit a header row).
Result<std::vector<GameExportRow>> mapGamesForExport(
    const std::vector<Game>& games,
    const std::vector<Deck>& decks,
    const std::vector<GameType>& gameTypes);

// RFC 4180 comma CSV with a leading UTF-8 BOM. Fields that contain a comma,
// quote, or line break are quoted.
Result<std::string> writeGameExportCsv(const std::vector<GameExportRow>& rows);

// Minimal XLSX package (first worksheet, inline string cells) that Excel and
// parseXlsx can both open.
Result<std::string> writeGameExportXlsx(const std::vector<GameExportRow>& rows);

}  // namespace tracker
