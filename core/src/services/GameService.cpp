#include "tracker/services/GameService.hpp"

#include <regex>

namespace tracker {

GameService::GameService(IGameRepository& repo)
    : repo_(repo) {}

static std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static bool isValidDate(const std::string& date) {
    // YYYY-MM-DD with basic range checks.
    static const std::regex pattern(R"(\d{4}-(0[1-9]|1[0-2])-(0[1-9]|[12]\d|3[01]))");
    return std::regex_match(date, pattern);
}

Result<Game> GameService::create(std::int64_t formatId,
                                 const std::string& playedOn,
                                 std::int64_t deckId,
                                 std::int64_t opponentDeckId,
                                 const std::string& opponent,
                                 MatchResult result,
                                 MatchScore score,
                                 std::int64_t gameTypeId,
                                 const std::string& notes) {
    if (formatId <= 0) {
        return Result<Game>::err("No format selected.");
    }
    const std::string trimmedDate = trim(playedOn);
    if (!isValidDate(trimmedDate)) {
        return Result<Game>::err("Date must be in YYYY-MM-DD format.");
    }
    if (deckId <= 0) {
        return Result<Game>::err("A deck must be selected.");
    }
    if (opponentDeckId <= 0) {
        return Result<Game>::err("An opponent deck must be selected.");
    }
    if (gameTypeId <= 0) {
        return Result<Game>::err("A game type must be selected.");
    }

    const std::string trimmedOpponent = trim(opponent);
    const std::string trimmedNotes = trim(notes);
    return repo_.create(formatId, trimmedDate, deckId, opponentDeckId,
                        trimmedOpponent, result, score, gameTypeId,
                        trimmedNotes);
}

Result<Game> GameService::update(std::int64_t gameId,
                                 const std::string& playedOn,
                                 std::int64_t deckId,
                                 std::int64_t opponentDeckId,
                                 const std::string& opponent,
                                 MatchResult result,
                                 MatchScore score,
                                 std::int64_t gameTypeId,
                                 const std::string& notes) {
    if (gameId <= 0) {
        return Result<Game>::err("No game selected.");
    }
    const std::string trimmedDate = trim(playedOn);
    if (!isValidDate(trimmedDate)) {
        return Result<Game>::err("Date must be in YYYY-MM-DD format.");
    }
    if (deckId <= 0) {
        return Result<Game>::err("A deck must be selected.");
    }
    if (opponentDeckId <= 0) {
        return Result<Game>::err("An opponent deck must be selected.");
    }
    if (gameTypeId <= 0) {
        return Result<Game>::err("A game type must be selected.");
    }

    const std::string trimmedOpponent = trim(opponent);
    const std::string trimmedNotes = trim(notes);
    return repo_.update(gameId, trimmedDate, deckId, opponentDeckId,
                        trimmedOpponent, result, score, gameTypeId,
                        trimmedNotes);
}

Result<Game> GameService::clearNotes(const Game& game) {
    return update(game.id, game.playedOn, game.deckId, game.opponentDeckId,
                  game.opponent, game.result, game.score, game.gameTypeId, "");
}

Result<std::vector<Game>> GameService::listByFormat(std::int64_t formatId) {
    return repo_.listByFormat(formatId);
}

}  // namespace tracker
