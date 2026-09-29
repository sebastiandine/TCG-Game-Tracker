#pragma once

// DataDirectoryService: ensure a data directory exists and open its tracker.db.

#include "tracker/ports/IDatabaseSession.hpp"
#include "tracker/ports/IFileSystem.hpp"
#include "tracker/util/Result.hpp"

#include <filesystem>

namespace tracker {

class DataDirectoryService {
public:
    DataDirectoryService(IFileSystem& fs, IDatabaseSession& session);

    // Create `dataDir` if needed, then open `{dataDir}/tracker.db`.
    // Fails if `dataDir` is empty.
    [[nodiscard]] Result<void> activate(const std::filesystem::path& dataDir);

private:
    IFileSystem& fs_;
    IDatabaseSession& session_;
};

}  // namespace tracker
