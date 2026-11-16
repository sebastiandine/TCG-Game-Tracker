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

void seedDefaultArchetypes(SqliteDatabase& db, std::int64_t gameId) {
    SqliteDeckRepository repo(db);
    REQUIRE(repo.createArchetype(gameId, "Aggro").isOk());
    REQUIRE(repo.createArchetype(gameId, "Midrange").isOk());
    REQUIRE(repo.createArchetype(gameId, "Control").isOk());
    REQUIRE(repo.createArchetype(gameId, "Combo").isOk());
}

}  // namespace

TEST_SUITE("SqliteDeckRepository") {
    TEST_CASE("new databases do not seed archetypes") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        SqliteDeckRepository repo(db);

        auto result = repo.listByGame(gameId);
        REQUIRE(result.isOk());
        CHECK(result.value().empty());
    }

    TEST_CASE("createArchetype returns consecutive ids and isolates games") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto magicId = seedGame(db, "Magic");
        const auto lorcanaId = seedGame(db, "Lorcana");
        SqliteDeckRepository repo(db);

        auto a1 = repo.createArchetype(magicId, "Aggro");
        REQUIRE(a1.isOk());
        CHECK(a1.value().id == 1);
        CHECK(a1.value().gameId == magicId);

        auto a2 = repo.createArchetype(lorcanaId, "Aggro");
        REQUIRE(a2.isOk());
        CHECK(a2.value().gameId == lorcanaId);

        auto dup = repo.createArchetype(magicId, "aggro");
        CHECK(dup.isErr());

        auto magic = repo.listByGame(magicId);
        REQUIRE(magic.isOk());
        CHECK(magic.value().size() == 1);
        auto lorcana = repo.listByGame(lorcanaId);
        REQUIRE(lorcana.isOk());
        CHECK(lorcana.value().size() == 1);
    }

    TEST_CASE("create returns deck with assigned id") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultArchetypes(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository repo(db);

        auto fmt = formats.create(gameId, "Vintage");
        REQUIRE(fmt.isOk());

        auto r1 = repo.create(fmt.value().id, 1, "Landstill", "", "");
        REQUIRE(r1.isOk());
        CHECK(r1.value().id == 1);
        CHECK(r1.value().formatId == fmt.value().id);
        CHECK(r1.value().archetypeId == 1);
        CHECK(r1.value().name == "Landstill");
        CHECK(r1.value().variant.empty());

        auto r2 = repo.create(fmt.value().id, 3, "Landstill", "Glaciers",
                              "Ice age lands variant");
        REQUIRE(r2.isOk());
        CHECK(r2.value().id == 2);
        CHECK(r2.value().variant == "Glaciers");
        CHECK(r2.value().variantNote == "Ice age lands variant");
    }

    TEST_CASE("listByFormat returns only decks for the given format") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultArchetypes(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository repo(db);

        auto vintage = formats.create(gameId, "Vintage");
        REQUIRE(vintage.isOk());
        auto legacy = formats.create(gameId, "Legacy");
        REQUIRE(legacy.isOk());

        REQUIRE(repo.create(vintage.value().id, 1, "Shops", "", "").isOk());
        REQUIRE(repo.create(legacy.value().id, 3, "Miracles", "", "").isOk());
        REQUIRE(repo.create(vintage.value().id, 4, "PO Storm", "", "").isOk());

        auto vintageDecks = repo.listByFormat(vintage.value().id);
        REQUIRE(vintageDecks.isOk());
        REQUIRE(vintageDecks.value().size() == 2);
        CHECK(vintageDecks.value()[0].name == "PO Storm");
        CHECK(vintageDecks.value()[1].name == "Shops");

        auto legacyDecks = repo.listByFormat(legacy.value().id);
        REQUIRE(legacyDecks.isOk());
        REQUIRE(legacyDecks.value().size() == 1);
        CHECK(legacyDecks.value()[0].name == "Miracles");
    }

    TEST_CASE("create rejects duplicate format+name+variant") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultArchetypes(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository repo(db);

        auto fmt = formats.create(gameId, "Modern");
        REQUIRE(fmt.isOk());

        REQUIRE(repo.create(fmt.value().id, 2, "Jund", "", "").isOk());

        auto dup = repo.create(fmt.value().id, 2, "jund", "", "");
        CHECK(dup.isErr());
        CHECK(dup.error().find("already exists") != std::string::npos);
    }

    TEST_CASE("same name allowed with different variant") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultArchetypes(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository repo(db);

        auto fmt = formats.create(gameId, "Legacy");
        REQUIRE(fmt.isOk());

        REQUIRE(repo.create(fmt.value().id, 3, "Landstill", "", "").isOk());
        auto r2 = repo.create(fmt.value().id, 3, "Landstill", "Glaciers", "");
        CHECK(r2.isOk());
    }

    TEST_CASE("create rejects invalid format FK") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteDeckRepository repo(db);

        auto result = repo.create(9999, 1, "Fake", "", "");
        CHECK(result.isErr());
        CHECK(result.error().find("Invalid format or archetype") !=
              std::string::npos);
    }

    TEST_CASE("create rejects invalid archetype FK") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultArchetypes(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository repo(db);

        auto fmt = formats.create(gameId, "Standard");
        REQUIRE(fmt.isOk());

        auto result = repo.create(fmt.value().id, 9999, "Fake", "", "");
        CHECK(result.isErr());
        CHECK(result.error().find("Invalid format or archetype") !=
              std::string::npos);
    }

    TEST_CASE("listByFormat on empty table returns empty vector") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteDeckRepository repo(db);

        auto result = repo.listByFormat(1);
        REQUIRE(result.isOk());
        CHECK(result.value().empty());
    }

    TEST_CASE("update modifies an existing deck") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultArchetypes(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository repo(db);

        auto fmt = formats.create(gameId, "Vintage");
        REQUIRE(fmt.isOk());

        auto created = repo.create(fmt.value().id, 1, "Shops", "", "");
        REQUIRE(created.isOk());

        auto updated = repo.update(created.value().id, 3, "Stax", "Prison",
                                   "Smokestack variant");
        REQUIRE(updated.isOk());
        CHECK(updated.value().id == created.value().id);
        CHECK(updated.value().name == "Stax");
        CHECK(updated.value().variant == "Prison");
        CHECK(updated.value().variantNote == "Smokestack variant");
        CHECK(updated.value().archetypeId == 3);
        CHECK(updated.value().formatId == fmt.value().id);
    }

    TEST_CASE("update rejects duplicate name+variant") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultArchetypes(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository repo(db);

        auto fmt = formats.create(gameId, "Legacy");
        REQUIRE(fmt.isOk());

        REQUIRE(repo.create(fmt.value().id, 1, "Jund", "", "").isOk());
        auto second = repo.create(fmt.value().id, 2, "Miracles", "", "");
        REQUIRE(second.isOk());

        auto result = repo.update(second.value().id, 2, "Jund", "", "");
        CHECK(result.isErr());
        CHECK(result.error().find("already exists") != std::string::npos);
    }

    TEST_CASE("update returns error for nonexistent deck") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        SqliteDeckRepository repo(db);

        auto result = repo.update(9999, 1, "Fake", "", "");
        CHECK(result.isErr());
        CHECK(result.error().find("not found") != std::string::npos);
    }

    TEST_CASE("renameFamily updates every row and preserves variants") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultArchetypes(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository repo(db);

        auto fmt = formats.create(gameId, "Legacy");
        REQUIRE(fmt.isOk());
        auto main = repo.create(fmt.value().id, 1, "Landstill", "", "core");
        auto glaciers = repo.create(fmt.value().id, 3, "Landstill", "Glaciers",
                                    "ice");
        auto other = repo.create(fmt.value().id, 2, "Shops", "", "");
        REQUIRE(main.isOk());
        REQUIRE(glaciers.isOk());
        REQUIRE(other.isOk());

        auto renamed = repo.renameFamily(fmt.value().id, "landstill",
                                         "Standstill");
        REQUIRE(renamed.isOk());

        auto list = repo.listByFormat(fmt.value().id);
        REQUIRE(list.isOk());
        REQUIRE(list.value().size() == 3);
        for (const auto& deck : list.value()) {
            if (deck.id == other.value().id) {
                CHECK(deck.name == "Shops");
            } else if (deck.id == main.value().id) {
                CHECK(deck.name == "Standstill");
                CHECK(deck.variant.empty());
                CHECK(deck.variantNote == "core");
                CHECK(deck.archetypeId == 1);
            } else {
                CHECK(deck.name == "Standstill");
                CHECK(deck.variant == "Glaciers");
                CHECK(deck.variantNote == "ice");
            }
        }
    }

    TEST_CASE("renameFamily collision changes nothing") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultArchetypes(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository repo(db);

        auto fmt = formats.create(gameId, "Legacy");
        REQUIRE(fmt.isOk());
        REQUIRE(repo.create(fmt.value().id, 1, "Landstill", "", "").isOk());
        REQUIRE(repo.create(fmt.value().id, 1, "Landstill", "Glaciers", "")
                    .isOk());
        REQUIRE(repo.create(fmt.value().id, 2, "Standstill", "", "").isOk());

        auto renamed = repo.renameFamily(fmt.value().id, "Landstill",
                                         "Standstill");
        CHECK(renamed.isErr());
        CHECK(renamed.error().find("already exists") != std::string::npos);

        auto list = repo.listByFormat(fmt.value().id);
        REQUIRE(list.isOk());
        int landstill = 0;
        int standstill = 0;
        for (const auto& deck : list.value()) {
            if (deck.name == "Landstill") {
                ++landstill;
                CHECK((deck.variant.empty() || deck.variant == "Glaciers"));
            }
            if (deck.name == "Standstill") ++standstill;
        }
        CHECK(landstill == 2);
        CHECK(standstill == 1);
    }

    TEST_CASE("update of one variant leaves sibling rows unchanged") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultArchetypes(db, gameId);
        SqliteFormatRepository formats(db);
        SqliteDeckRepository repo(db);

        auto fmt = formats.create(gameId, "Legacy");
        REQUIRE(fmt.isOk());
        auto main = repo.create(fmt.value().id, 1, "Landstill", "", "");
        auto glaciers = repo.create(fmt.value().id, 1, "Landstill", "Glaciers",
                                    "");
        REQUIRE(main.isOk());
        REQUIRE(glaciers.isOk());

        auto updated = repo.update(glaciers.value().id, 1, "Landstill",
                                   "Beasts", "");
        REQUIRE(updated.isOk());
        CHECK(updated.value().variant == "Beasts");

        auto listedMain = repo.findById(main.value().id);
        REQUIRE(listedMain.isOk());
        CHECK(listedMain.value().name == "Landstill");
        CHECK(listedMain.value().variant.empty());
    }

    TEST_CASE("remove deletes games that reference the deck and leaves others") {
        SqliteDatabase db;
        REQUIRE(db.openMemory().isOk());
        const auto gameId = seedGame(db);
        seedDefaultArchetypes(db, gameId);
        SqliteGameTypeRepository types(db);
        REQUIRE(types.create(gameId, "League", Competitiveness::NonCompetitive,
                             PlayMedium::Online)
                    .isOk());
        SqliteFormatRepository formats(db);
        SqliteDeckRepository decks(db);
        SqliteGameRepository games(db);

        auto fmt = formats.create(gameId, "Legacy");
        REQUIRE(fmt.isOk());
        auto main = decks.create(fmt.value().id, 1, "Landstill", "", "");
        auto glaciers = decks.create(fmt.value().id, 1, "Landstill",
                                     "Glaciers", "");
        auto shops = decks.create(fmt.value().id, 2, "Shops", "", "");
        REQUIRE(main.isOk());
        REQUIRE(glaciers.isOk());
        REQUIRE(shops.isOk());

        REQUIRE(games.create(fmt.value().id, "2026-01-01", glaciers.value().id,
                             shops.value().id, "", MatchResult::Win,
                             MatchScore::TwoOne, 1, "")
                    .isOk());
        REQUIRE(games.create(fmt.value().id, "2026-01-02", shops.value().id,
                             glaciers.value().id, "", MatchResult::Loss,
                             MatchScore::OneTwo, 1, "")
                    .isOk());
        REQUIRE(games.create(fmt.value().id, "2026-01-03", main.value().id,
                             shops.value().id, "", MatchResult::Win,
                             MatchScore::TwoZero, 1, "")
                    .isOk());
        REQUIRE(games.create(fmt.value().id, "2026-01-04", shops.value().id,
                             shops.value().id, "", MatchResult::Draw,
                             MatchScore::OneOne, 1, "")
                    .isOk());

        auto removed = decks.remove(glaciers.value().id);
        REQUIRE(removed.isOk());
        CHECK(removed.value() == 2);
        CHECK(decks.findById(glaciers.value().id).isErr());
        CHECK(decks.findById(main.value().id).isOk());

        auto remaining = games.listByFormat(fmt.value().id);
        REQUIRE(remaining.isOk());
        REQUIRE(remaining.value().size() == 2);
        for (const auto& game : remaining.value()) {
            CHECK(game.deckId != glaciers.value().id);
            CHECK(game.opponentDeckId != glaciers.value().id);
        }

        auto blocked = decks.remove(9999);
        CHECK(blocked.isErr());
        CHECK(games.listByFormat(fmt.value().id).value().size() == 2);

        auto mainRemoved = decks.remove(main.value().id);
        REQUIRE(mainRemoved.isOk());
        CHECK(mainRemoved.value() == 1);
        CHECK(decks.findById(main.value().id).isErr());
        auto left = games.listByFormat(fmt.value().id);
        REQUIRE(left.isOk());
        REQUIRE(left.value().size() == 1);
        CHECK(left.value()[0].deckId == shops.value().id);
        CHECK(left.value()[0].opponentDeckId == shops.value().id);
    }
}
