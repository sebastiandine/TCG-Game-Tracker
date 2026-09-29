#pragma once

// AppContext: the only thing that crosses the UI boundary. Holds references
// to the shared core services. The wxWidgets layer never sees a concrete
// adapter type - swap in a Qt/imgui frontend by reimplementing the consumers
// of this struct only.

#include "tracker/services/ConfigService.hpp"
#include "tracker/services/DeckService.hpp"
#include "tracker/services/FormatService.hpp"
#include "tracker/services/GameImportService.hpp"
#include "tracker/services/GameService.hpp"
#include "tracker/services/GameTitleService.hpp"
#include "tracker/services/GameTypeService.hpp"

namespace tracker::ui {

struct AppContext {
    ConfigService&     config;
    GameTitleService&  gameTitles;
    FormatService&     formats;
    DeckService&       decks;
    GameTypeService&   gameTypes;
    GameService&       games;
    GameImportService& gameImport;
};

}  // namespace tracker::ui
