#include "tracker/domain/Configuration.hpp"

namespace tracker {

void to_json(nlohmann::json& j, const Configuration& c) {
    j = nlohmann::json{
        {"dataStorage", c.dataStorage},
        {"theme", c.theme},
        {"selectedGameId", c.selectedGameId},
        {"selectedFormatId", c.selectedFormatId},
    };
}

void from_json(const nlohmann::json& j, Configuration& c) {
    j.at("dataStorage").get_to(c.dataStorage);
    c.theme = j.value("theme", Theme::Light);
    c.selectedGameId = j.value("selectedGameId", std::int64_t{0});
    c.selectedFormatId = j.value("selectedFormatId", std::int64_t{0});
}

}  // namespace tracker
