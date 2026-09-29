#include "tracker/services/FormatService.hpp"

namespace tracker {

FormatService::FormatService(IFormatRepository& repo)
    : repo_(repo) {}

static std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

Result<Format> FormatService::create(std::int64_t gameId,
                                     const std::string& name) {
    if (gameId <= 0) {
        return Result<Format>::err("No game selected.");
    }
    const std::string trimmed = trim(name);
    if (trimmed.empty()) {
        return Result<Format>::err("Format name must not be empty.");
    }
    return repo_.create(gameId, trimmed);
}

Result<std::vector<Format>> FormatService::listByGame(std::int64_t gameId) {
    return repo_.listByGame(gameId);
}

}  // namespace tracker
