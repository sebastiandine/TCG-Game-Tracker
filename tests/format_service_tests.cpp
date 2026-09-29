#include <doctest/doctest.h>

#include "fakes/InMemoryFormatRepository.hpp"
#include "tracker/services/FormatService.hpp"

using namespace tracker;

TEST_SUITE("FormatService") {
    TEST_CASE("create trims whitespace") {
        testing::InMemoryFormatRepository repo;
        FormatService svc(repo);

        auto result = svc.create(1, "  Standard  ");
        REQUIRE(result.isOk());
        CHECK(result.value().name == "Standard");
        CHECK(result.value().gameId == 1);
    }

    TEST_CASE("create rejects missing game") {
        testing::InMemoryFormatRepository repo;
        FormatService svc(repo);

        auto result = svc.create(0, "Standard");
        CHECK(result.isErr());
        CHECK(result.error() == "No game selected.");
    }

    TEST_CASE("create rejects empty name") {
        testing::InMemoryFormatRepository repo;
        FormatService svc(repo);

        auto result = svc.create(1, "");
        CHECK(result.isErr());
        CHECK(result.error() == "Format name must not be empty.");
    }

    TEST_CASE("create rejects whitespace-only name") {
        testing::InMemoryFormatRepository repo;
        FormatService svc(repo);

        auto result = svc.create(1, "   ");
        CHECK(result.isErr());
        CHECK(result.error() == "Format name must not be empty.");
    }

    TEST_CASE("create delegates to repository") {
        testing::InMemoryFormatRepository repo;
        FormatService svc(repo);

        auto r1 = svc.create(1, "Standard");
        REQUIRE(r1.isOk());
        CHECK(r1.value().id == 1);

        auto r2 = svc.create(1, "Commander");
        REQUIRE(r2.isOk());
        CHECK(r2.value().id == 2);

        auto list = svc.listByGame(1);
        REQUIRE(list.isOk());
        CHECK(list.value().size() == 2);
        CHECK(list.value()[0].name == "Standard");
        CHECK(list.value()[1].name == "Commander");
    }

    TEST_CASE("create rejects duplicate name in the same game") {
        testing::InMemoryFormatRepository repo;
        FormatService svc(repo);

        auto r1 = svc.create(1, "Standard");
        REQUIRE(r1.isOk());

        auto r2 = svc.create(1, "standard");
        CHECK(r2.isErr());
    }

    TEST_CASE("same format name is allowed in different games") {
        testing::InMemoryFormatRepository repo;
        FormatService svc(repo);

        auto r1 = svc.create(1, "Standard");
        REQUIRE(r1.isOk());

        auto r2 = svc.create(2, "Standard");
        REQUIRE(r2.isOk());
        CHECK(r2.value().gameId == 2);

        auto g1 = svc.listByGame(1);
        REQUIRE(g1.isOk());
        CHECK(g1.value().size() == 1);

        auto g2 = svc.listByGame(2);
        REQUIRE(g2.isOk());
        CHECK(g2.value().size() == 1);
        CHECK(g2.value()[0].name == "Standard");
    }
}
