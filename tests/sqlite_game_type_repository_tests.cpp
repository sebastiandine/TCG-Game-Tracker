#include <doctest/doctest.h>

#include "tracker/infra/SqliteDatabase.hpp"
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

}  // namespace

TEST_SUITE("SqliteGameTypeRepository") {
    TEST_CASE("new databases do not seed game types") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        SqliteGameTypeRepository repo(db);

        auto result = repo.listByGame(gameId);
        REQUIRE(result.isOk());
        CHECK(result.value().empty());
    }

    TEST_CASE("create returns game type with assigned id") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        SqliteGameTypeRepository repo(db);

        auto r = repo.create(gameId, "Weekly", Competitiveness::NonCompetitive,
                             PlayMedium::Paper);
        REQUIRE(r.isOk());
        CHECK(r.value().id == 1);
        CHECK(r.value().gameId == gameId);
        CHECK(r.value().name == "Weekly");
        CHECK(r.value().competitiveness == Competitiveness::NonCompetitive);
        CHECK(r.value().medium == PlayMedium::Paper);
    }

    TEST_CASE("listByGame returns game types ordered by id") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        SqliteGameTypeRepository repo(db);

        REQUIRE(repo.create(gameId, "Weekly", Competitiveness::NonCompetitive,
                            PlayMedium::Paper).isOk());
        REQUIRE(repo.create(gameId, "Grand Prix", Competitiveness::Competitive,
                            PlayMedium::Paper).isOk());

        auto result = repo.listByGame(gameId);
        REQUIRE(result.isOk());
        REQUIRE(result.value().size() == 2);
        CHECK(result.value()[0].name == "Weekly");
        CHECK(result.value()[1].name == "Grand Prix");
    }

    TEST_CASE("listByGame isolates games") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto magicId = seedGame(db, "Magic");
        const auto lorcanaId = seedGame(db, "Lorcana");
        SqliteGameTypeRepository repo(db);

        REQUIRE(repo.create(magicId, "FNM", Competitiveness::NonCompetitive,
                            PlayMedium::Paper).isOk());
        REQUIRE(repo.create(lorcanaId, "FNM", Competitiveness::Competitive,
                            PlayMedium::Online).isOk());

        auto magic = repo.listByGame(magicId);
        REQUIRE(magic.isOk());
        REQUIRE(magic.value().size() == 1);
        CHECK(magic.value()[0].medium == PlayMedium::Paper);

        auto lorcana = repo.listByGame(lorcanaId);
        REQUIRE(lorcana.isOk());
        REQUIRE(lorcana.value().size() == 1);
        CHECK(lorcana.value()[0].competitiveness == Competitiveness::Competitive);
    }

    TEST_CASE("create rejects duplicate name case-insensitive in the same game") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        SqliteGameTypeRepository repo(db);

        REQUIRE(repo.create(gameId, "FNM", Competitiveness::NonCompetitive,
                            PlayMedium::Paper).isOk());

        auto result = repo.create(gameId, "fnm", Competitiveness::Competitive,
                                  PlayMedium::Online);
        CHECK(result.isErr());
        CHECK(result.error().find("already exists") != std::string::npos);
    }

    TEST_CASE("create allows the same name in different games") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto magicId = seedGame(db, "Magic");
        const auto lorcanaId = seedGame(db, "Lorcana");
        SqliteGameTypeRepository repo(db);

        REQUIRE(repo.create(magicId, "Tournament", Competitiveness::Competitive,
                            PlayMedium::Paper).isOk());
        auto result = repo.create(lorcanaId, "Tournament",
                                  Competitiveness::NonCompetitive,
                                  PlayMedium::Online);
        REQUIRE(result.isOk());
        CHECK(result.value().gameId == lorcanaId);
    }

    TEST_CASE("create rejects invalid game FK") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteGameTypeRepository repo(db);

        auto result = repo.create(9999, "FNM", Competitiveness::NonCompetitive,
                                  PlayMedium::Paper);
        CHECK(result.isErr());
        CHECK(result.error().find("Invalid") != std::string::npos);
    }

    TEST_CASE("reopen clears in-memory state") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        SqliteGameTypeRepository repo(db);

        REQUIRE(repo.create(gameId, "Weekly", Competitiveness::NonCompetitive,
                            PlayMedium::Paper).isOk());

        REQUIRE(db.openMemory().isOk());

        auto result = repo.listByGame(gameId);
        REQUIRE(result.isOk());
        CHECK(result.value().empty());
    }
}
