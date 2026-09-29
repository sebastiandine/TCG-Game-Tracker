#include <doctest/doctest.h>

#include "fakes/InMemoryGameTitleRepository.hpp"
#include "tracker/services/GameTitleService.hpp"

using namespace tracker;

TEST_SUITE("GameTitleService") {
    TEST_CASE("create trims whitespace") {
        testing::InMemoryGameTitleRepository repo;
        GameTitleService svc(repo);

        auto result = svc.create("  Magic: The Gathering  ");
        REQUIRE(result.isOk());
        CHECK(result.value().name == "Magic: The Gathering");
    }

    TEST_CASE("create rejects empty name") {
        testing::InMemoryGameTitleRepository repo;
        GameTitleService svc(repo);

        auto result = svc.create("");
        CHECK(result.isErr());
        CHECK(result.error() == "Game name must not be empty.");
    }

    TEST_CASE("create rejects whitespace-only name") {
        testing::InMemoryGameTitleRepository repo;
        GameTitleService svc(repo);

        auto result = svc.create("   ");
        CHECK(result.isErr());
        CHECK(result.error() == "Game name must not be empty.");
    }

    TEST_CASE("create delegates to repository") {
        testing::InMemoryGameTitleRepository repo;
        GameTitleService svc(repo);

        auto r1 = svc.create("Magic: The Gathering");
        REQUIRE(r1.isOk());
        CHECK(r1.value().id == 1);

        auto r2 = svc.create("Lorcana");
        REQUIRE(r2.isOk());
        CHECK(r2.value().id == 2);

        auto list = svc.listAll();
        REQUIRE(list.isOk());
        CHECK(list.value().size() == 2);
        CHECK(list.value()[0].name == "Magic: The Gathering");
        CHECK(list.value()[1].name == "Lorcana");
    }

    TEST_CASE("create rejects duplicate name") {
        testing::InMemoryGameTitleRepository repo;
        GameTitleService svc(repo);

        auto r1 = svc.create("Lorcana");
        REQUIRE(r1.isOk());

        auto r2 = svc.create("lorcana");
        CHECK(r2.isErr());
    }
}
