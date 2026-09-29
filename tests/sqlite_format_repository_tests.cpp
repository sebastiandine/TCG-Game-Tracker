#include <doctest/doctest.h>

#include "tracker/infra/SqliteDatabase.hpp"
#include "tracker/infra/SqliteFormatRepository.hpp"
#include "tracker/infra/SqliteGameTitleRepository.hpp"

using namespace tracker;

namespace {

std::int64_t seedGame(SqliteDatabase& db, const char* name = "Test Game") {
    SqliteGameTitleRepository titles(db);
    auto g = titles.create(name);
    REQUIRE(g.isOk());
    return g.value().id;
}

}  // namespace

TEST_SUITE("SqliteFormatRepository") {
    TEST_CASE("create returns format with consecutive ids") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        SqliteFormatRepository repo(db);

        auto r1 = repo.create(gameId, "Standard");
        REQUIRE(r1.isOk());
        CHECK(r1.value().id == 1);
        CHECK(r1.value().gameId == gameId);
        CHECK(r1.value().name == "Standard");

        auto r2 = repo.create(gameId, "Commander");
        REQUIRE(r2.isOk());
        CHECK(r2.value().id == 2);
        CHECK(r2.value().name == "Commander");
    }

    TEST_CASE("listByGame returns formats ordered by id") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        SqliteFormatRepository repo(db);

        REQUIRE(repo.create(gameId, "Commander").isOk());
        REQUIRE(repo.create(gameId, "Standard").isOk());
        REQUIRE(repo.create(gameId, "Modern").isOk());

        auto result = repo.listByGame(gameId);
        REQUIRE(result.isOk());
        REQUIRE(result.value().size() == 3);
        CHECK(result.value()[0].id == 1);
        CHECK(result.value()[0].name == "Commander");
        CHECK(result.value()[1].id == 2);
        CHECK(result.value()[1].name == "Standard");
        CHECK(result.value()[2].id == 3);
        CHECK(result.value()[2].name == "Modern");
    }

    TEST_CASE("listByGame isolates games") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto magicId = seedGame(db, "Magic");
        const auto lorcanaId = seedGame(db, "Lorcana");
        SqliteFormatRepository repo(db);

        REQUIRE(repo.create(magicId, "Standard").isOk());
        REQUIRE(repo.create(lorcanaId, "Standard").isOk());
        REQUIRE(repo.create(magicId, "Modern").isOk());

        auto magic = repo.listByGame(magicId);
        REQUIRE(magic.isOk());
        REQUIRE(magic.value().size() == 2);
        CHECK(magic.value()[0].name == "Standard");
        CHECK(magic.value()[1].name == "Modern");

        auto lorcana = repo.listByGame(lorcanaId);
        REQUIRE(lorcana.isOk());
        REQUIRE(lorcana.value().size() == 1);
        CHECK(lorcana.value()[0].name == "Standard");
    }

    TEST_CASE("listByGame on empty table returns empty vector") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        SqliteFormatRepository repo(db);

        auto result = repo.listByGame(gameId);
        REQUIRE(result.isOk());
        CHECK(result.value().empty());
    }

    TEST_CASE("create rejects duplicate name case-insensitive in the same game") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        SqliteFormatRepository repo(db);

        REQUIRE(repo.create(gameId, "Standard").isOk());

        auto result = repo.create(gameId, "standard");
        CHECK(result.isErr());
        CHECK(result.error().find("already exists") != std::string::npos);
    }

    TEST_CASE("create allows the same name in different games") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto magicId = seedGame(db, "Magic");
        const auto lorcanaId = seedGame(db, "Lorcana");
        SqliteFormatRepository repo(db);

        REQUIRE(repo.create(magicId, "Standard").isOk());
        auto result = repo.create(lorcanaId, "Standard");
        REQUIRE(result.isOk());
        CHECK(result.value().gameId == lorcanaId);
    }

    TEST_CASE("create rejects invalid game FK") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteFormatRepository repo(db);

        auto result = repo.create(9999, "Standard");
        CHECK(result.isErr());
        CHECK(result.error().find("Invalid") != std::string::npos);
    }

    TEST_CASE("reopen clears in-memory state") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        SqliteFormatRepository repo(db);

        REQUIRE(repo.create(gameId, "Standard").isOk());

        REQUIRE(db.openMemory().isOk());

        auto result = repo.listByGame(gameId);
        REQUIRE(result.isOk());
        CHECK(result.value().empty());
    }
}
