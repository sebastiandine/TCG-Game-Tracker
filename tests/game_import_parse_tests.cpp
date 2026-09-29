#include <doctest/doctest.h>

#include "tracker/import/GameImportParse.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace tracker;

namespace {

void appendU16(std::string& o, std::uint16_t v) {
    o.push_back(static_cast<char>(v & 0xFF));
    o.push_back(static_cast<char>((v >> 8) & 0xFF));
}

void appendU32(std::string& o, std::uint32_t v) {
    o.push_back(static_cast<char>(v & 0xFF));
    o.push_back(static_cast<char>((v >> 8) & 0xFF));
    o.push_back(static_cast<char>((v >> 16) & 0xFF));
    o.push_back(static_cast<char>((v >> 24) & 0xFF));
}

std::uint32_t crc32(std::string_view data) {
    std::uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char b : data) {
        crc ^= b;
        for (int i = 0; i < 8; ++i) {
            crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

std::string makeZip(const std::vector<std::pair<std::string, std::string>>& files) {
    std::string out;
    struct Entry {
        std::string name;
        std::uint32_t crc{0};
        std::uint32_t size{0};
        std::uint32_t offset{0};
    };
    std::vector<Entry> entries;
    for (const auto& [name, data] : files) {
        Entry e;
        e.name = name;
        e.crc = crc32(data);
        e.size = static_cast<std::uint32_t>(data.size());
        e.offset = static_cast<std::uint32_t>(out.size());
        out += "PK\x03\x04";
        appendU16(out, 20);
        appendU16(out, 0);
        appendU16(out, 0);
        appendU16(out, 0);
        appendU16(out, 0);
        appendU32(out, e.crc);
        appendU32(out, e.size);
        appendU32(out, e.size);
        appendU16(out, static_cast<std::uint16_t>(name.size()));
        appendU16(out, 0);
        out += name;
        out += data;
        entries.push_back(std::move(e));
    }
    const std::uint32_t cdOffset = static_cast<std::uint32_t>(out.size());
    for (const auto& e : entries) {
        out += "PK\x01\x02";
        appendU16(out, 20);
        appendU16(out, 20);
        appendU16(out, 0);
        appendU16(out, 0);
        appendU16(out, 0);
        appendU16(out, 0);
        appendU32(out, e.crc);
        appendU32(out, e.size);
        appendU32(out, e.size);
        appendU16(out, static_cast<std::uint16_t>(e.name.size()));
        appendU16(out, 0);
        appendU16(out, 0);
        appendU16(out, 0);
        appendU16(out, 0);
        appendU32(out, 0);
        appendU32(out, e.offset);
        out += e.name;
    }
    const std::uint32_t cdSize =
        static_cast<std::uint32_t>(out.size()) - cdOffset;
    out += "PK\x05\x06";
    appendU16(out, 0);
    appendU16(out, 0);
    appendU16(out, static_cast<std::uint16_t>(entries.size()));
    appendU16(out, static_cast<std::uint16_t>(entries.size()));
    appendU32(out, cdSize);
    appendU32(out, cdOffset);
    appendU16(out, 0);
    return out;
}

std::string makeXlsx(const std::string& sharedStrings,
                     const std::string& sheetData) {
    const std::string workbook =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\""
        " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<sheets><sheet name=\"Sheet1\" sheetId=\"1\" r:id=\"rId1\"/></sheets>"
        "</workbook>";
    const std::string rels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\""
        " Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\""
        " Target=\"worksheets/sheet1.xml\"/>"
        "</Relationships>";
    const std::string sheet =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<sheetData>" +
        sheetData + "</sheetData></worksheet>";
    const std::string sst =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<sst xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">" +
        sharedStrings + "</sst>";
    return makeZip({
        {"xl/workbook.xml", workbook},
        {"xl/_rels/workbook.xml.rels", rels},
        {"xl/worksheets/sheet1.xml", sheet},
        {"xl/sharedStrings.xml", sst},
    });
}

}  // namespace

TEST_SUITE("GameImportParse") {
    TEST_CASE("parseCsv splits comma fields and strips BOM") {
        const std::string csv =
            "\xEF\xBB\xBF"
            "Date,Deck,Variante\n06.01.2026,Oath Ponza,GR Oath Ponza\n";
        auto grid = parseCsv(csv);
        REQUIRE(grid.isOk());
        REQUIRE(grid.value().size() == 2);
        CHECK(grid.value()[0][0] == "Date");
        CHECK(grid.value()[1][1] == "Oath Ponza");
        CHECK(grid.value()[1][2] == "GR Oath Ponza");
    }

    TEST_CASE("parseCsv sniffs semicolon delimiter") {
        const std::string csv = "Date;Deck;Opponent\n06.01.2026;Enchantress;Sligh\n";
        auto grid = parseCsv(csv);
        REQUIRE(grid.isOk());
        REQUIRE(grid.value().size() == 2);
        CHECK(grid.value()[1][0] == "06.01.2026");
        CHECK(grid.value()[1][2] == "Sligh");
    }

    TEST_CASE("parseCsv keeps quoted commas") {
        const std::string csv = "Notes\n\"hello, world\"\n";
        auto grid = parseCsv(csv);
        REQUIRE(grid.isOk());
        REQUIRE(grid.value().size() == 2);
        CHECK(grid.value()[1][0] == "hello, world");
    }

    TEST_CASE("parseCsv rejects unclosed quote") {
        auto grid = parseCsv("Deck\n\"oops\n");
        CHECK(grid.isErr());
    }

    TEST_CASE("parseCsv rejects empty input") {
        CHECK(parseCsv("").isErr());
        CHECK(parseCsv("\xEF\xBB\xBF").isErr());
    }

    TEST_CASE("mapGameImportTable converts DMY dates and empty variante") {
        const std::string csv =
            "Date,Deck,Variante,Opponent,Win,Result,Event,Opponent Player,Notes\n"
            "06.01.2026,Oath Ponza,,Sligh,TRUE,2-0,MTGO Practice,Clifford J,note one\n";
        auto grid = parseCsv(csv);
        REQUIRE(grid.isOk());
        auto mapped = mapGameImportTable(grid.value());
        REQUIRE(mapped.isOk());
        REQUIRE(mapped.value().rows.size() == 1);
        CHECK(mapped.value().issues.empty());
        const auto& row = mapped.value().rows[0];
        CHECK(row.playedOn == "2026-01-06");
        CHECK(row.deckName == "Oath Ponza");
        CHECK(row.variant.empty());
        CHECK(row.opponentDeckName == "Sligh");
        CHECK(row.score == MatchScore::TwoZero);
        CHECK(row.result == MatchResult::Win);
        CHECK(row.eventName == "MTGO Practice");
        CHECK(row.opponent == "Clifford J");
        CHECK(row.notes == "note one");
        CHECK(row.lineNumber == 2);
    }

    TEST_CASE("mapGameImportTable keeps named variante and ISO dates") {
        const std::string csv =
            "Date,Deck,Variante,Opponent,Result,Event\n"
            "2026-01-07,Oath Ponza,GR Oath Ponza,The Rock,1-2,MTGO League\n";
        auto mapped = mapGameImportTable(parseCsv(csv).value());
        REQUIRE(mapped.isOk());
        REQUIRE(mapped.value().rows.size() == 1);
        CHECK(mapped.value().rows[0].variant == "GR Oath Ponza");
        CHECK(mapped.value().rows[0].score == MatchScore::OneTwo);
        CHECK(mapped.value().rows[0].result == MatchResult::Loss);
    }

    TEST_CASE("mapGameImportTable accepts Variant alias and skips empty rows") {
        const std::string csv =
            "date,deck,variant,opponent,result,event\n"
            "\n"
            "08.01.2026,Enchantress, ,Pit Rack,0-2,MTGO Practice\n";
        auto mapped = mapGameImportTable(parseCsv(csv).value());
        REQUIRE(mapped.isOk());
        REQUIRE(mapped.value().rows.size() == 1);
        CHECK(mapped.value().rows[0].variant.empty());
        CHECK(mapped.value().rows[0].score == MatchScore::ZeroTwo);
    }

    TEST_CASE("mapGameImportTable records per-row errors") {
        const std::string csv =
            "Date,Deck,Opponent,Result,Event\n"
            "not-a-date,A,B,2-0,FNM\n"
            "06.01.2026,A,B,9-9,FNM\n"
            "06.01.2026,A,B,2-0,\n";
        auto mapped = mapGameImportTable(parseCsv(csv).value());
        REQUIRE(mapped.isOk());
        CHECK(mapped.value().rows.empty());
        REQUIRE(mapped.value().issues.size() == 3);
        CHECK(mapped.value().issues[0].lineNumber == 2);
        CHECK(mapped.value().issues[1].message.find("Result") != std::string::npos);
        CHECK(mapped.value().issues[2].message.find("Event") != std::string::npos);
    }

    TEST_CASE("mapGameImportTable rejects missing required headers") {
        auto mapped = mapGameImportTable({{"Date", "Deck"}});
        CHECK(mapped.isErr());
    }

    TEST_CASE("mapGameImportTable converts Excel serial dates") {
        auto mapped = mapGameImportTable({
            {"Date", "Deck", "Opponent", "Result", "Event"},
            {"46028", "A", "B", "2-1", "FNM"},
        });
        REQUIRE(mapped.isOk());
        REQUIRE(mapped.value().rows.size() == 1);
        CHECK(mapped.value().rows[0].playedOn == "2026-01-06");
        CHECK(mapped.value().rows[0].score == MatchScore::TwoOne);
    }

    TEST_CASE("parseXlsx reads shared strings numbers and booleans") {
        const std::string sst =
            "<si><t>Date</t></si>"
            "<si><t>Deck</t></si>"
            "<si><t>Variante</t></si>"
            "<si><t>Opponent</t></si>"
            "<si><t>Win</t></si>"
            "<si><t>Result</t></si>"
            "<si><t>Event</t></si>"
            "<si><t>Opponent Player</t></si>"
            "<si><t>Notes</t></si>"
            "<si><t>Oath Ponza</t></si>"
            "<si><t>GR Oath Ponza</t></si>"
            "<si><t>Sligh</t></si>"
            "<si><t>2-0</t></si>"
            "<si><t>MTGO Practice</t></si>"
            "<si><t>Clifford J</t></si>";
        const std::string sheet =
            "<row r=\"1\">"
            "<c r=\"A1\" t=\"s\"><v>0</v></c>"
            "<c r=\"B1\" t=\"s\"><v>1</v></c>"
            "<c r=\"C1\" t=\"s\"><v>2</v></c>"
            "<c r=\"D1\" t=\"s\"><v>3</v></c>"
            "<c r=\"E1\" t=\"s\"><v>4</v></c>"
            "<c r=\"F1\" t=\"s\"><v>5</v></c>"
            "<c r=\"G1\" t=\"s\"><v>6</v></c>"
            "<c r=\"H1\" t=\"s\"><v>7</v></c>"
            "<c r=\"I1\" t=\"s\"><v>8</v></c>"
            "</row>"
            "<row r=\"2\">"
            "<c r=\"A2\"><v>46028</v></c>"
            "<c r=\"B2\" t=\"s\"><v>9</v></c>"
            "<c r=\"C2\" t=\"s\"><v>10</v></c>"
            "<c r=\"D2\" t=\"s\"><v>11</v></c>"
            "<c r=\"E2\" t=\"b\"><v>1</v></c>"
            "<c r=\"F2\" t=\"s\"><v>12</v></c>"
            "<c r=\"G2\" t=\"s\"><v>13</v></c>"
            "<c r=\"H2\" t=\"s\"><v>14</v></c>"
            "</row>";
        auto grid = parseXlsx(makeXlsx(sst, sheet));
        REQUIRE(grid.isOk());
        REQUIRE(grid.value().size() == 2);
        CHECK(grid.value()[0][0] == "Date");
        CHECK(grid.value()[1][0] == "46028");
        CHECK(grid.value()[1][1] == "Oath Ponza");
        CHECK(grid.value()[1][2] == "GR Oath Ponza");
        CHECK(grid.value()[1][4] == "TRUE");
        CHECK(grid.value()[1][5] == "2-0");

        auto mapped = mapGameImportTable(grid.value());
        REQUIRE(mapped.isOk());
        REQUIRE(mapped.value().rows.size() == 1);
        CHECK(mapped.value().rows[0].playedOn == "2026-01-06");
        CHECK(mapped.value().rows[0].variant == "GR Oath Ponza");
        CHECK(mapped.value().rows[0].opponent == "Clifford J");
    }

    TEST_CASE("parseXlsx rejects empty bytes") {
        CHECK(parseXlsx("").isErr());
        CHECK(parseXlsx("not a zip").isErr());
    }
}
