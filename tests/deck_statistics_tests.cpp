#include <doctest/doctest.h>

#include "tracker/domain/DeckStatistics.hpp"

#include <vector>

using namespace tracker;

namespace {

Deck makeDeck(std::int64_t id, const char* name, const char* variant = "",
              std::int64_t archetypeId = 1) {
    Deck d;
    d.id = id;
    d.formatId = 1;
    d.archetypeId = archetypeId;
    d.name = name;
    d.variant = variant;
    return d;
}

Game makeGame(std::int64_t deckId, MatchResult result,
              std::int64_t opponentDeckId = 99) {
    Game g;
    g.formatId = 1;
    g.playedOn = "2026-09-13";
    g.deckId = deckId;
    g.opponentDeckId = opponentDeckId;
    g.result = result;
    g.score = MatchScore::TwoOne;
    g.gameTypeId = 1;
    return g;
}

DeckStatsKey nameKey(const char* name) {
    DeckStatsKey key;
    key.scope = DeckStatsScope::NameAggregate;
    key.deckName = name;
    return key;
}

DeckStatsKey variantKey(std::int64_t id, const char* variant = "") {
    DeckStatsKey key;
    key.scope = DeckStatsScope::Variant;
    key.variantDeckId = id;
    key.variant = variant;
    return key;
}

const std::vector<DeckArchetype> kArchetypes{
    DeckArchetype{1, 1, "Aggro"},
    DeckArchetype{2, 1, "Midrange"},
    DeckArchetype{3, 1, "Control"},
    DeckArchetype{4, 1, "Combo"},
};

}  // namespace

TEST_SUITE("MatchRecord") {
    TEST_CASE("win percentage is wins over total including draws") {
        MatchRecord rec{2, 1, 1};
        REQUIRE(rec.total() == 4);
        REQUIRE(rec.winPercentage().has_value());
        CHECK(*rec.winPercentage() == doctest::Approx(50.0));
    }

    TEST_CASE("win percentage is empty when there are no games") {
        CHECK_FALSE(MatchRecord{}.winPercentage().has_value());
        CHECK(MatchRecord{}.total() == 0);
    }
}

TEST_SUITE("computeDeckStatistics") {
    TEST_CASE("empty decks and games returns empty output") {
        CHECK(computeDeckStatistics({}, {}).empty());
    }

    TEST_CASE("games on a single core deck produce one leaf group") {
        const auto decks = std::vector<Deck>{makeDeck(1, "Jund")};
        const auto games = std::vector<Game>{
            makeGame(1, MatchResult::Win),
            makeGame(1, MatchResult::Loss),
        };

        const auto rows = computeDeckStatistics(decks, games);
        REQUIRE(rows.size() == 1);
        CHECK(rows[0].name == "Jund");
        REQUIRE(rows[0].variants.size() == 1);
        CHECK(rows[0].variants[0].deck.id == 1);
        CHECK(rows[0].aggregate == MatchRecord{1, 1, 0});
        CHECK(rows[0].variants[0].record == rows[0].aggregate);
    }

    TEST_CASE("core plus variants aggregate on the parent and stay separate") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Landstill"),
            makeDeck(2, "Landstill", "Glaciers"),
            makeDeck(3, "Landstill", "Mage/Angel"),
        };
        const auto games = std::vector<Game>{
            makeGame(1, MatchResult::Win),
            makeGame(1, MatchResult::Win),
            makeGame(1, MatchResult::Loss),
            makeGame(2, MatchResult::Win),
            makeGame(2, MatchResult::Draw),
            makeGame(3, MatchResult::Loss),
        };

        const auto rows = computeDeckStatistics(decks, games);
        REQUIRE(rows.size() == 1);
        CHECK(rows[0].name == "Landstill");
        REQUIRE(rows[0].variants.size() == 3);
        CHECK(rows[0].variants[0].deck.variant.empty());
        CHECK(rows[0].variants[1].deck.variant == "Glaciers");
        CHECK(rows[0].variants[2].deck.variant == "Mage/Angel");
        CHECK(rows[0].variants[0].record == MatchRecord{2, 1, 0});
        CHECK(rows[0].variants[1].record == MatchRecord{1, 0, 1});
        CHECK(rows[0].variants[2].record == MatchRecord{0, 1, 0});
        CHECK(rows[0].aggregate == MatchRecord{3, 2, 1});
        CHECK(rows[0].aggregate.wins ==
              rows[0].variants[0].record.wins +
                  rows[0].variants[1].record.wins +
                  rows[0].variants[2].record.wins);
        CHECK(rows[0].aggregate.losses ==
              rows[0].variants[0].record.losses +
                  rows[0].variants[1].record.losses +
                  rows[0].variants[2].record.losses);
        CHECK(rows[0].aggregate.draws ==
              rows[0].variants[0].record.draws +
                  rows[0].variants[1].record.draws +
                  rows[0].variants[2].record.draws);
    }

    TEST_CASE("unplayed sibling variant is kept with a zero record") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Landstill"),
            makeDeck(2, "Landstill", "Glaciers"),
        };
        const auto games = std::vector<Game>{makeGame(2, MatchResult::Win)};

        const auto rows = computeDeckStatistics(decks, games);
        REQUIRE(rows.size() == 1);
        REQUIRE(rows[0].variants.size() == 2);
        CHECK(rows[0].variants[0].deck.variant.empty());
        CHECK(rows[0].variants[0].record == MatchRecord{});
        CHECK(rows[0].variants[1].deck.variant == "Glaciers");
        CHECK(rows[0].variants[1].record == MatchRecord{1, 0, 0});
        CHECK(rows[0].aggregate == MatchRecord{1, 0, 0});
    }

    TEST_CASE("never-played deck name is omitted") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Jund"),
            makeDeck(2, "Shops"),
        };
        const auto games = std::vector<Game>{makeGame(1, MatchResult::Win)};

        const auto rows = computeDeckStatistics(decks, games);
        REQUIRE(rows.size() == 1);
        CHECK(rows[0].name == "Jund");
    }

    TEST_CASE("opponent-only games do not count") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Jund"),
            makeDeck(2, "Shops"),
        };
        const auto games = std::vector<Game>{
            makeGame(1, MatchResult::Win, /*opponentDeckId=*/2),
        };

        const auto rows = computeDeckStatistics(decks, games);
        REQUIRE(rows.size() == 1);
        CHECK(rows[0].name == "Jund");
        CHECK(rows[0].aggregate == MatchRecord{1, 0, 0});
    }

    TEST_CASE("grouping is case-insensitive") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Jund"),
            makeDeck(2, "jund", "Saga"),
        };
        const auto games = std::vector<Game>{
            makeGame(1, MatchResult::Win),
            makeGame(2, MatchResult::Loss),
        };

        const auto rows = computeDeckStatistics(decks, games);
        REQUIRE(rows.size() == 1);
        REQUIRE(rows[0].variants.size() == 2);
        CHECK(rows[0].aggregate == MatchRecord{1, 1, 0});
    }
}

TEST_SUITE("computeDeckDetailStatistics") {
    TEST_CASE("name aggregate includes core and variant games") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Landstill"),
            makeDeck(2, "Landstill", "Glaciers"),
            makeDeck(10, "Shops", "", 1),
        };
        const auto games = std::vector<Game>{
            makeGame(1, MatchResult::Win, 10),
            makeGame(1, MatchResult::Win, 10),
            makeGame(2, MatchResult::Loss, 10),
        };

        const auto family = computeDeckDetailStatistics(
            decks, kArchetypes, games,
            nameKey("Landstill"));
        CHECK(family.title == "Landstill");
        CHECK(family.overview == MatchRecord{2, 1, 0});

        const auto variant = computeDeckDetailStatistics(
            decks, kArchetypes, games,
            variantKey(2));
        CHECK(variant.title == "Landstill \xE2\x80\x94 Glaciers");
        CHECK(variant.overview == MatchRecord{0, 1, 0});
        CHECK(variant.overview.total() < family.overview.total());
    }

    TEST_CASE("matchups group by opponent archetype and deck name") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Landstill"),
            makeDeck(10, "Shops", "", 1),
            makeDeck(11, "Jund", "", 2),
            makeDeck(12, "Tron", "", 3),
        };
        const auto games = std::vector<Game>{
            makeGame(1, MatchResult::Win, 10),
            makeGame(1, MatchResult::Win, 10),
            makeGame(1, MatchResult::Loss, 11),
            makeGame(1, MatchResult::Draw, 12),
        };

        const auto stats = computeDeckDetailStatistics(
            decks, kArchetypes, games,
            nameKey("Landstill"));
        CHECK(stats.overview == MatchRecord{2, 1, 1});

        REQUIRE(stats.vsArchetypes.size() == 3);
        CHECK(stats.vsArchetypes[0].label == "Aggro");
        CHECK(stats.vsArchetypes[0].record == MatchRecord{2, 0, 0});
        CHECK(stats.vsArchetypes[1].label == "Control");
        CHECK(stats.vsArchetypes[1].record == MatchRecord{0, 0, 1});
        CHECK(stats.vsArchetypes[2].label == "Midrange");
        CHECK(stats.vsArchetypes[2].record == MatchRecord{0, 1, 0});

        REQUIRE(stats.vsDecks.size() == 3);
        CHECK(stats.vsDecks[0].label == "Jund");
        CHECK(stats.vsDecks[0].record == MatchRecord{0, 1, 0});
        CHECK(stats.vsDecks[1].label == "Shops");
        CHECK(stats.vsDecks[1].record == MatchRecord{2, 0, 0});
        CHECK(stats.vsDecks[2].label == "Tron");
        CHECK(stats.vsDecks[2].record == MatchRecord{0, 0, 1});
    }

    TEST_CASE("best and worst matchups rank by win percentage") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Jund"),
            makeDeck(10, "Alpha", "", 1),
            makeDeck(11, "Bravo", "", 1),
            makeDeck(12, "Charlie", "", 1),
            makeDeck(13, "Delta", "", 1),
            makeDeck(14, "Echo", "", 1),
            makeDeck(15, "Foxtrot", "", 1),
        };
        const auto games = std::vector<Game>{
            makeGame(1, MatchResult::Win, 10),   // Alpha 100%
            makeGame(1, MatchResult::Win, 11),   // Bravo 100%
            makeGame(1, MatchResult::Win, 11),
            makeGame(1, MatchResult::Win, 12),   // Charlie 50%
            makeGame(1, MatchResult::Loss, 12),
            makeGame(1, MatchResult::Loss, 13),  // Delta 0%
            makeGame(1, MatchResult::Loss, 14),  // Echo 0%
            makeGame(1, MatchResult::Loss, 14),
            makeGame(1, MatchResult::Win, 15),   // Foxtrot 33.3%
            makeGame(1, MatchResult::Loss, 15),
            makeGame(1, MatchResult::Loss, 15),
        };

        const auto stats = computeDeckDetailStatistics(
            decks, kArchetypes, games,
            nameKey("Jund"));
        REQUIRE(stats.vsDecks.size() == 6);

        REQUIRE(stats.bestMatchups.size() == 5);
        CHECK(stats.bestMatchups[0].label == "Bravo");   // 100%, 2 games
        CHECK(stats.bestMatchups[1].label == "Alpha");   // 100%, 1 game
        CHECK(stats.bestMatchups[2].label == "Charlie"); // 50%
        CHECK(stats.bestMatchups[3].label == "Foxtrot"); // 33.3%
        CHECK(stats.bestMatchups[4].label == "Echo");    // 0%, 2 games

        REQUIRE(stats.worstMatchups.size() == 5);
        CHECK(stats.worstMatchups[0].label == "Echo");    // 0%, 2 games
        CHECK(stats.worstMatchups[1].label == "Delta");   // 0%, 1 game
        CHECK(stats.worstMatchups[2].label == "Foxtrot");
        CHECK(stats.worstMatchups[3].label == "Charlie");
        CHECK(stats.worstMatchups[4].label == "Bravo");   // 100%, more games than Alpha
    }

    TEST_CASE("fewer than five matchups appear in both best and worst") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Jund"),
            makeDeck(10, "Shops", "", 1),
            makeDeck(11, "Tron", "", 3),
        };
        const auto games = std::vector<Game>{
            makeGame(1, MatchResult::Win, 10),
            makeGame(1, MatchResult::Loss, 11),
        };

        const auto stats = computeDeckDetailStatistics(
            decks, kArchetypes, games,
            nameKey("Jund"));
        REQUIRE(stats.bestMatchups.size() == 2);
        REQUIRE(stats.worstMatchups.size() == 2);
        CHECK(stats.bestMatchups[0].label == "Shops");
        CHECK(stats.bestMatchups[1].label == "Tron");
        CHECK(stats.worstMatchups[0].label == "Tron");
        CHECK(stats.worstMatchups[1].label == "Shops");
    }

    TEST_CASE("unresolved opponent is omitted from matchups but counted in overview") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Jund"),
            makeDeck(10, "Shops", "", 1),
        };
        const auto games = std::vector<Game>{
            makeGame(1, MatchResult::Win, 10),
            makeGame(1, MatchResult::Loss, 99),
        };

        const auto stats = computeDeckDetailStatistics(
            decks, kArchetypes, games,
            nameKey("Jund"));
        CHECK(stats.overview == MatchRecord{1, 1, 0});
        REQUIRE(stats.vsDecks.size() == 1);
        CHECK(stats.vsDecks[0].label == "Shops");
        REQUIRE(stats.vsArchetypes.size() == 1);
        CHECK(stats.vsArchetypes[0].label == "Aggro");
    }

    TEST_CASE("unknown name or variant id returns empty statistics") {
        const auto decks = std::vector<Deck>{makeDeck(1, "Jund")};
        const auto games = std::vector<Game>{makeGame(1, MatchResult::Win, 1)};

        const auto missingName = computeDeckDetailStatistics(
            decks, kArchetypes, games,
            nameKey("Missing"));
        CHECK(missingName.title.empty());
        CHECK(missingName.overview == MatchRecord{});
        CHECK(missingName.vsDecks.empty());

        const auto missingId = computeDeckDetailStatistics(
            decks, kArchetypes, games,
            variantKey(42));
        CHECK(missingId.title.empty());
        CHECK(missingId.overview == MatchRecord{});
    }

    TEST_CASE("name aggregate matches case-insensitively") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Jund"),
            makeDeck(2, "jund", "Saga"),
            makeDeck(10, "Shops", "", 1),
        };
        const auto games = std::vector<Game>{
            makeGame(1, MatchResult::Win, 10),
            makeGame(2, MatchResult::Loss, 10),
        };

        const auto stats = computeDeckDetailStatistics(
            decks, kArchetypes, games,
            nameKey("JUND"));
        CHECK(stats.title == "Jund");
        CHECK(stats.overview == MatchRecord{1, 1, 0});
    }

    TEST_CASE("open notes collect non-empty player-side notes newest first") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Jund"),
            makeDeck(10, "Shops", "", 1),
            makeDeck(11, "Tron", "", 3),
        };
        Game older = makeGame(1, MatchResult::Win, 10);
        older.id = 1;
        older.playedOn = "2026-09-01";
        older.notes = "boarded out Thoughtseize";
        Game empty = makeGame(1, MatchResult::Loss, 11);
        empty.id = 2;
        empty.playedOn = "2026-09-10";
        Game newer = makeGame(1, MatchResult::Win, 11);
        newer.id = 3;
        newer.playedOn = "2026-09-10";
        newer.notes = "keep Duress";
        Game sameDayOlderId = makeGame(1, MatchResult::Draw, 10);
        sameDayOlderId.id = 4;
        sameDayOlderId.playedOn = "2026-09-10";
        sameDayOlderId.notes = "split games";

        const auto stats = computeDeckDetailStatistics(
            decks, kArchetypes, {older, empty, newer, sameDayOlderId},
            nameKey("Jund"));
        REQUIRE(stats.openNotes.size() == 3);
        CHECK(stats.openNotes[0].game.id == 4);
        CHECK(stats.openNotes[0].opponentDeck == "Shops");
        CHECK(stats.openNotes[0].game.notes == "split games");
        CHECK(stats.openNotes[1].game.id == 3);
        CHECK(stats.openNotes[1].opponentDeck == "Tron");
        CHECK(stats.openNotes[1].game.notes == "keep Duress");
        CHECK(stats.openNotes[2].game.id == 1);
        CHECK(stats.openNotes[2].opponentDeck == "Shops");
        CHECK(stats.openNotes[2].game.notes == "boarded out Thoughtseize");
    }

    TEST_CASE("open notes omit other decks and empty notes") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Jund"),
            makeDeck(2, "Shops"),
            makeDeck(10, "Tron", "", 3),
        };
        Game own = makeGame(1, MatchResult::Win, 10);
        own.id = 1;
        own.notes = "jund note";
        Game other = makeGame(2, MatchResult::Win, 10);
        other.id = 2;
        other.notes = "shops note";
        Game blank = makeGame(1, MatchResult::Loss, 10);
        blank.id = 3;
        blank.notes = "";

        const auto stats = computeDeckDetailStatistics(
            decks, kArchetypes, {own, other, blank},
            nameKey("Jund"));
        REQUIRE(stats.openNotes.size() == 1);
        CHECK(stats.openNotes[0].game.id == 1);
        CHECK(stats.openNotes[0].game.notes == "jund note");
    }

    TEST_CASE("open notes family includes variants and variant scope is narrow") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Landstill"),
            makeDeck(2, "Landstill", "Glaciers"),
            makeDeck(10, "Shops", "", 1),
        };
        Game core = makeGame(1, MatchResult::Win, 10);
        core.id = 1;
        core.notes = "core note";
        Game variant = makeGame(2, MatchResult::Loss, 10);
        variant.id = 2;
        variant.notes = "glacier note";

        const auto family = computeDeckDetailStatistics(
            decks, kArchetypes, {core, variant},
            nameKey("Landstill"));
        REQUIRE(family.openNotes.size() == 2);

        const auto onlyVariant = computeDeckDetailStatistics(
            decks, kArchetypes, {core, variant},
            variantKey(2));
        REQUIRE(onlyVariant.openNotes.size() == 1);
        CHECK(onlyVariant.openNotes[0].game.id == 2);
        CHECK(onlyVariant.openNotes[0].game.notes == "glacier note");
    }

    TEST_CASE("open notes keep unresolved opponents as Unknown deck") {
        const auto decks = std::vector<Deck>{makeDeck(1, "Jund")};
        Game missingOpp = makeGame(1, MatchResult::Win, 99);
        missingOpp.id = 7;
        missingOpp.notes = "still useful";

        const auto stats = computeDeckDetailStatistics(
            decks, kArchetypes, {missingOpp},
            nameKey("Jund"));
        CHECK(stats.vsDecks.empty());
        REQUIRE(stats.openNotes.size() == 1);
        CHECK(stats.openNotes[0].opponentDeck == "Unknown deck");
        CHECK(stats.openNotes[0].game.notes == "still useful");
    }
}

TEST_SUITE("computeFormatStatistics") {
    TEST_CASE("empty input is zeros") {
        const auto stats = computeFormatStatistics({}, {});
        CHECK(stats.overview == MatchRecord{});
        CHECK(stats.decksPlayed == 0);
        CHECK(stats.uniqueOpponents == 0);
        CHECK(stats.streak == 0);
        CHECK(stats.bestDecks.empty());
        CHECK(stats.worstDecks.empty());
        CHECK(stats.mostPlayed.empty());
    }

    TEST_CASE("aggregates player-side games and counts played names") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Jund"),
            makeDeck(2, "Jund", "Saga"),
            makeDeck(3, "Shops"),
        };
        const auto games = std::vector<Game>{
            makeGame(1, MatchResult::Win, 3),
            makeGame(2, MatchResult::Loss, 3),
            makeGame(3, MatchResult::Win, 1),
        };

        const auto stats = computeFormatStatistics(decks, games);
        CHECK(stats.overview == MatchRecord{2, 1, 0});
        CHECK(stats.decksPlayed == 2);
        REQUIRE(stats.bestDecks.size() == 2);
        CHECK(stats.bestDecks[0].label == "Shops");
        CHECK(stats.bestDecks[1].label == "Jund");
    }

    TEST_CASE("unique opponents are case-insensitive and skip blanks") {
        const auto decks = std::vector<Deck>{makeDeck(1, "Jund")};
        Game a = makeGame(1, MatchResult::Win);
        a.opponent = "Alice";
        Game b = makeGame(1, MatchResult::Loss);
        b.opponent = "alice";
        Game c = makeGame(1, MatchResult::Win);
        c.opponent = "Bob";
        Game d = makeGame(1, MatchResult::Draw);
        d.opponent = "";

        const auto stats = computeFormatStatistics(decks, {a, b, c, d});
        CHECK(stats.uniqueOpponents == 2);
        CHECK(stats.overview == MatchRecord{2, 1, 1});
    }

    TEST_CASE("streak counts consecutive newest results") {
        const auto decks = std::vector<Deck>{makeDeck(1, "Jund")};
        Game older = makeGame(1, MatchResult::Loss);
        older.id = 1;
        older.playedOn = "2026-09-01";
        Game mid = makeGame(1, MatchResult::Win);
        mid.id = 2;
        mid.playedOn = "2026-09-10";
        Game newest = makeGame(1, MatchResult::Win);
        newest.id = 3;
        newest.playedOn = "2026-09-10";

        CHECK(computeFormatStatistics(decks, {older, mid, newest}).streak == 2);

        Game draw = makeGame(1, MatchResult::Draw);
        draw.id = 4;
        draw.playedOn = "2026-09-11";
        CHECK(computeFormatStatistics(decks, {older, mid, newest, draw}).streak ==
              0);

        Game loss = makeGame(1, MatchResult::Loss);
        loss.id = 5;
        loss.playedOn = "2026-09-12";
        Game loss2 = makeGame(1, MatchResult::Loss);
        loss2.id = 6;
        loss2.playedOn = "2026-09-13";
        CHECK(computeFormatStatistics(decks, {loss, loss2}).streak == -2);
    }

    TEST_CASE("most played ranks by games then name") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Jund"),
            makeDeck(2, "Shops"),
            makeDeck(3, "Tron"),
        };
        const auto games = std::vector<Game>{
            makeGame(1, MatchResult::Win, 2),
            makeGame(1, MatchResult::Loss, 2),
            makeGame(1, MatchResult::Loss, 3),
            makeGame(2, MatchResult::Win, 1),
            makeGame(3, MatchResult::Win, 1),
            makeGame(3, MatchResult::Win, 1),
        };

        const auto stats = computeFormatStatistics(decks, games);
        REQUIRE(stats.mostPlayed.size() == 3);
        CHECK(stats.mostPlayed[0].label == "Jund");
        CHECK(stats.mostPlayed[0].record.total() == 3);
        CHECK(stats.mostPlayed[1].label == "Tron");
        CHECK(stats.mostPlayed[2].label == "Shops");
        REQUIRE(stats.bestDecks.size() == 3);
        CHECK(stats.bestDecks[0].label == "Tron");
        CHECK(stats.worstDecks[0].label == "Jund");
    }

    TEST_CASE("opponent-only games are ignored") {
        const auto decks = std::vector<Deck>{
            makeDeck(1, "Jund"),
            makeDeck(2, "Shops"),
        };
        Game onlyOpp = makeGame(99, MatchResult::Win, 1);
        onlyOpp.opponent = "Alice";

        const auto stats = computeFormatStatistics(decks, {onlyOpp});
        CHECK(stats.overview == MatchRecord{});
        CHECK(stats.decksPlayed == 0);
        CHECK(stats.uniqueOpponents == 0);
        CHECK(stats.streak == 0);
    }
}
