#include <doctest/doctest.h>

#include "tracker/infra/SqliteDatabase.hpp"
#include "tracker/infra/SqliteDeckRepository.hpp"
#include "tracker/infra/SqliteFormatRepository.hpp"
#include "tracker/infra/SqliteGameRepository.hpp"
#include "tracker/infra/SqliteGameTitleRepository.hpp"
#include "tracker/infra/SqliteGameTypeRepository.hpp"

#include <sqlite3.h>

using namespace tracker;

namespace {

Result<void> execSql(sqlite3* db, const char* sql) {
    char* errMsg = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        return Result<void>::err(msg);
    }
    return Result<void>::ok();
}

}  // namespace

TEST_SUITE("Sqlite schema migration") {
    TEST_CASE("legacy formats and game types attach to Magic with the same ids") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());

        REQUIRE(execSql(db.handle(), "PRAGMA foreign_keys = OFF;").isOk());
        REQUIRE(execSql(db.handle(),
            "DROP TABLE IF EXISTS Games;"
            "DROP TABLE IF EXISTS Decks;"
            "DROP TABLE IF EXISTS GameTypes;"
            "DROP TABLE IF EXISTS Formats;"
            "DROP TABLE IF EXISTS GameTitles;"
            "DROP TABLE IF EXISTS DeckArchetypes;"
        ).isOk());

        REQUIRE(execSql(db.handle(),
            "CREATE TABLE Formats ("
            "  id   INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  name TEXT    NOT NULL UNIQUE COLLATE NOCASE"
            ");"
            "CREATE TABLE DeckArchetypes ("
            "  id   INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  name TEXT    NOT NULL UNIQUE COLLATE NOCASE"
            ");"
            "INSERT INTO DeckArchetypes (name) VALUES"
            "  ('Aggro'), ('Midrange'), ('Control'), ('Combo');"
            "CREATE TABLE GameTypes ("
            "  id              INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  name            TEXT    NOT NULL UNIQUE COLLATE NOCASE,"
            "  competitiveness TEXT    NOT NULL,"
            "  medium          TEXT    NOT NULL"
            ");"
            "INSERT INTO GameTypes (name, competitiveness, medium) VALUES"
            "  ('MTGO Friendly', 'Non-Competitive', 'Online'),"
            "  ('MTGO League',   'Competitive',     'Online'),"
            "  ('FNM',           'Non-Competitive', 'Paper'),"
            "  ('Tournament',    'Competitive',     'Paper');"
            "CREATE TABLE Decks ("
            "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  format_id     INTEGER NOT NULL,"
            "  archetype_id  INTEGER NOT NULL,"
            "  name          TEXT    NOT NULL COLLATE NOCASE,"
            "  variant       TEXT    NOT NULL DEFAULT '' COLLATE NOCASE,"
            "  variant_note  TEXT    NOT NULL DEFAULT '',"
            "  FOREIGN KEY (format_id)    REFERENCES Formats(id),"
            "  FOREIGN KEY (archetype_id) REFERENCES DeckArchetypes(id),"
            "  UNIQUE (format_id, name, variant)"
            ");"
            "CREATE TABLE Games ("
            "  id               INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  format_id        INTEGER NOT NULL,"
            "  played_on        TEXT    NOT NULL,"
            "  deck_id          INTEGER NOT NULL,"
            "  opponent_deck_id INTEGER NOT NULL,"
            "  opponent         TEXT    NOT NULL DEFAULT '',"
            "  result           TEXT    NOT NULL,"
            "  score            TEXT    NOT NULL,"
            "  game_type_id     INTEGER NOT NULL,"
            "  notes            TEXT    NOT NULL DEFAULT '',"
            "  FOREIGN KEY (format_id)        REFERENCES Formats(id),"
            "  FOREIGN KEY (deck_id)          REFERENCES Decks(id),"
            "  FOREIGN KEY (opponent_deck_id) REFERENCES Decks(id),"
            "  FOREIGN KEY (game_type_id)     REFERENCES GameTypes(id)"
            ");"
            "INSERT INTO Formats (name) VALUES ('Vintage'), ('Legacy');"
        ).isOk());
        REQUIRE(execSql(db.handle(), "PRAGMA foreign_keys = ON;").isOk());

        REQUIRE(db.migrate().isOk());

        SqliteGameTitleRepository titles(db);
        auto listedTitles = titles.listAll();
        REQUIRE(listedTitles.isOk());
        REQUIRE(listedTitles.value().size() == 1);
        CHECK(listedTitles.value()[0].name == "Magic: The Gathering");
        const auto magicId = listedTitles.value()[0].id;

        SqliteFormatRepository formats(db);
        auto listedFormats = formats.listByGame(magicId);
        REQUIRE(listedFormats.isOk());
        REQUIRE(listedFormats.value().size() == 2);
        CHECK(listedFormats.value()[0].id == 1);
        CHECK(listedFormats.value()[0].name == "Vintage");
        CHECK(listedFormats.value()[0].gameId == magicId);
        CHECK(listedFormats.value()[1].id == 2);
        CHECK(listedFormats.value()[1].name == "Legacy");

        SqliteGameTypeRepository types(db);
        auto listedTypes = types.listByGame(magicId);
        REQUIRE(listedTypes.isOk());
        REQUIRE(listedTypes.value().size() == 4);
        CHECK(listedTypes.value()[0].id == 1);
        CHECK(listedTypes.value()[0].name == "MTGO Friendly");
        CHECK(listedTypes.value()[2].id == 3);
        CHECK(listedTypes.value()[2].name == "FNM");
        CHECK(listedTypes.value()[3].id == 4);
        CHECK(listedTypes.value()[3].name == "Tournament");

        SqliteDeckRepository decks(db);
        auto listedArch = decks.listByGame(magicId);
        REQUIRE(listedArch.isOk());
        REQUIRE(listedArch.value().size() == 4);
        CHECK(listedArch.value()[0].id == 1);
        CHECK(listedArch.value()[0].name == "Aggro");
        CHECK(listedArch.value()[3].name == "Combo");

        auto deck = decks.create(1, 1, "Landstill", "", "");
        REQUIRE(deck.isOk());
        SqliteGameRepository games(db);
        auto game = games.create(1, "2026-09-13", deck.value().id,
                                 deck.value().id, "", MatchResult::Win,
                                 MatchScore::TwoOne, 3, "");
        REQUIRE(game.isOk());
        CHECK(game.value().gameTypeId == 3);
    }
}
