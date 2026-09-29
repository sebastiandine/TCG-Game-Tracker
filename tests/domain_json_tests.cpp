#include <doctest/doctest.h>

#include "tracker/domain/Configuration.hpp"
#include "tracker/domain/Enums.hpp"

#include <nlohmann/json.hpp>

using namespace tracker;

TEST_SUITE("Theme JSON") {
    TEST_CASE("Light round-trip") {
        nlohmann::json j;
        to_json(j, Theme::Light);
        CHECK(j == "Light");
        Theme t{};
        from_json(j, t);
        CHECK(t == Theme::Light);
    }

    TEST_CASE("Dark round-trip") {
        nlohmann::json j;
        to_json(j, Theme::Dark);
        CHECK(j == "Dark");
        Theme t{};
        from_json(j, t);
        CHECK(t == Theme::Dark);
    }

    TEST_CASE("unknown theme throws") {
        nlohmann::json j = "Sepia";
        Theme t{};
        CHECK_THROWS(from_json(j, t));
    }
}

TEST_SUITE("Competitiveness JSON") {
    TEST_CASE("Competitive round-trip") {
        nlohmann::json j;
        to_json(j, Competitiveness::Competitive);
        CHECK(j == "Competitive");
        Competitiveness c{};
        from_json(j, c);
        CHECK(c == Competitiveness::Competitive);
    }

    TEST_CASE("Non-Competitive round-trip") {
        nlohmann::json j;
        to_json(j, Competitiveness::NonCompetitive);
        CHECK(j == "Non-Competitive");
        Competitiveness c{};
        from_json(j, c);
        CHECK(c == Competitiveness::NonCompetitive);
    }

    TEST_CASE("unknown competitiveness throws") {
        nlohmann::json j = "Casual";
        Competitiveness c{};
        CHECK_THROWS(from_json(j, c));
    }
}

TEST_SUITE("PlayMedium JSON") {
    TEST_CASE("Paper round-trip") {
        nlohmann::json j;
        to_json(j, PlayMedium::Paper);
        CHECK(j == "Paper");
        PlayMedium m{};
        from_json(j, m);
        CHECK(m == PlayMedium::Paper);
    }

    TEST_CASE("Online round-trip") {
        nlohmann::json j;
        to_json(j, PlayMedium::Online);
        CHECK(j == "Online");
        PlayMedium m{};
        from_json(j, m);
        CHECK(m == PlayMedium::Online);
    }

    TEST_CASE("unknown play medium throws") {
        nlohmann::json j = "Digital";
        PlayMedium m{};
        CHECK_THROWS(from_json(j, m));
    }
}

TEST_SUITE("Configuration JSON") {
    TEST_CASE("full round-trip") {
        Configuration c;
        c.dataStorage = "/some/path";
        c.theme = Theme::Dark;
        c.selectedGameId = 9;
        c.selectedFormatId = 42;

        nlohmann::json j = c;
        CHECK(j.at("dataStorage") == "/some/path");
        CHECK(j.at("theme") == "Dark");
        CHECK(j.at("selectedGameId") == 9);
        CHECK(j.at("selectedFormatId") == 42);

        auto loaded = j.get<Configuration>();
        CHECK(loaded == c);
    }

    TEST_CASE("missing theme defaults to Light") {
        nlohmann::json j = {{"dataStorage", "/data"}};
        auto c = j.get<Configuration>();
        CHECK(c.dataStorage == "/data");
        CHECK(c.theme == Theme::Light);
    }

    TEST_CASE("missing selectedFormatId defaults to 0") {
        nlohmann::json j = {{"dataStorage", "/data"}, {"theme", "Light"}};
        auto c = j.get<Configuration>();
        CHECK(c.selectedFormatId == 0);
    }

    TEST_CASE("missing selectedGameId defaults to 0") {
        nlohmann::json j = {{"dataStorage", "/data"}, {"theme", "Light"}};
        auto c = j.get<Configuration>();
        CHECK(c.selectedGameId == 0);
    }
}

TEST_SUITE("MatchResult JSON") {
    TEST_CASE("Win round-trip") {
        nlohmann::json j;
        to_json(j, MatchResult::Win);
        CHECK(j == "Win");
        MatchResult r{};
        from_json(j, r);
        CHECK(r == MatchResult::Win);
    }

    TEST_CASE("Loss round-trip") {
        nlohmann::json j;
        to_json(j, MatchResult::Loss);
        CHECK(j == "Loss");
        MatchResult r{};
        from_json(j, r);
        CHECK(r == MatchResult::Loss);
    }

    TEST_CASE("Draw round-trip") {
        nlohmann::json j;
        to_json(j, MatchResult::Draw);
        CHECK(j == "Draw");
        MatchResult r{};
        from_json(j, r);
        CHECK(r == MatchResult::Draw);
    }

    TEST_CASE("unknown match result throws") {
        nlohmann::json j = "Tie";
        MatchResult r{};
        CHECK_THROWS(from_json(j, r));
    }
}

TEST_SUITE("MatchScore JSON") {
    TEST_CASE("2-1 round-trip") {
        nlohmann::json j;
        to_json(j, MatchScore::TwoOne);
        CHECK(j == "2-1");
        MatchScore s{};
        from_json(j, s);
        CHECK(s == MatchScore::TwoOne);
    }

    TEST_CASE("2-0 round-trip") {
        nlohmann::json j;
        to_json(j, MatchScore::TwoZero);
        CHECK(j == "2-0");
        MatchScore s{};
        from_json(j, s);
        CHECK(s == MatchScore::TwoZero);
    }

    TEST_CASE("1-2 round-trip") {
        nlohmann::json j;
        to_json(j, MatchScore::OneTwo);
        CHECK(j == "1-2");
        MatchScore s{};
        from_json(j, s);
        CHECK(s == MatchScore::OneTwo);
    }

    TEST_CASE("0-2 round-trip") {
        nlohmann::json j;
        to_json(j, MatchScore::ZeroTwo);
        CHECK(j == "0-2");
        MatchScore s{};
        from_json(j, s);
        CHECK(s == MatchScore::ZeroTwo);
    }

    TEST_CASE("1-1 round-trip") {
        nlohmann::json j;
        to_json(j, MatchScore::OneOne);
        CHECK(j == "1-1");
        MatchScore s{};
        from_json(j, s);
        CHECK(s == MatchScore::OneOne);
    }

    TEST_CASE("unknown match score throws") {
        nlohmann::json j = "3-0";
        MatchScore s{};
        CHECK_THROWS(from_json(j, s));
    }

    TEST_CASE("resultForScore maps correctly") {
        CHECK(resultForScore(MatchScore::TwoOne) == MatchResult::Win);
        CHECK(resultForScore(MatchScore::TwoZero) == MatchResult::Win);
        CHECK(resultForScore(MatchScore::OneTwo) == MatchResult::Loss);
        CHECK(resultForScore(MatchScore::ZeroTwo) == MatchResult::Loss);
        CHECK(resultForScore(MatchScore::OneOne) == MatchResult::Draw);
    }
}
