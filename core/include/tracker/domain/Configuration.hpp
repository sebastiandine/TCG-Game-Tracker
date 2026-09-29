#pragma once

// Configuration: matches the persisted `config.json` schema used by this app.
//
//   { "dataStorage": "/abs/path", "theme": "Light",
//     "selectedGameId": 0, "selectedFormatId": 0 }

#include "tracker/domain/Enums.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace tracker {

struct Configuration {
    std::string  dataStorage;
    Theme        theme{Theme::Light};
    std::int64_t selectedGameId{0};
    std::int64_t selectedFormatId{0};

    friend bool operator==(const Configuration&, const Configuration&) = default;
};

void to_json(nlohmann::json& j, const Configuration& c);
void from_json(const nlohmann::json& j, Configuration& c);

}  // namespace tracker
