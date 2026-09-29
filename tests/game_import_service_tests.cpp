#include <doctest/doctest.h>

#include "fakes/InMemoryDeckRepository.hpp"
#include "fakes/InMemoryGameRepository.hpp"
#include "fakes/InMemoryGameTypeRepository.hpp"
#include "tracker/import/GameImportParse.hpp"
#include "tracker/ports/IGameImportSink.hpp"
#include "tracker/services/DeckService.hpp"
#include "tracker/services/GameImportService.hpp"
#include "tracker/services/GameService.hpp"
#include "tracker/services/GameTypeService.hpp"

#include <string>
#include <unordered_map>
#include <vector>

using namespace tracker;

namespace {

GameImportRow makeRow(std::size_t line,
                      const std::string& date,
                      const std::string& deck,
                      const std::string& variant,
                      const std::string& oppDeck,
                      MatchScore score,
                      const std::string& event,
                      const std::string& opponent = "",
                      const std::string& notes = "") {
    GameImportRow row;
    row.lineNumber = line;
    row.playedOn = date;
    row.deckName = deck;
    row.variant = variant;
    row.opponentDeckName = oppDeck;
    row.score = score;
    row.result = resultForScore(score);
    row.eventName = event;
    row.opponent = opponent;
    row.notes = notes;
    return row;
}

class ScriptedImportSink final : public IGameImportSink {
public:
    std::int64_t defaultArchetypeId{2};
    GameTypeChoice defaultType{};
    bool continueProgress{true};
    std::vector<std::string> archetypeNames;
    std::vector<std::string> gameTypeNames;
    std::unordered_map<std::string, std::int64_t> archetypeByName;
    std::unordered_map<std::string, GameTypeChoice> typeByName;
    std::unordered_map<std::string, std::string> cancelArchetype;
    std::unordered_map<std::string, std::string> cancelGameType;

    Result<std::int64_t> chooseArchetype(const std::string& deckName) override {
        archetypeNames.push_back(deckName);
        auto cancel = cancelArchetype.find(deckName);
        if (cancel != cancelArchetype.end()) {
            return Result<std::int64_t>::err(cancel->second);
        }
        auto named = archetypeByName.find(deckName);
        if (named != archetypeByName.end()) {
            return Result<std::int64_t>::ok(named->second);
        }
        return Result<std::int64_t>::ok(defaultArchetypeId);
    }

    Result<GameTypeChoice> chooseGameType(const std::string& eventName) override {
        gameTypeNames.push_back(eventName);
        auto cancel = cancelGameType.find(eventName);
        if (cancel != cancelGameType.end()) {
            return Result<GameTypeChoice>::err(cancel->second);
        }
        auto named = typeByName.find(eventName);
        if (named != typeByName.end()) {
            return Result<GameTypeChoice>::ok(named->second);
        }
        return Result<GameTypeChoice>::ok(defaultType);
    }

    bool onProgress(std::size_t, std::size_t) override {
        return continueProgress;
    }
};

}  // namespace

TEST_SUITE("GameImportService") {
    TEST_CASE("creates missing core variant opponent and asks sink for details") {
        testing::InMemoryDeckRepository decksRepo;
        decksRepo.seedArchetypes();
        testing::InMemoryGameTypeRepository typesRepo;
        testing::InMemoryGameRepository gamesRepo;
        DeckService decks(decksRepo);
        GameTypeService types(typesRepo);
        GameService games(gamesRepo);
        GameImportService svc(decks, types, games);

        ScriptedImportSink sink;
        sink.defaultArchetypeId = 2;
        sink.defaultType = {Competitiveness::NonCompetitive, PlayMedium::Online};

        auto row = makeRow(2, "2026-01-06", "Oath Ponza", "GR Oath Ponza",
                           "Sligh", MatchScore::TwoZero, "MTGO Practice",
                           "Clifford J", "note");
        auto report = svc.import(1, 1, {row}, sink);
        REQUIRE(report.isOk());
        CHECK(report.value().gamesImported == 1);
        CHECK(report.value().decksCreated == 3);
        CHECK(report.value().gameTypesCreated == 1);
        CHECK(report.value().skipped.empty());
        CHECK(sink.archetypeNames.size() == 2);
        CHECK(sink.archetypeNames[0] == "Oath Ponza");
        CHECK(sink.archetypeNames[1] == "Sligh");
        CHECK(sink.gameTypeNames == std::vector<std::string>{"MTGO Practice"});

        auto listedDecks = decks.listByFormat(1);
        REQUIRE(listedDecks.isOk());
        REQUIRE(listedDecks.value().size() == 3);

        std::int64_t playerVariantId = 0;
        std::int64_t oathCoreId = 0;
        std::int64_t slighId = 0;
        for (const auto& d : listedDecks.value()) {
            CHECK(d.archetypeId == 2);
            if (d.name == "Oath Ponza" && d.variant.empty()) oathCoreId = d.id;
            if (d.name == "Oath Ponza" && d.variant == "GR Oath Ponza") {
                playerVariantId = d.id;
            }
            if (d.name == "Sligh" && d.variant.empty()) slighId = d.id;
        }
        CHECK(playerVariantId != 0);
        CHECK(oathCoreId != 0);
        CHECK(slighId != 0);

        auto listedTypes = types.listByGame(1);
        REQUIRE(listedTypes.isOk());
        REQUIRE(listedTypes.value().size() == 1);
        CHECK(listedTypes.value()[0].name == "MTGO Practice");
        CHECK(listedTypes.value()[0].competitiveness ==
              Competitiveness::NonCompetitive);
        CHECK(listedTypes.value()[0].medium == PlayMedium::Online);

        auto listedGames = games.listByFormat(1);
        REQUIRE(listedGames.isOk());
        REQUIRE(listedGames.value().size() == 1);
        CHECK(listedGames.value()[0].deckId == playerVariantId);
        CHECK(listedGames.value()[0].opponentDeckId == slighId);
        CHECK(listedGames.value()[0].opponent == "Clifford J");
        CHECK(listedGames.value()[0].notes == "note");
        CHECK(listedGames.value()[0].score == MatchScore::TwoZero);
        CHECK(listedGames.value()[0].result == MatchResult::Win);
        CHECK(listedGames.value()[0].playedOn == "2026-01-06");
    }

    TEST_CASE("empty variante uses core and opponent is always core") {
        testing::InMemoryDeckRepository decksRepo;
        decksRepo.seedArchetypes();
        testing::InMemoryGameTypeRepository typesRepo;
        testing::InMemoryGameRepository gamesRepo;
        DeckService decks(decksRepo);
        GameTypeService types(typesRepo);
        GameService games(gamesRepo);
        GameImportService svc(decks, types, games);

        REQUIRE(decks.create(1, 1, "Enchantress", "Stax", "").isOk());

        ScriptedImportSink sink;
        sink.defaultArchetypeId = 4;
        auto row = makeRow(2, "2026-01-08", "Enchantress", "", "Enchantress",
                           MatchScore::ZeroTwo, "League");
        auto report = svc.import(1, 1, {row}, sink);
        REQUIRE(report.isOk());
        CHECK(report.value().gamesImported == 1);
        CHECK(report.value().decksCreated == 1);
        CHECK(sink.archetypeNames == std::vector<std::string>{"Enchantress"});

        auto listedDecks = decks.listByFormat(1);
        REQUIRE(listedDecks.isOk());
        std::int64_t coreId = 0;
        std::int64_t staxId = 0;
        for (const auto& d : listedDecks.value()) {
            if (d.variant.empty()) coreId = d.id;
            if (d.variant == "Stax") staxId = d.id;
        }
        auto listedGames = games.listByFormat(1);
        REQUIRE(listedGames.isOk());
        CHECK(listedGames.value()[0].deckId == coreId);
        CHECK(listedGames.value()[0].opponentDeckId == coreId);
        CHECK(listedGames.value()[0].deckId != staxId);
    }

    TEST_CASE("reuses existing names case-insensitively without prompting") {
        testing::InMemoryDeckRepository decksRepo;
        decksRepo.seedArchetypes();
        testing::InMemoryGameTypeRepository typesRepo;
        testing::InMemoryGameRepository gamesRepo;
        DeckService decks(decksRepo);
        GameTypeService types(typesRepo);
        GameService games(gamesRepo);
        GameImportService svc(decks, types, games);

        REQUIRE(decks.create(1, 3, "Landstill", "", "").isOk());
        REQUIRE(types.create(1, "FNM", Competitiveness::NonCompetitive,
                             PlayMedium::Paper).isOk());

        ScriptedImportSink sink;
        auto row = makeRow(2, "2026-01-09", "landstill", "", "landstill",
                           MatchScore::TwoOne, "fnm");
        auto report = svc.import(1, 1, {row}, sink);
        REQUIRE(report.isOk());
        CHECK(report.value().decksCreated == 0);
        CHECK(report.value().gameTypesCreated == 0);
        CHECK(report.value().gamesImported == 1);
        CHECK(sink.archetypeNames.empty());
        CHECK(sink.gameTypeNames.empty());

        auto listedTypes = types.listByGame(1);
        REQUIRE(listedTypes.isOk());
        CHECK(listedTypes.value().size() == 1);
        CHECK(listedTypes.value()[0].medium == PlayMedium::Paper);
    }

    TEST_CASE("asks once per new core deck and uses chosen event details") {
        testing::InMemoryDeckRepository decksRepo;
        decksRepo.seedArchetypes();
        testing::InMemoryGameTypeRepository typesRepo;
        testing::InMemoryGameRepository gamesRepo;
        DeckService decks(decksRepo);
        GameTypeService types(typesRepo);
        GameService games(gamesRepo);
        GameImportService svc(decks, types, games);

        ScriptedImportSink sink;
        sink.archetypeByName["A"] = 1;
        sink.archetypeByName["B"] = 3;
        sink.typeByName["League"] = {Competitiveness::Competitive,
                                     PlayMedium::Paper};

        auto r1 = makeRow(2, "2026-01-10", "A", "", "B", MatchScore::TwoZero,
                          "League");
        auto r2 = makeRow(3, "2026-01-11", "A", "v2", "B", MatchScore::TwoOne,
                          "League");
        auto report = svc.import(1, 1, {r1, r2}, sink);
        REQUIRE(report.isOk());
        CHECK(report.value().gamesImported == 2);
        CHECK(sink.archetypeNames.size() == 2);
        CHECK(sink.gameTypeNames.size() == 1);

        auto listedDecks = decks.listByFormat(1);
        REQUIRE(listedDecks.isOk());
        for (const auto& d : listedDecks.value()) {
            if (d.name == "A") CHECK(d.archetypeId == 1);
            if (d.name == "B") CHECK(d.archetypeId == 3);
        }
        auto listedTypes = types.listByGame(1);
        REQUIRE(listedTypes.isOk());
        REQUIRE(listedTypes.value().size() == 1);
        CHECK(listedTypes.value()[0].competitiveness ==
              Competitiveness::Competitive);
        CHECK(listedTypes.value()[0].medium == PlayMedium::Paper);
    }

    TEST_CASE("cancelled deck prompt skips later rows for that name") {
        testing::InMemoryDeckRepository decksRepo;
        decksRepo.seedArchetypes();
        testing::InMemoryGameTypeRepository typesRepo;
        testing::InMemoryGameRepository gamesRepo;
        DeckService decks(decksRepo);
        GameTypeService types(typesRepo);
        GameService games(gamesRepo);
        GameImportService svc(decks, types, games);

        ScriptedImportSink sink;
        sink.cancelArchetype["A"] = "Skipped creating deck \"A\".";
        auto r1 = makeRow(2, "2026-01-10", "A", "", "B", MatchScore::TwoZero,
                          "FNM");
        auto r2 = makeRow(3, "2026-01-11", "A", "", "B", MatchScore::TwoZero,
                          "FNM");
        auto report = svc.import(1, 1, {r1, r2}, sink);
        REQUIRE(report.isOk());
        CHECK(report.value().gamesImported == 0);
        CHECK(sink.archetypeNames.size() == 1);
        REQUIRE(report.value().skipped.size() == 2);
        CHECK(report.value().skipped[0].lineNumber == 2);
        CHECK(report.value().skipped[1].lineNumber == 3);
    }

    TEST_CASE("skips a bad row and continues") {
        testing::InMemoryDeckRepository decksRepo;
        decksRepo.seedArchetypes();
        testing::InMemoryGameTypeRepository typesRepo;
        testing::InMemoryGameRepository gamesRepo;
        DeckService decks(decksRepo);
        GameTypeService types(typesRepo);
        GameService games(gamesRepo);
        GameImportService svc(decks, types, games);

        ScriptedImportSink sink;
        auto bad = makeRow(2, "not-a-date", "A", "", "B", MatchScore::TwoZero,
                           "FNM");
        auto good = makeRow(3, "2026-01-10", "A", "", "B", MatchScore::TwoZero,
                            "FNM");
        auto report = svc.import(1, 1, {bad, good}, sink);
        REQUIRE(report.isOk());
        CHECK(report.value().gamesImported == 1);
        REQUIRE(report.value().skipped.size() == 1);
        CHECK(report.value().skipped[0].lineNumber == 2);
    }

    TEST_CASE("progress cancel keeps already imported rows") {
        testing::InMemoryDeckRepository decksRepo;
        decksRepo.seedArchetypes();
        testing::InMemoryGameTypeRepository typesRepo;
        testing::InMemoryGameRepository gamesRepo;
        DeckService decks(decksRepo);
        GameTypeService types(typesRepo);
        GameService games(gamesRepo);
        GameImportService svc(decks, types, games);

        class StopAfterFirst final : public IGameImportSink {
        public:
            Result<std::int64_t> chooseArchetype(const std::string&) override {
                return Result<std::int64_t>::ok(1);
            }
            Result<GameTypeChoice> chooseGameType(const std::string&) override {
                return Result<GameTypeChoice>::ok({});
            }
            bool onProgress(std::size_t current, std::size_t) override {
                return current == 0;
            }
        } sink;

        auto r1 = makeRow(2, "2026-01-10", "A", "", "B", MatchScore::TwoZero,
                          "FNM");
        auto r2 = makeRow(3, "2026-01-11", "A", "", "B", MatchScore::TwoZero,
                          "FNM");
        auto report = svc.import(1, 1, {r1, r2}, sink);
        REQUIRE(report.isOk());
        CHECK(report.value().aborted);
        CHECK(report.value().gamesImported == 1);
    }

    TEST_CASE("rejects missing format") {
        testing::InMemoryDeckRepository decksRepo;
        decksRepo.seedArchetypes();
        testing::InMemoryGameTypeRepository typesRepo;
        testing::InMemoryGameRepository gamesRepo;
        DeckService decks(decksRepo);
        GameTypeService types(typesRepo);
        GameService games(gamesRepo);
        GameImportService svc(decks, types, games);

        ScriptedImportSink sink;
        auto row = makeRow(2, "2026-01-10", "A", "", "B", MatchScore::TwoZero,
                           "FNM");
        CHECK(svc.import(0, 1, {row}, sink).isErr());
        CHECK(svc.import(1, 0, {row}, sink).isErr());
    }

    TEST_CASE("infers archetype from Aggro Control Combo Midrange in the name") {
        testing::InMemoryDeckRepository decksRepo;
        decksRepo.seedArchetypes();
        testing::InMemoryGameTypeRepository typesRepo;
        testing::InMemoryGameRepository gamesRepo;
        DeckService decks(decksRepo);
        GameTypeService types(typesRepo);
        GameService games(gamesRepo);
        GameImportService svc(decks, types, games);

        ScriptedImportSink sink;
        auto aggro = makeRow(2, "2026-01-10", "Mono Black Aggro", "",
                             "Dimir Control", MatchScore::TwoZero, "FNM");
        auto combo = makeRow(3, "2026-01-11", "Storm Combo", "",
                             "Jund midrange", MatchScore::TwoOne, "FNM");
        auto report = svc.import(1, 1, {aggro, combo}, sink);
        REQUIRE(report.isOk());
        CHECK(report.value().gamesImported == 2);
        CHECK(sink.archetypeNames.empty());

        auto listedDecks = decks.listByFormat(1);
        REQUIRE(listedDecks.isOk());
        REQUIRE(listedDecks.value().size() == 4);
        for (const auto& d : listedDecks.value()) {
            if (d.name == "Mono Black Aggro") CHECK(d.archetypeId == 1);
            if (d.name == "Dimir Control") CHECK(d.archetypeId == 3);
            if (d.name == "Storm Combo") CHECK(d.archetypeId == 4);
            if (d.name == "Jund midrange") CHECK(d.archetypeId == 2);
        }
    }
}
