#include "tracker/services/DataDirectoryService.hpp"

namespace tracker {

DataDirectoryService::DataDirectoryService(IFileSystem& fs,
                                           IDatabaseSession& session)
    : fs_(fs), session_(session) {}

Result<void> DataDirectoryService::activate(const std::filesystem::path& dataDir) {
    if (dataDir.empty()) {
        return Result<void>::err("Data directory must not be empty.");
    }
    auto ensured = fs_.ensureDirectory(dataDir);
    if (!ensured) return ensured;
    return session_.open(dataDir / "tracker.db");
}

}  // namespace tracker
