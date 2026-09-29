#include <doctest/doctest.h>

#include "fakes/InMemoryGameRepository.hpp"
#include "tracker/services/GameService.hpp"

using namespace tracker;

TEST_SUITE("GameService") {
    TEST_CASE("create trims notes") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto result = svc.create(1, "2026-09-13", 1, 2,
                                 "", MatchResult::Win, MatchScore::TwoOne,
                                 1, "  some notes  ");
        REQUIRE(result.isOk());
        CHECK(result.value().notes == "some notes");
    }

    TEST_CASE("create trims opponent") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto result = svc.create(1, "2026-09-13", 1, 2,
                                 "  Alice  ", MatchResult::Win, MatchScore::TwoOne,
                                 1, "");
        REQUIRE(result.isOk());
        CHECK(result.value().opponent == "Alice");
    }

    TEST_CASE("create rejects empty date") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto result = svc.create(1, "", 1, 2,
                                 "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        CHECK(result.isErr());
        CHECK(result.error().find("YYYY-MM-DD") != std::string::npos);
    }

    TEST_CASE("create rejects invalid date format") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto result = svc.create(1, "13-09-2026", 1, 2,
                                 "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        CHECK(result.isErr());
        CHECK(result.error().find("YYYY-MM-DD") != std::string::npos);
    }

    TEST_CASE("create rejects zero formatId") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto result = svc.create(0, "2026-09-13", 1, 2,
                                 "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        CHECK(result.isErr());
        CHECK(result.error() == "No format selected.");
    }

    TEST_CASE("create rejects zero deckId") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto result = svc.create(1, "2026-09-13", 0, 2,
                                 "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        CHECK(result.isErr());
        CHECK(result.error() == "A deck must be selected.");
    }

    TEST_CASE("create rejects zero opponentDeckId") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto result = svc.create(1, "2026-09-13", 1, 0,
                                 "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        CHECK(result.isErr());
        CHECK(result.error() == "An opponent deck must be selected.");
    }

    TEST_CASE("create rejects zero gameTypeId") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto result = svc.create(1, "2026-09-13", 1, 2,
                                 "", MatchResult::Win, MatchScore::TwoOne, 0, "");
        CHECK(result.isErr());
        CHECK(result.error() == "A game type must be selected.");
    }

    TEST_CASE("create happy path stores all fields") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto result = svc.create(1, "2026-09-13", 10, 20,
                                 "Bob", MatchResult::Loss, MatchScore::OneTwo,
                                 3, "Close match");
        REQUIRE(result.isOk());
        CHECK(result.value().id == 1);
        CHECK(result.value().formatId == 1);
        CHECK(result.value().playedOn == "2026-09-13");
        CHECK(result.value().deckId == 10);
        CHECK(result.value().opponentDeckId == 20);
        CHECK(result.value().opponent == "Bob");
        CHECK(result.value().result == MatchResult::Loss);
        CHECK(result.value().score == MatchScore::OneTwo);
        CHECK(result.value().gameTypeId == 3);
        CHECK(result.value().notes == "Close match");
    }

    TEST_CASE("listByFormat returns newest first") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        REQUIRE(svc.create(1, "2026-09-10", 1, 2,
                           "", MatchResult::Win, MatchScore::TwoZero, 1, "").isOk());
        REQUIRE(svc.create(1, "2026-09-13", 1, 2,
                           "", MatchResult::Loss, MatchScore::ZeroTwo, 1, "").isOk());
        REQUIRE(svc.create(1, "2026-09-11", 1, 2,
                           "", MatchResult::Draw, MatchScore::OneOne, 1, "").isOk());

        auto list = svc.listByFormat(1);
        REQUIRE(list.isOk());
        REQUIRE(list.value().size() == 3);
        CHECK(list.value()[0].playedOn == "2026-09-13");
        CHECK(list.value()[1].playedOn == "2026-09-11");
        CHECK(list.value()[2].playedOn == "2026-09-10");
    }

    TEST_CASE("listByFormat isolates formats") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        REQUIRE(svc.create(1, "2026-09-13", 1, 2,
                           "", MatchResult::Win, MatchScore::TwoOne, 1, "").isOk());
        REQUIRE(svc.create(2, "2026-09-13", 3, 4,
                           "", MatchResult::Loss, MatchScore::OneTwo, 2, "").isOk());

        auto list1 = svc.listByFormat(1);
        REQUIRE(list1.isOk());
        CHECK(list1.value().size() == 1);
        CHECK(list1.value()[0].result == MatchResult::Win);

        auto list2 = svc.listByFormat(2);
        REQUIRE(list2.isOk());
        CHECK(list2.value().size() == 1);
        CHECK(list2.value()[0].result == MatchResult::Loss);
    }

    TEST_CASE("update trims notes and validates") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto created = svc.create(1, "2026-09-13", 1, 2,
                                  "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        REQUIRE(created.isOk());

        auto updated = svc.update(created.value().id, "2026-09-14", 3, 4,
                                  "  Carol  ", MatchResult::Draw, MatchScore::OneOne,
                                  2, "  updated notes  ");
        REQUIRE(updated.isOk());
        CHECK(updated.value().playedOn == "2026-09-14");
        CHECK(updated.value().deckId == 3);
        CHECK(updated.value().opponentDeckId == 4);
        CHECK(updated.value().opponent == "Carol");
        CHECK(updated.value().result == MatchResult::Draw);
        CHECK(updated.value().score == MatchScore::OneOne);
        CHECK(updated.value().gameTypeId == 2);
        CHECK(updated.value().notes == "updated notes");
    }

    TEST_CASE("update rejects zero gameId") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto result = svc.update(0, "2026-09-13", 1, 2,
                                 "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        CHECK(result.isErr());
        CHECK(result.error() == "No game selected.");
    }

    TEST_CASE("update rejects invalid date") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto created = svc.create(1, "2026-09-13", 1, 2,
                                  "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        REQUIRE(created.isOk());

        auto result = svc.update(created.value().id, "not-a-date", 1, 2,
                                 "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        CHECK(result.isErr());
        CHECK(result.error().find("YYYY-MM-DD") != std::string::npos);
    }

    TEST_CASE("update rejects zero gameTypeId") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto created = svc.create(1, "2026-09-13", 1, 2,
                                  "", MatchResult::Win, MatchScore::TwoOne, 1, "");
        REQUIRE(created.isOk());

        auto result = svc.update(created.value().id, "2026-09-13", 1, 2,
                                 "", MatchResult::Win, MatchScore::TwoOne, 0, "");
        CHECK(result.isErr());
        CHECK(result.error() == "A game type must be selected.");
    }

    TEST_CASE("clearNotes empties notes and keeps other fields") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        auto created = svc.create(1, "2026-09-13", 10, 20,
                                  "Bob", MatchResult::Loss, MatchScore::OneTwo,
                                  3, "article fodder");
        REQUIRE(created.isOk());

        auto cleared = svc.clearNotes(created.value());
        REQUIRE(cleared.isOk());
        CHECK(cleared.value().id == created.value().id);
        CHECK(cleared.value().formatId == 1);
        CHECK(cleared.value().playedOn == "2026-09-13");
        CHECK(cleared.value().deckId == 10);
        CHECK(cleared.value().opponentDeckId == 20);
        CHECK(cleared.value().opponent == "Bob");
        CHECK(cleared.value().result == MatchResult::Loss);
        CHECK(cleared.value().score == MatchScore::OneTwo);
        CHECK(cleared.value().gameTypeId == 3);
        CHECK(cleared.value().notes.empty());

        auto list = svc.listByFormat(1);
        REQUIRE(list.isOk());
        REQUIRE(list.value().size() == 1);
        CHECK(list.value()[0].notes.empty());
        CHECK(list.value()[0].opponent == "Bob");
    }

    TEST_CASE("clearNotes rejects invalid id") {
        testing::InMemoryGameRepository repo;
        GameService svc(repo);

        Game game;
        game.id = 0;
        game.playedOn = "2026-09-13";
        game.deckId = 1;
        game.opponentDeckId = 2;
        game.gameTypeId = 1;
        auto result = svc.clearNotes(game);
        CHECK(result.isErr());
        CHECK(result.error() == "No game selected.");
    }
}
