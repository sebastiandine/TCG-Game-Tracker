#pragma once

// Spreadsheet parsers and header mapping for game-record import.
// CSV and XLSX both produce a cell grid; the mapper turns that into rows.

#include "tracker/domain/Enums.hpp"
#include "tracker/util/Result.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace tracker {

struct GameImportRow {
    std::size_t  lineNumber{0};  // 1-based spreadsheet row (header is 1)
    std::string  playedOn;       // YYYY-MM-DD
    std::string  deckName;
    std::string  variant;        // empty = core
    std::string  opponentDeckName;
    MatchScore   score{MatchScore::TwoZero};
    MatchResult  result{MatchResult::Win};
    std::string  eventName;
    std::string  opponent;
    std::string  notes;
};

struct GameImportIssue {
    std::size_t lineNumber{0};
    std::string message;
};

struct GameImportParseResult {
    std::vector<GameImportRow>    rows;
    std::vector<GameImportIssue>  issues;
};

// RFC 4180 CSV. Sniffs comma vs semicolon from the header line. Strips a
// leading UTF-8 BOM. Quoted fields and embedded newlines are supported.
Result<std::vector<std::vector<std::string>>> parseCsv(std::string_view text);

// First worksheet of an XLSX package, decoded from an in-memory ZIP.
// Shared strings, inline strings, booleans, and numbers are converted to text.
Result<std::vector<std::vector<std::string>>> parseXlsx(std::string_view bytes);

// Map a cell grid (header + data) onto GameImportRow values. Missing required
// headers are a hard error. Per-row problems go into issues and those rows
// are omitted. Empty rows are skipped silently.
Result<GameImportParseResult> mapGameImportTable(
    const std::vector<std::vector<std::string>>& cells);

}  // namespace tracker
