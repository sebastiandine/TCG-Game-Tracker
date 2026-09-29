#include <doctest/doctest.h>

#include "tracker/domain/Deck.hpp"
#include "tracker/domain/Game.hpp"
#include "tracker/domain/GameType.hpp"
#include "tracker/export/GameExportWrite.hpp"
#include "tracker/import/GameImportParse.hpp"

#include <string>

using namespace tracker;

namespace {

GameExportRow sampleRow() {
    GameExportRow row;
    row.playedOn = "2026-01-06";
    row.deckName = "Oath Ponza";
    row.variant = "GR Oath Ponza";
    row.opponentDeckName = "Sligh";
    row.score = MatchScore::OneTwo;
    row.eventName = "MTGO League";
    row.opponent = "Clifford J";
    row.notes = "hello, world";
    return row;
}

void checkMappedSample(const GameImportRow& row) {
    CHECK(row.playedOn == "2026-01-06");
    CHECK(row.deckName == "Oath Ponza");
    CHECK(row.variant == "GR Oath Ponza");
    CHECK(row.opponentDeckName == "Sligh");
    CHECK(row.score == MatchScore::OneTwo);
    CHECK(row.result == MatchResult::Loss);
    CHECK(row.eventName == "MTGO League");
    CHECK(row.opponent == "Clifford J");
    CHECK(row.notes == "hello, world");
}

}  // namespace

TEST_SUITE("GameExportWrite") {
    TEST_CASE("mapGamesForExport reverses newest-first games") {
        const Deck player{1, 1, 1, "Oath Ponza", "GR Oath Ponza", ""};
        const Deck opp{2, 1, 1, "Sligh", "", ""};
        const GameType event{3, 1, "MTGO Practice",
                             Competitiveness::NonCompetitive,
                             PlayMedium::Online};
        const Game newer{10, 1, "2026-01-07", 1, 2, "Ann", MatchResult::Win,
                         MatchScore::TwoZero, 3, ""};
        const Game older{9, 1, "2026-01-06", 1, 2, "Bob", MatchResult::Loss,
                         MatchScore::OneTwo, 3, "note"};

        auto rows = mapGamesForExport({newer, older}, {player, opp}, {event});
        REQUIRE(rows.isOk());
        REQUIRE(rows.value().size() == 2);
        CHECK(rows.value()[0].playedOn == "2026-01-06");
        CHECK(rows.value()[0].opponent == "Bob");
        CHECK(rows.value()[0].variant == "GR Oath Ponza");
        CHECK(rows.value()[0].opponentDeckName == "Sligh");
        CHECK(rows.value()[0].eventName == "MTGO Practice");
        CHECK(rows.value()[1].playedOn == "2026-01-07");
        CHECK(rows.value()[1].opponent == "Ann");
        CHECK(rows.value()[1].score == MatchScore::TwoZero);
    }

    TEST_CASE("mapGamesForExport accepts an empty game list") {
        auto rows = mapGamesForExport({}, {}, {});
        REQUIRE(rows.isOk());
        CHECK(rows.value().empty());
    }

    TEST_CASE("mapGamesForExport rejects a missing deck") {
        const GameType event{3, 1, "FNM", Competitiveness::NonCompetitive,
                             PlayMedium::Paper};
        const Game g{12, 1, "2026-01-06", 99, 2, "", MatchResult::Win,
                     MatchScore::TwoZero, 3, ""};
        auto rows = mapGamesForExport({g}, {}, {event});
        REQUIRE(rows.isErr());
        CHECK(rows.error().find("unknown deck 99") != std::string::npos);
    }

    TEST_CASE("mapGamesForExport rejects a missing event type") {
        const Deck player{1, 1, 1, "A", "", ""};
        const Deck opp{2, 1, 1, "B", "", ""};
        const Game g{12, 1, "2026-01-06", 1, 2, "", MatchResult::Win,
                     MatchScore::TwoZero, 5, ""};
        auto rows = mapGamesForExport({g}, {player, opp}, {});
        REQUIRE(rows.isErr());
        CHECK(rows.error().find("unknown event type 5") != std::string::npos);
    }

    TEST_CASE("writeGameExportCsv quotes commas and doubled quotes") {
        GameExportRow row = sampleRow();
        row.deckName = "Oath, Ponza";
        row.notes = "said \"hi\"";
        auto csv = writeGameExportCsv({row});
        REQUIRE(csv.isOk());
        CHECK(csv.value().find("\"Oath, Ponza\"") != std::string::npos);
        CHECK(csv.value().find("\"said \"\"hi\"\"\"") != std::string::npos);
    }

    TEST_CASE("writeGameExportCsv round-trips through import mapping") {
        auto csv = writeGameExportCsv({sampleRow()});
        REQUIRE(csv.isOk());
        auto grid = parseCsv(csv.value());
        REQUIRE(grid.isOk());
        auto mapped = mapGameImportTable(grid.value());
        REQUIRE(mapped.isOk());
        REQUIRE(mapped.value().rows.size() == 1);
        CHECK(mapped.value().issues.empty());
        checkMappedSample(mapped.value().rows[0]);
    }

    TEST_CASE("writeGameExportCsv emits a header-only file for no rows") {
        auto csv = writeGameExportCsv({});
        REQUIRE(csv.isOk());
        auto grid = parseCsv(csv.value());
        REQUIRE(grid.isOk());
        REQUIRE(grid.value().size() == 1);
        CHECK(grid.value()[0][0] == "Date");
        auto mapped = mapGameImportTable(grid.value());
        REQUIRE(mapped.isOk());
        CHECK(mapped.value().rows.empty());
    }

    TEST_CASE("writeGameExportXlsx round-trips through import mapping") {
        auto xlsx = writeGameExportXlsx({sampleRow()});
        REQUIRE(xlsx.isOk());
        auto grid = parseXlsx(xlsx.value());
        REQUIRE(grid.isOk());
        auto mapped = mapGameImportTable(grid.value());
        REQUIRE(mapped.isOk());
        REQUIRE(mapped.value().rows.size() == 1);
        CHECK(mapped.value().issues.empty());
        checkMappedSample(mapped.value().rows[0]);
    }

    TEST_CASE("writeGameExportXlsx emits a header-only sheet for no rows") {
        auto xlsx = writeGameExportXlsx({});
        REQUIRE(xlsx.isOk());
        auto grid = parseXlsx(xlsx.value());
        REQUIRE(grid.isOk());
        REQUIRE(grid.value().size() == 1);
        CHECK(grid.value()[0][0] == "Date");
        auto mapped = mapGameImportTable(grid.value());
        REQUIRE(mapped.isOk());
        CHECK(mapped.value().rows.empty());
    }
}
