#include <doctest/doctest.h>

#include "fakes/InMemoryDeckRepository.hpp"
#include "tracker/services/DeckService.hpp"

using namespace tracker;

TEST_SUITE("DeckService") {
    TEST_CASE("create trims name and variant") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto result = svc.create(1, 1, "  Landstill  ", "  Glaciers  ",
                                 "  some notes  ");
        REQUIRE(result.isOk());
        CHECK(result.value().name == "Landstill");
        CHECK(result.value().variant == "Glaciers");
        CHECK(result.value().variantNote == "some notes");
    }

    TEST_CASE("create rejects empty name") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto result = svc.create(1, 1, "", "", "");
        CHECK(result.isErr());
        CHECK(result.error() == "Deck name must not be empty.");
    }

    TEST_CASE("create rejects whitespace-only name") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto result = svc.create(1, 1, "   ", "", "");
        CHECK(result.isErr());
        CHECK(result.error() == "Deck name must not be empty.");
    }

    TEST_CASE("create rejects zero formatId") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto result = svc.create(0, 1, "Deck", "", "");
        CHECK(result.isErr());
        CHECK(result.error() == "No format selected.");
    }

    TEST_CASE("create rejects zero archetypeId") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto result = svc.create(1, 0, "Deck", "", "");
        CHECK(result.isErr());
        CHECK(result.error() == "An archetype must be selected.");
    }

    TEST_CASE("create delegates to repository") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto r1 = svc.create(1, 2, "Jund", "", "");
        REQUIRE(r1.isOk());
        CHECK(r1.value().id == 1);
        CHECK(r1.value().formatId == 1);
        CHECK(r1.value().archetypeId == 2);

        auto r2 = svc.create(1, 3, "Miracles", "", "");
        REQUIRE(r2.isOk());
        CHECK(r2.value().id == 2);

        auto list = svc.listByFormat(1);
        REQUIRE(list.isOk());
        CHECK(list.value().size() == 2);
    }

    TEST_CASE("create rejects duplicate") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        REQUIRE(svc.create(1, 1, "Jund", "", "").isOk());
        auto dup = svc.create(1, 1, "Jund", "", "");
        CHECK(dup.isErr());
        CHECK(dup.error().find("already exists") != std::string::npos);
    }

    TEST_CASE("listByGame returns archetypes for that game") {
        testing::InMemoryDeckRepository repo;
        DeckService svc(repo);

        REQUIRE(svc.createArchetype(1, "Aggro").isOk());
        REQUIRE(svc.createArchetype(2, "Amber").isOk());

        auto g1 = svc.listByGame(1);
        REQUIRE(g1.isOk());
        REQUIRE(g1.value().size() == 1);
        CHECK(g1.value()[0].name == "Aggro");

        auto g2 = svc.listByGame(2);
        REQUIRE(g2.isOk());
        REQUIRE(g2.value().size() == 1);
        CHECK(g2.value()[0].name == "Amber");
    }

    TEST_CASE("createArchetype rejects empty name and missing game") {
        testing::InMemoryDeckRepository repo;
        DeckService svc(repo);

        CHECK(svc.createArchetype(0, "Aggro").isErr());
        CHECK(svc.createArchetype(1, "  ").isErr());
    }

    TEST_CASE("listByFormat isolates formats") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        REQUIRE(svc.create(1, 1, "Deck A", "", "").isOk());
        REQUIRE(svc.create(2, 2, "Deck B", "", "").isOk());

        auto list1 = svc.listByFormat(1);
        REQUIRE(list1.isOk());
        REQUIRE(list1.value().size() == 1);
        CHECK(list1.value()[0].name == "Deck A");

        auto list2 = svc.listByFormat(2);
        REQUIRE(list2.isOk());
        REQUIRE(list2.value().size() == 1);
        CHECK(list2.value()[0].name == "Deck B");
    }

    TEST_CASE("update trims and validates") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto created = svc.create(1, 1, "Jund", "", "");
        REQUIRE(created.isOk());

        auto updated = svc.update(created.value().id, 3,
                                  "  Jund Updated  ", "  Saga  ", "  notes  ");
        REQUIRE(updated.isOk());
        CHECK(updated.value().name == "Jund Updated");
        CHECK(updated.value().variant == "Saga");
        CHECK(updated.value().variantNote == "notes");
        CHECK(updated.value().archetypeId == 3);
    }

    TEST_CASE("update rejects empty name") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto created = svc.create(1, 1, "Jund", "", "");
        REQUIRE(created.isOk());

        auto result = svc.update(created.value().id, 1, "", "", "");
        CHECK(result.isErr());
        CHECK(result.error() == "Deck name must not be empty.");
    }

    TEST_CASE("update rejects zero deckId") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto result = svc.update(0, 1, "Deck", "", "");
        CHECK(result.isErr());
        CHECK(result.error() == "No deck selected.");
    }
}
