#include <doctest/doctest.h>

#include "tracker/infra/SqliteDatabase.hpp"
#include "tracker/infra/SqliteGameTitleRepository.hpp"

using namespace tracker;

TEST_SUITE("SqliteGameTitleRepository") {
    TEST_CASE("create returns title with consecutive ids") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteGameTitleRepository repo(db);

        auto r1 = repo.create("Magic: The Gathering");
        REQUIRE(r1.isOk());
        CHECK(r1.value().id == 1);
        CHECK(r1.value().name == "Magic: The Gathering");

        auto r2 = repo.create("Lorcana");
        REQUIRE(r2.isOk());
        CHECK(r2.value().id == 2);
        CHECK(r2.value().name == "Lorcana");
    }

    TEST_CASE("listAll returns titles ordered by id") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteGameTitleRepository repo(db);

        REQUIRE(repo.create("Lorcana").isOk());
        REQUIRE(repo.create("Magic: The Gathering").isOk());
        REQUIRE(repo.create("Flesh and Blood").isOk());

        auto result = repo.listAll();
        REQUIRE(result.isOk());
        REQUIRE(result.value().size() == 3);
        CHECK(result.value()[0].id == 1);
        CHECK(result.value()[0].name == "Lorcana");
        CHECK(result.value()[1].id == 2);
        CHECK(result.value()[1].name == "Magic: The Gathering");
        CHECK(result.value()[2].id == 3);
        CHECK(result.value()[2].name == "Flesh and Blood");
    }

    TEST_CASE("listAll on empty table returns empty vector") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteGameTitleRepository repo(db);

        auto result = repo.listAll();
        REQUIRE(result.isOk());
        CHECK(result.value().empty());
    }

    TEST_CASE("create rejects duplicate name case-insensitive") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteGameTitleRepository repo(db);

        REQUIRE(repo.create("Lorcana").isOk());

        auto result = repo.create("lorcana");
        CHECK(result.isErr());
        CHECK(result.error().find("already exists") != std::string::npos);
    }

    TEST_CASE("new databases do not seed a game title") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteGameTitleRepository repo(db);

        auto result = repo.listAll();
        REQUIRE(result.isOk());
        CHECK(result.value().empty());
    }
}
