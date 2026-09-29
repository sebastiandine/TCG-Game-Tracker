#include "tracker/import/GameImportParse.hpp"

#include "ImportUtil.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <optional>
#include <utility>

namespace tracker {
namespace {

using import_detail::iequals;
using import_detail::toLower;
using import_detail::trim;

std::string formatIso(int year, unsigned month, unsigned day) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02u-%02u", year, month, day);
    return std::string(buf);
}

std::optional<std::string> parseIsoDate(const std::string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return std::nullopt;
    int y = 0;
    int m = 0;
    int d = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d) != 3) return std::nullopt;
    const std::chrono::year_month_day ymd{
        std::chrono::year{y},
        std::chrono::month{static_cast<unsigned>(m)},
        std::chrono::day{static_cast<unsigned>(d)}};
    if (!ymd.ok()) return std::nullopt;
    return formatIso(y, static_cast<unsigned>(m), static_cast<unsigned>(d));
}

std::optional<std::string> parseDmyDate(const std::string& s) {
    int d = 0;
    int m = 0;
    int y = 0;
    if (std::sscanf(s.c_str(), "%d.%d.%d", &d, &m, &y) != 3) return std::nullopt;
    if (y < 1000) return std::nullopt;
    const std::chrono::year_month_day ymd{
        std::chrono::year{y},
        std::chrono::month{static_cast<unsigned>(m)},
        std::chrono::day{static_cast<unsigned>(d)}};
    if (!ymd.ok()) return std::nullopt;
    return formatIso(y, static_cast<unsigned>(m), static_cast<unsigned>(d));
}

std::optional<std::string> parseExcelSerial(const std::string& s) {
    bool seenDot = false;
    bool anyDigit = false;
    for (char c : s) {
        if (c >= '0' && c <= '9') {
            anyDigit = true;
            continue;
        }
        if (c == '.' && !seenDot) {
            seenDot = true;
            continue;
        }
        return std::nullopt;
    }
    if (!anyDigit) return std::nullopt;

    const double serial = std::strtod(s.c_str(), nullptr);
    const int serialDays = static_cast<int>(serial);
    if (serialDays < 1 || serialDays > 80000) return std::nullopt;

    const std::chrono::sys_days epoch =
        std::chrono::sys_days{std::chrono::year{1899} / 12 / 30};
    const std::chrono::year_month_day ymd{
        epoch + std::chrono::days{serialDays}};
    if (!ymd.ok()) return std::nullopt;
    return formatIso(static_cast<int>(ymd.year()),
                     static_cast<unsigned>(ymd.month()),
                     static_cast<unsigned>(ymd.day()));
}

std::optional<std::string> parsePlayedOn(const std::string& raw) {
    const std::string s = trim(raw);
    if (s.empty()) return std::nullopt;
    if (auto iso = parseIsoDate(s)) return iso;
    if (auto dmy = parseDmyDate(s)) return dmy;
    if (auto serial = parseExcelSerial(s)) return serial;
    return std::nullopt;
}

std::string normalizeScore(std::string s) {
    s = trim(s);
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size();) {
        const auto uc = static_cast<unsigned char>(s[i]);
        if (uc == ' ' || uc == '\t') {
            ++i;
            continue;
        }
        if (uc == 0xE2 && i + 2 < s.size()) {
            const auto b1 = static_cast<unsigned char>(s[i + 1]);
            const auto b2 = static_cast<unsigned char>(s[i + 2]);
            if (b1 == 0x80 && (b2 == 0x93 || b2 == 0x94)) {
                out.push_back('-');
                i += 3;
                continue;
            }
            if (b1 == 0x88 && b2 == 0x92) {
                out.push_back('-');
                i += 3;
                continue;
            }
        }
        out.push_back(s[i]);
        ++i;
    }
    return out;
}

bool isEmptyRow(const std::vector<std::string>& row) {
    for (const auto& cell : row) {
        if (!trim(cell).empty()) return false;
    }
    return true;
}

std::string cellAt(const std::vector<std::string>& row, int idx) {
    if (idx < 0) return {};
    const auto u = static_cast<std::size_t>(idx);
    if (u >= row.size()) return {};
    return row[u];
}

int findColumn(const std::vector<std::string>& header,
               std::initializer_list<const char*> names) {
    for (std::size_t i = 0; i < header.size(); ++i) {
        const std::string key = toLower(trim(header[i]));
        for (const char* name : names) {
            if (key == name) return static_cast<int>(i);
        }
    }
    return -1;
}

}  // namespace

Result<GameImportParseResult> mapGameImportTable(
    const std::vector<std::vector<std::string>>& cells) {
    if (cells.empty()) {
        return Result<GameImportParseResult>::err(
            "Import file has no header row.");
    }

    const auto& header = cells[0];
    const int colDate = findColumn(header, {"date"});
    const int colDeck = findColumn(header, {"deck"});
    const int colVariant = findColumn(header, {"variante", "variant"});
    const int colOppDeck = findColumn(header, {"opponent"});
    const int colResult = findColumn(header, {"result"});
    const int colEvent = findColumn(header, {"event"});
    const int colOppPlayer =
        findColumn(header, {"opponent player", "opponent_player",
                            "opponentplayer"});
    const int colNotes = findColumn(header, {"notes"});

    if (colDate < 0 || colDeck < 0 || colOppDeck < 0 || colResult < 0 ||
        colEvent < 0) {
        return Result<GameImportParseResult>::err(
            "Import file is missing required columns "
            "(Date, Deck, Opponent, Result, Event).");
    }

    GameImportParseResult out;
    for (std::size_t r = 1; r < cells.size(); ++r) {
        const std::size_t line = r + 1;
        if (isEmptyRow(cells[r])) continue;

        GameImportRow row;
        row.lineNumber = line;

        const auto playedOn = parsePlayedOn(cellAt(cells[r], colDate));
        if (!playedOn) {
            out.issues.push_back(
                {line, "Date must be DD.MM.YYYY, YYYY-MM-DD, or an Excel date."});
            continue;
        }
        row.playedOn = *playedOn;

        row.deckName = trim(cellAt(cells[r], colDeck));
        if (row.deckName.empty()) {
            out.issues.push_back({line, "Deck name must not be empty."});
            continue;
        }

        row.variant = trim(cellAt(cells[r], colVariant));

        row.opponentDeckName = trim(cellAt(cells[r], colOppDeck));
        if (row.opponentDeckName.empty()) {
            out.issues.push_back({line, "Opponent deck must not be empty."});
            continue;
        }

        const auto score = matchScoreFromString(
            normalizeScore(cellAt(cells[r], colResult)));
        if (!score) {
            out.issues.push_back(
                {line, "Result must be one of 2-0, 2-1, 1-2, 0-2, 1-1."});
            continue;
        }
        row.score = *score;
        row.result = resultForScore(*score);

        row.eventName = trim(cellAt(cells[r], colEvent));
        if (row.eventName.empty()) {
            out.issues.push_back({line, "Event must not be empty."});
            continue;
        }

        row.opponent = trim(cellAt(cells[r], colOppPlayer));
        row.notes = trim(cellAt(cells[r], colNotes));
        out.rows.push_back(std::move(row));
    }

    return Result<GameImportParseResult>::ok(std::move(out));
}

}  // namespace tracker
