#include <doctest/doctest.h>

#include "tracker/services/ConfigService.hpp"

#include "fakes/InMemoryFileSystem.hpp"

#include <nlohmann/json.hpp>

using namespace tracker;
using tracker::testing::InMemoryFileSystem;

TEST_SUITE("ConfigService") {
    TEST_CASE("missing file is created with defaults") {
        InMemoryFileSystem fs;
        ConfigService svc{fs, "/app/config.json", "/data"};

        REQUIRE(svc.initialize().isOk());
        CHECK(svc.current().dataStorage == "/data");
        CHECK(svc.current().theme == Theme::Light);
        CHECK(svc.current().selectedGameId == 0);
        CHECK(svc.current().selectedFormatId == 0);

        const auto& written = fs.files();
        REQUIRE(written.count("/app/config.json"));
        const auto j = nlohmann::json::parse(written.at("/app/config.json"));
        CHECK(j.at("dataStorage") == "/data");
        CHECK(j.at("theme") == "Light");
        CHECK(j.at("selectedGameId") == 0);
        CHECK(j.at("selectedFormatId") == 0);
    }

    TEST_CASE("existing file is loaded verbatim") {
        InMemoryFileSystem fs;
        REQUIRE(fs.writeText("/app/config.json",
            R"({"dataStorage":"/somewhere","theme":"Dark"})").isOk());

        ConfigService svc{fs, "/app/config.json", "/default"};
        REQUIRE(svc.initialize().isOk());
        CHECK(svc.current().dataStorage == "/somewhere");
        CHECK(svc.current().theme == Theme::Dark);
    }

    TEST_CASE("store updates both the live config and the file") {
        InMemoryFileSystem fs;
        ConfigService svc{fs, "/app/config.json", "/data"};
        REQUIRE(svc.initialize().isOk());

        Configuration next;
        next.dataStorage = "/new/place";
        next.theme = Theme::Dark;
        next.selectedGameId = 3;
        next.selectedFormatId = 7;
        REQUIRE(svc.store(next).isOk());

        CHECK(svc.current() == next);
        const auto j = nlohmann::json::parse(fs.files().at("/app/config.json"));
        CHECK(j.at("dataStorage") == "/new/place");
        CHECK(j.at("theme") == "Dark");
        CHECK(j.at("selectedGameId") == 3);
        CHECK(j.at("selectedFormatId") == 7);
    }

    TEST_CASE("missing theme field defaults to light for compatibility") {
        InMemoryFileSystem fs;
        REQUIRE(fs.writeText("/app/config.json",
            R"({"dataStorage":"/old"})").isOk());

        ConfigService svc{fs, "/app/config.json", "/default"};
        REQUIRE(svc.initialize().isOk());
        CHECK(svc.current().theme == Theme::Light);
    }

    TEST_CASE("missing selectedFormatId defaults to 0 for compatibility") {
        InMemoryFileSystem fs;
        REQUIRE(fs.writeText("/app/config.json",
            R"({"dataStorage":"/old","theme":"Dark"})").isOk());

        ConfigService svc{fs, "/app/config.json", "/default"};
        REQUIRE(svc.initialize().isOk());
        CHECK(svc.current().selectedFormatId == 0);
    }

    TEST_CASE("missing selectedGameId defaults to 0 for compatibility") {
        InMemoryFileSystem fs;
        REQUIRE(fs.writeText("/app/config.json",
            R"({"dataStorage":"/old","theme":"Dark"})").isOk());

        ConfigService svc{fs, "/app/config.json", "/default"};
        REQUIRE(svc.initialize().isOk());
        CHECK(svc.current().selectedGameId == 0);
    }

    TEST_CASE("malformed JSON returns an error") {
        InMemoryFileSystem fs;
        REQUIRE(fs.writeText("/app/config.json", "not json at all").isOk());

        ConfigService svc{fs, "/app/config.json", "/default"};
        auto result = svc.initialize();
        CHECK(result.isErr());
    }
}
