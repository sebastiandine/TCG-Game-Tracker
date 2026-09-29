#include "tracker/services/GameTitleService.hpp"

namespace tracker {

GameTitleService::GameTitleService(IGameTitleRepository& repo)
    : repo_(repo) {}

static std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

Result<GameTitle> GameTitleService::create(const std::string& name) {
    const std::string trimmed = trim(name);
    if (trimmed.empty()) {
        return Result<GameTitle>::err("Game name must not be empty.");
    }
    return repo_.create(trimmed);
}

Result<std::vector<GameTitle>> GameTitleService::listAll() {
    return repo_.listAll();
}

}  // namespace tracker
