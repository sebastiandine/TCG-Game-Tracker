#include <doctest/doctest.h>

#include "tracker/infra/SqliteDatabase.hpp"
#include "tracker/infra/SqliteDeckRepository.hpp"
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
}
