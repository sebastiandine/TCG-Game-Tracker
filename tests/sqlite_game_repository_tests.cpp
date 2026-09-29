#include <doctest/doctest.h>

#include "tracker/infra/SqliteDatabase.hpp"
#include "tracker/infra/SqliteDeckRepository.hpp"
#include "tracker/infra/SqliteFormatRepository.hpp"
#include "tracker/infra/SqliteGameRepository.hpp"
#include "tracker/infra/SqliteGameTitleRepository.hpp"
#include "tracker/infra/SqliteGameTypeRepository.hpp"

using namespace tracker;

namespace {

std::int64_t seedGame(SqliteDatabase& db, const char* name = "Test Game") {
    SqliteGameTitleRepository titles(db);
    auto g = titles.create(name);
    REQUIRE(g.isOk());
    return g.value().id;
}

void seedDefaultLookups(SqliteDatabase& db, std::int64_t gameId) {
    SqliteDeckRepository decks(db);
    REQUIRE(decks.createArchetype(gameId, "Aggro").isOk());
    REQUIRE(decks.createArchetype(gameId, "Midrange").isOk());
    REQUIRE(decks.createArchetype(gameId, "Control").isOk());
    REQUIRE(decks.createArchetype(gameId, "Combo").isOk());

    SqliteGameTypeRepository types(db);
    REQUIRE(types.create(gameId, "Type A", Competitiveness::NonCompetitive,
                         PlayMedium::Paper).isOk());
    REQUIRE(types.create(gameId, "Type B", Competitiveness::Competitive,
                         PlayMedium::Online).isOk());
    REQUIRE(types.create(gameId, "Type C", Competitiveness::Competitive,
                         PlayMedium::Paper).isOk());
}

}  // namespace

TEST_SUITE("SqliteGameRepository") {
    TEST_CASE("create returns game with assigned id") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultLookups(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository decks(db);
        SqliteGameRepository repo(db);

        auto fmt = formats.create(gameId, "Vintage");
        REQUIRE(fmt.isOk());

        auto d1 = decks.create(fmt.value().id, 1, "Landstill", "", "");
        REQUIRE(d1.isOk());
        auto d2 = decks.create(fmt.value().id, 2, "Shops", "", "");
        REQUIRE(d2.isOk());

        auto r = repo.create(fmt.value().id, "2026-09-13",
                             d1.value().id, d2.value().id,
                             "Alice", MatchResult::Win, MatchScore::TwoOne,
                             1, "Good game");
        REQUIRE(r.isOk());
        CHECK(r.value().id == 1);
        CHECK(r.value().formatId == fmt.value().id);
        CHECK(r.value().playedOn == "2026-09-13");
        CHECK(r.value().deckId == d1.value().id);
        CHECK(r.value().opponentDeckId == d2.value().id);
        CHECK(r.value().opponent == "Alice");
        CHECK(r.value().result == MatchResult::Win);
        CHECK(r.value().score == MatchScore::TwoOne);
        CHECK(r.value().gameTypeId == 1);
        CHECK(r.value().notes == "Good game");

        auto listed = repo.listByFormat(fmt.value().id);
        REQUIRE(listed.isOk());
        REQUIRE(listed.value().size() == 1);
        CHECK(listed.value()[0].opponent == "Alice");
    }

    TEST_CASE("listByFormat returns newest first") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultLookups(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository decks(db);
        SqliteGameRepository repo(db);

        auto fmt = formats.create(gameId, "Legacy");
        REQUIRE(fmt.isOk());

        auto d1 = decks.create(fmt.value().id, 1, "A", "", "");
        REQUIRE(d1.isOk());
        auto d2 = decks.create(fmt.value().id, 2, "B", "", "");
        REQUIRE(d2.isOk());

        REQUIRE(repo.create(fmt.value().id, "2026-09-10",
                            d1.value().id, d2.value().id,
                            "", MatchResult::Win, MatchScore::TwoZero, 1, "").isOk());
        REQUIRE(repo.create(fmt.value().id, "2026-09-13",
                            d1.value().id, d2.value().id,
                            "", MatchResult::Loss, MatchScore::ZeroTwo, 2, "").isOk());
        REQUIRE(repo.create(fmt.value().id, "2026-09-11",
                            d1.value().id, d2.value().id,
                            "", MatchResult::Draw, MatchScore::OneOne, 3, "").isOk());

        auto list = repo.listByFormat(fmt.value().id);
        REQUIRE(list.isOk());
        REQUIRE(list.value().size() == 3);
        CHECK(list.value()[0].playedOn == "2026-09-13");
        CHECK(list.value()[1].playedOn == "2026-09-11");
        CHECK(list.value()[2].playedOn == "2026-09-10");
    }

    TEST_CASE("listByFormat isolates formats") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultLookups(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository decks(db);
        SqliteGameRepository repo(db);

        auto vintage = formats.create(gameId, "Vintage");
        REQUIRE(vintage.isOk());
        auto legacy = formats.create(gameId, "Legacy");
        REQUIRE(legacy.isOk());

        auto dv = decks.create(vintage.value().id, 1, "A", "", "");
        REQUIRE(dv.isOk());
        auto dv2 = decks.create(vintage.value().id, 2, "B", "", "");
        REQUIRE(dv2.isOk());

        auto dl = decks.create(legacy.value().id, 3, "C", "", "");
        REQUIRE(dl.isOk());
        auto dl2 = decks.create(legacy.value().id, 4, "D", "", "");
        REQUIRE(dl2.isOk());

        REQUIRE(repo.create(vintage.value().id, "2026-09-13",
                            dv.value().id, dv2.value().id,
                            "", MatchResult::Win, MatchScore::TwoOne, 1, "").isOk());
        REQUIRE(repo.create(legacy.value().id, "2026-09-13",
                            dl.value().id, dl2.value().id,
                            "", MatchResult::Loss, MatchScore::OneTwo, 2, "").isOk());

        auto vGames = repo.listByFormat(vintage.value().id);
        REQUIRE(vGames.isOk());
        CHECK(vGames.value().size() == 1);
        CHECK(vGames.value()[0].result == MatchResult::Win);

        auto lGames = repo.listByFormat(legacy.value().id);
        REQUIRE(lGames.isOk());
        CHECK(lGames.value().size() == 1);
        CHECK(lGames.value()[0].result == MatchResult::Loss);
    }

    TEST_CASE("create rejects invalid format FK") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteGameRepository repo(db);

        auto result = repo.create(9999, "2026-09-13", 1, 2,
                                  "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        CHECK(result.isErr());
        CHECK(result.error().find("Invalid") != std::string::npos);
    }

    TEST_CASE("create rejects invalid deck FK") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultLookups(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteGameRepository repo(db);

        auto fmt = formats.create(gameId, "Modern");
        REQUIRE(fmt.isOk());

        auto result = repo.create(fmt.value().id, "2026-09-13", 9999, 9998,
                                  "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        CHECK(result.isErr());
        CHECK(result.error().find("Invalid") != std::string::npos);
    }

    TEST_CASE("create rejects invalid game type FK") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultLookups(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository decks(db);
        SqliteGameRepository repo(db);

        auto fmt = formats.create(gameId, "Vintage");
        REQUIRE(fmt.isOk());
        auto d1 = decks.create(fmt.value().id, 1, "X", "", "");
        REQUIRE(d1.isOk());
        auto d2 = decks.create(fmt.value().id, 2, "Y", "", "");
        REQUIRE(d2.isOk());

        auto result = repo.create(fmt.value().id, "2026-09-13",
                                  d1.value().id, d2.value().id,
                                  "", MatchResult::Win, MatchScore::TwoOne,
                                  9999, "");
        CHECK(result.isErr());
        CHECK(result.error().find("Invalid") != std::string::npos);
    }

    TEST_CASE("update modifies an existing game") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultLookups(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository decks(db);
        SqliteGameRepository repo(db);

        auto fmt = formats.create(gameId, "Vintage");
        REQUIRE(fmt.isOk());

        auto d1 = decks.create(fmt.value().id, 1, "X", "", "");
        REQUIRE(d1.isOk());
        auto d2 = decks.create(fmt.value().id, 2, "Y", "", "");
        REQUIRE(d2.isOk());
        auto d3 = decks.create(fmt.value().id, 3, "Z", "", "");
        REQUIRE(d3.isOk());

        auto created = repo.create(fmt.value().id, "2026-09-13",
                                   d1.value().id, d2.value().id,
                                   "", MatchResult::Win, MatchScore::TwoOne,
                                   1, "");
        REQUIRE(created.isOk());

        auto updated = repo.update(created.value().id, "2026-09-14",
                                   d3.value().id, d1.value().id,
                                   "Dana", MatchResult::Draw, MatchScore::OneOne,
                                   2, "Updated");
        REQUIRE(updated.isOk());
        CHECK(updated.value().id == created.value().id);
        CHECK(updated.value().playedOn == "2026-09-14");
        CHECK(updated.value().deckId == d3.value().id);
        CHECK(updated.value().opponentDeckId == d1.value().id);
        CHECK(updated.value().opponent == "Dana");
        CHECK(updated.value().result == MatchResult::Draw);
        CHECK(updated.value().score == MatchScore::OneOne);
        CHECK(updated.value().gameTypeId == 2);
        CHECK(updated.value().notes == "Updated");
    }

    TEST_CASE("update returns error for nonexistent game") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteGameRepository repo(db);

        auto result = repo.update(9999, "2026-09-13", 1, 2,
                                  "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        CHECK(result.isErr());
        CHECK(result.error().find("not found") != std::string::npos);
    }

    TEST_CASE("listByFormat on empty table returns empty vector") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteGameRepository repo(db);

        auto result = repo.listByFormat(1);
        REQUIRE(result.isOk());
        CHECK(result.value().empty());
    }
}
