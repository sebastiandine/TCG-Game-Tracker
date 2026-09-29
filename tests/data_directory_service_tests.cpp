#include <doctest/doctest.h>

#include "fakes/InMemoryFileSystem.hpp"
#include "tracker/ports/IDatabaseSession.hpp"
#include "tracker/services/DataDirectoryService.hpp"

#include <filesystem>

using namespace tracker;
using tracker::testing::InMemoryFileSystem;

namespace {

class FakeDatabaseSession final : public IDatabaseSession {
public:
    Result<void> open(const std::filesystem::path& dbPath) override {
        lastOpened_ = dbPath.generic_string();
        ++openCount_;
        if (!openError_.empty()) {
            return Result<void>::err(openError_);
        }
        return Result<void>::ok();
    }

    void failOpen(std::string message) { openError_ = std::move(message); }

    [[nodiscard]] const std::string& lastOpened() const noexcept {
        return lastOpened_;
    }
    [[nodiscard]] int openCount() const noexcept { return openCount_; }

private:
    std::string lastOpened_;
    std::string openError_;
    int openCount_{0};
};

}  // namespace

TEST_SUITE("DataDirectoryService") {
    TEST_CASE("activate creates the directory and opens tracker.db") {
        InMemoryFileSystem fs;
        FakeDatabaseSession session;
        DataDirectoryService svc{fs, session};

        REQUIRE(svc.activate("/data").isOk());
        CHECK(fs.isDirectory("/data"));
        CHECK(session.lastOpened() ==
              (std::filesystem::path("/data") / "tracker.db").generic_string());
        CHECK(session.openCount() == 1);
    }

    TEST_CASE("activate rejects an empty path") {
        InMemoryFileSystem fs;
        FakeDatabaseSession session;
        DataDirectoryService svc{fs, session};

        auto result = svc.activate({});
        CHECK(result.isErr());
        CHECK(result.error() == "Data directory must not be empty.");
        CHECK(session.openCount() == 0);
    }

    TEST_CASE("activate surfaces ensureDirectory failure") {
        InMemoryFileSystem fs;
        REQUIRE(fs.writeText("/blocked", "not a directory").isOk());
        FakeDatabaseSession session;
        DataDirectoryService svc{fs, session};

        auto result = svc.activate("/blocked");
        CHECK(result.isErr());
        CHECK(session.openCount() == 0);
    }

    TEST_CASE("activate surfaces database open failure") {
        InMemoryFileSystem fs;
        FakeDatabaseSession session;
        session.failOpen("cannot open");
        DataDirectoryService svc{fs, session};

        auto result = svc.activate("/data");
        CHECK(result.isErr());
        CHECK(result.error() == "cannot open");
        CHECK(fs.isDirectory("/data"));
        CHECK(session.openCount() == 1);
    }
}
