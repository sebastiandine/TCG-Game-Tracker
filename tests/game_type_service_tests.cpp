#include <doctest/doctest.h>

#include "fakes/InMemoryGameTypeRepository.hpp"
#include "tracker/services/GameTypeService.hpp"

using namespace tracker;

TEST_SUITE("GameTypeService") {
    TEST_CASE("create trims whitespace") {
        testing::InMemoryGameTypeRepository repo;
        GameTypeService svc(repo);

        auto result = svc.create(1, "  FNM  ", Competitiveness::NonCompetitive,
                                 PlayMedium::Paper);
        REQUIRE(result.isOk());
        CHECK(result.value().name == "FNM");
        CHECK(result.value().gameId == 1);
    }

    TEST_CASE("create rejects missing game") {
        testing::InMemoryGameTypeRepository repo;
        GameTypeService svc(repo);

        auto result = svc.create(0, "FNM", Competitiveness::Competitive,
                                 PlayMedium::Online);
        CHECK(result.isErr());
        CHECK(result.error() == "No game selected.");
    }

    TEST_CASE("create rejects empty name") {
        testing::InMemoryGameTypeRepository repo;
        GameTypeService svc(repo);

        auto result = svc.create(1, "", Competitiveness::Competitive,
                                 PlayMedium::Online);
        CHECK(result.isErr());
        CHECK(result.error() == "Game type name must not be empty.");
    }

    TEST_CASE("create rejects whitespace-only name") {
        testing::InMemoryGameTypeRepository repo;
        GameTypeService svc(repo);

        auto result = svc.create(1, "   ", Competitiveness::Competitive,
                                 PlayMedium::Paper);
        CHECK(result.isErr());
        CHECK(result.error() == "Game type name must not be empty.");
    }

    TEST_CASE("create delegates to repository") {
        testing::InMemoryGameTypeRepository repo;
        GameTypeService svc(repo);

        auto r1 = svc.create(1, "FNM", Competitiveness::NonCompetitive,
                             PlayMedium::Paper);
        REQUIRE(r1.isOk());
        CHECK(r1.value().id == 1);
        CHECK(r1.value().competitiveness == Competitiveness::NonCompetitive);
        CHECK(r1.value().medium == PlayMedium::Paper);

        auto r2 = svc.create(1, "Tournament", Competitiveness::Competitive,
                             PlayMedium::Paper);
        REQUIRE(r2.isOk());
        CHECK(r2.value().id == 2);

        auto list = svc.listByGame(1);
        REQUIRE(list.isOk());
        CHECK(list.value().size() == 2);
        CHECK(list.value()[0].name == "FNM");
        CHECK(list.value()[1].name == "Tournament");
    }

    TEST_CASE("create rejects duplicate name in the same game") {
        testing::InMemoryGameTypeRepository repo;
        GameTypeService svc(repo);

        auto r1 = svc.create(1, "FNM", Competitiveness::NonCompetitive,
                             PlayMedium::Paper);
        REQUIRE(r1.isOk());

        auto r2 = svc.create(1, "fnm", Competitiveness::Competitive,
                             PlayMedium::Online);
        CHECK(r2.isErr());
    }

    TEST_CASE("same game type name is allowed in different games") {
        testing::InMemoryGameTypeRepository repo;
        GameTypeService svc(repo);

        auto r1 = svc.create(1, "Tournament", Competitiveness::Competitive,
                             PlayMedium::Paper);
        REQUIRE(r1.isOk());

        auto r2 = svc.create(2, "Tournament", Competitiveness::NonCompetitive,
                             PlayMedium::Online);
        REQUIRE(r2.isOk());
        CHECK(r2.value().gameId == 2);

        auto g1 = svc.listByGame(1);
        REQUIRE(g1.isOk());
        CHECK(g1.value().size() == 1);

        auto g2 = svc.listByGame(2);
        REQUIRE(g2.isOk());
        CHECK(g2.value().size() == 1);
        CHECK(g2.value()[0].medium == PlayMedium::Online);
    }
}
