#include <doctest/doctest.h>

#include "fakes/InMemoryDeckRepository.hpp"
#include "fakes/InMemoryGameRepository.hpp"
#include "tracker/services/DeckService.hpp"

using namespace tracker;

namespace {

Game recordGame(testing::InMemoryGameRepository& games, std::int64_t deckId,
                std::int64_t opponentDeckId) {
    auto created = games.create(1, "2026-01-01", deckId, opponentDeckId, "",
                                MatchResult::Win, MatchScore::TwoOne, 1, "");
    REQUIRE(created.isOk());
    return created.value();
}

}  // namespace

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

    TEST_CASE("renaming a deck renames every variant and keeps labels") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto main = svc.create(1, 1, "Landstill", "", "core note");
        auto glaciers = svc.create(1, 3, "Landstill", "Glaciers", "ice");
        auto other = svc.create(1, 2, "Shops", "", "");
        REQUIRE(main.isOk());
        REQUIRE(glaciers.isOk());
        REQUIRE(other.isOk());

        auto updated = svc.update(glaciers.value().id, 3, "Standstill",
                                  "Glaciers", "ice");
        REQUIRE(updated.isOk());
        CHECK(updated.value().name == "Standstill");
        CHECK(updated.value().variant == "Glaciers");
        CHECK(updated.value().variantNote == "ice");

        auto list = svc.listByFormat(1);
        REQUIRE(list.isOk());
        REQUIRE(list.value().size() == 3);
        for (const auto& deck : list.value()) {
            if (deck.id == main.value().id) {
                CHECK(deck.name == "Standstill");
                CHECK(deck.variant.empty());
                CHECK(deck.variantNote == "core note");
                CHECK(deck.archetypeId == 1);
            } else if (deck.id == glaciers.value().id) {
                CHECK(deck.name == "Standstill");
                CHECK(deck.variant == "Glaciers");
            } else {
                CHECK(deck.name == "Shops");
            }
        }
    }

    TEST_CASE("renaming a variant changes only that row") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto main = svc.create(1, 1, "Landstill", "", "");
        auto glaciers = svc.create(1, 1, "Landstill", "Glaciers", "ice");
        REQUIRE(main.isOk());
        REQUIRE(glaciers.isOk());

        auto updated = svc.update(glaciers.value().id, 1, "Landstill",
                                  "Beasts", "ice");
        REQUIRE(updated.isOk());
        CHECK(updated.value().variant == "Beasts");

        auto listedMain = repo.findById(main.value().id);
        REQUIRE(listedMain.isOk());
        CHECK(listedMain.value().name == "Landstill");
        CHECK(listedMain.value().variant.empty());
    }

    TEST_CASE("family rename that collides changes nothing") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        REQUIRE(svc.create(1, 1, "Landstill", "", "").isOk());
        auto glaciers = svc.create(1, 1, "Landstill", "Glaciers", "");
        REQUIRE(glaciers.isOk());
        REQUIRE(svc.create(1, 2, "Standstill", "", "").isOk());

        auto renamed = svc.update(glaciers.value().id, 1, "Standstill",
                                  "Glaciers", "");
        CHECK(renamed.isErr());
        CHECK(renamed.error().find("already exists") != std::string::npos);

        auto list = svc.listByFormat(1);
        REQUIRE(list.isOk());
        int landstill = 0;
        int standstill = 0;
        for (const auto& deck : list.value()) {
            if (deck.name == "Landstill") ++landstill;
            if (deck.name == "Standstill") ++standstill;
        }
        CHECK(landstill == 2);
        CHECK(standstill == 1);
    }

    TEST_CASE("a colliding variant edit does not leave a partial rename") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        REQUIRE(svc.create(1, 1, "Landstill", "", "").isOk());
        auto glaciers = svc.create(1, 1, "Landstill", "Glaciers", "");
        REQUIRE(glaciers.isOk());

        auto renamed = svc.update(glaciers.value().id, 1, "Standstill", "", "");
        CHECK(renamed.isErr());

        auto list = svc.listByFormat(1);
        REQUIRE(list.isOk());
        REQUIRE(list.value().size() == 2);
        for (const auto& deck : list.value()) {
            CHECK(deck.name == "Landstill");
        }
    }

    TEST_CASE("delete variant removes only games that reference that row") {
        testing::InMemoryDeckRepository decks;
        testing::InMemoryGameRepository games;
        decks.seedArchetypes();
        decks.bindGames(&games);
        DeckService svc(decks);

        auto main = svc.create(1, 1, "Landstill", "", "");
        auto glaciers = svc.create(1, 1, "Landstill", "Glaciers", "");
        auto shops = svc.create(1, 2, "Shops", "", "");
        REQUIRE(main.isOk());
        REQUIRE(glaciers.isOk());
        REQUIRE(shops.isOk());

        recordGame(games, glaciers.value().id, shops.value().id);
        recordGame(games, main.value().id, shops.value().id);
        recordGame(games, shops.value().id, glaciers.value().id);
        recordGame(games, shops.value().id, main.value().id);

        auto removed = svc.deleteVariant(glaciers.value().id);
        REQUIRE(removed.isOk());
        CHECK(removed.value() == 2);
        CHECK(decks.findById(glaciers.value().id).isErr());
        CHECK(decks.findById(main.value().id).isOk());

        auto remaining = games.listByFormat(1);
        REQUIRE(remaining.isOk());
        REQUIRE(remaining.value().size() == 2);
        for (const auto& game : remaining.value()) {
            CHECK(game.deckId != glaciers.value().id);
            CHECK(game.opponentDeckId != glaciers.value().id);
        }
    }

    TEST_CASE("delete deck fails while a variant exists") {
        testing::InMemoryDeckRepository decks;
        testing::InMemoryGameRepository games;
        decks.seedArchetypes();
        decks.bindGames(&games);
        DeckService svc(decks);

        auto main = svc.create(1, 1, "Landstill", "", "");
        auto glaciers = svc.create(1, 1, "Landstill", "Glaciers", "");
        REQUIRE(main.isOk());
        REQUIRE(glaciers.isOk());
        recordGame(games, main.value().id, glaciers.value().id);

        auto removed = svc.deleteDeck(main.value().id);
        CHECK(removed.isErr());
        CHECK(removed.error().find("Delete the variants") != std::string::npos);
        CHECK(decks.findById(main.value().id).isOk());
        CHECK(games.listByFormat(1).value().size() == 1);

        auto asVariant = svc.deleteDeck(glaciers.value().id);
        CHECK(asVariant.isErr());
        CHECK(decks.findById(glaciers.value().id).isOk());
    }

    TEST_CASE("delete deck removes a lone main deck and its games") {
        testing::InMemoryDeckRepository decks;
        testing::InMemoryGameRepository games;
        decks.seedArchetypes();
        decks.bindGames(&games);
        DeckService svc(decks);

        auto main = svc.create(1, 1, "Landstill", "", "");
        auto shops = svc.create(1, 2, "Shops", "", "");
        REQUIRE(main.isOk());
        REQUIRE(shops.isOk());
        recordGame(games, main.value().id, shops.value().id);
        recordGame(games, shops.value().id, main.value().id);
        recordGame(games, shops.value().id, shops.value().id);

        auto removed = svc.deleteDeck(main.value().id);
        REQUIRE(removed.isOk());
        CHECK(removed.value() == 2);
        CHECK(decks.findById(main.value().id).isErr());
        CHECK(decks.findById(shops.value().id).isOk());

        auto remaining = games.listByFormat(1);
        REQUIRE(remaining.isOk());
        REQUIRE(remaining.value().size() == 1);
        CHECK(remaining.value()[0].deckId == shops.value().id);
        CHECK(remaining.value()[0].opponentDeckId == shops.value().id);
    }

    TEST_CASE("delete variant can remove the last row of a name") {
        testing::InMemoryDeckRepository decks;
        testing::InMemoryGameRepository games;
        decks.seedArchetypes();
        decks.bindGames(&games);
        DeckService svc(decks);

        auto only = svc.create(1, 1, "Landstill", "Glaciers", "");
        REQUIRE(only.isOk());
        recordGame(games, only.value().id, only.value().id);

        auto removed = svc.deleteVariant(only.value().id);
        REQUIRE(removed.isOk());
        CHECK(removed.value() == 1);
        CHECK(decks.listByFormat(1).value().empty());
        CHECK(games.listByFormat(1).value().empty());
    }

    TEST_CASE("delete variant rejects the main deck") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto main = svc.create(1, 1, "Landstill", "", "");
        REQUIRE(main.isOk());
        auto removed = svc.deleteVariant(main.value().id);
        CHECK(removed.isErr());
        CHECK(repo.findById(main.value().id).isOk());
    }

    TEST_CASE("the same deck name is allowed in another rotation") {
        testing::InMemoryDeckRepository repo;
        repo.seedArchetypes();
        DeckService svc(repo);

        auto first = svc.create(1, 1, "Burn", "", "", 10);
        REQUIRE(first.isOk());
        auto second = svc.create(1, 1, "Burn", "", "", 11);
        REQUIRE(second.isOk());
        CHECK(second.value().rotationId == 11);

        auto again = svc.create(1, 1, "burn", "", "", 10);
        CHECK(again.isErr());

        auto listed = svc.listByFormat(1, 10);
        REQUIRE(listed.isOk());
        REQUIRE(listed.value().size() == 1);
        CHECK(listed.value()[0].id == first.value().id);

        auto renamed = svc.renameFamily(1, "Burn", "Red", 10);
        REQUIRE(renamed.isOk());
        CHECK(svc.listByFormat(1, 11).value()[0].name == "Burn");
        CHECK(svc.listByFormat(1, 10).value()[0].name == "Red");
    }
}
