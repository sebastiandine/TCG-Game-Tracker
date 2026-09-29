#include "tracker/services/GameTypeService.hpp"

namespace tracker {

namespace {

std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const auto start = s.find_first_not_of(ws);
    if (start == std::string::npos) return {};
    return s.substr(start, s.find_last_not_of(ws) - start + 1);
}

}  // namespace

GameTypeService::GameTypeService(IGameTypeRepository& repo)
    : repo_(repo) {}

Result<GameType> GameTypeService::create(
        std::int64_t gameId,
        const std::string& name,
        Competitiveness competitiveness,
        PlayMedium medium) {
    if (gameId <= 0) {
        return Result<GameType>::err("No game selected.");
    }
    const std::string trimmed = trim(name);
    if (trimmed.empty()) {
        return Result<GameType>::err("Game type name must not be empty.");
    }
    return repo_.create(gameId, trimmed, competitiveness, medium);
}

Result<std::vector<GameType>> GameTypeService::listByGame(std::int64_t gameId) {
    return repo_.listByGame(gameId);
}

}  // namespace tracker
