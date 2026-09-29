// Composition root: builds the dependency graph from the bottom up and hands
// it to the wxWidgets UI layer. This is the only place where concrete adapter
// types are mentioned - everything downstream depends on interfaces.

#include "tracker/infra/SqliteDatabase.hpp"
#include "tracker/infra/SqliteDeckRepository.hpp"
#include "tracker/infra/SqliteFormatRepository.hpp"
#include "tracker/infra/SqliteGameRepository.hpp"
#include "tracker/infra/SqliteGameTitleRepository.hpp"
#include "tracker/infra/SqliteGameTypeRepository.hpp"
#include "tracker/infra/StdFileSystem.hpp"
#include "tracker/services/ConfigService.hpp"
#include "tracker/services/DataDirectoryService.hpp"
#include "tracker/services/DeckService.hpp"
#include "tracker/services/FormatService.hpp"
#include "tracker/services/GameImportService.hpp"
#include "tracker/services/GameService.hpp"
#include "tracker/services/GameTitleService.hpp"
#include "tracker/services/GameTypeService.hpp"
#include "tracker/ui/AppContext.hpp"
#include "tracker/ui/MainFrame.hpp"

#include <wx/app.h>
#include <wx/image.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>

#include <filesystem>
#include <memory>

class TrackerApp : public wxApp {
public:
    bool OnInit() override {
        wxImage::AddHandler(new wxPNGHandler);
        wxImage::AddHandler(new wxJPEGHandler);

        const std::filesystem::path exeDir =
            std::filesystem::path(wxStandardPaths::Get().GetExecutablePath().ToStdString())
                .parent_path();
        const auto configPath = exeDir / "config.json";
        const auto defaultDataDir = exeDir / "data";

        fs_ = std::make_unique<tracker::StdFileSystem>();
        config_ = std::make_unique<tracker::ConfigService>(*fs_, configPath, defaultDataDir);

        auto initResult = config_->initialize();
        if (!initResult) {
            wxMessageBox(
                wxString::Format("Failed to load configuration: %s",
                    wxString::FromUTF8(initResult.error().c_str())),
                "Startup Error", wxOK | wxICON_ERROR);
            return false;
        }

        db_ = std::make_unique<tracker::SqliteDatabase>();
        dataDirectory_ = std::make_unique<tracker::DataDirectoryService>(
            *fs_, *db_);
        auto dbResult = dataDirectory_->activate(
            std::filesystem::path(config_->current().dataStorage));
        if (!dbResult) {
            wxMessageBox(
                wxString::Format("Failed to open data directory: %s",
                    wxString::FromUTF8(dbResult.error().c_str())),
                "Startup Error", wxOK | wxICON_ERROR);
            return false;
        }

        gameTitlesRepo_ = std::make_unique<tracker::SqliteGameTitleRepository>(*db_);
        gameTitles_ = std::make_unique<tracker::GameTitleService>(*gameTitlesRepo_);

        formatsRepo_ = std::make_unique<tracker::SqliteFormatRepository>(*db_);
        formats_ = std::make_unique<tracker::FormatService>(*formatsRepo_);

        decksRepo_ = std::make_unique<tracker::SqliteDeckRepository>(*db_);
        decks_ = std::make_unique<tracker::DeckService>(*decksRepo_);

        gameTypesRepo_ = std::make_unique<tracker::SqliteGameTypeRepository>(*db_);
        gameTypes_ = std::make_unique<tracker::GameTypeService>(*gameTypesRepo_);

        gamesRepo_ = std::make_unique<tracker::SqliteGameRepository>(*db_);
        games_ = std::make_unique<tracker::GameService>(*gamesRepo_);
        gameImport_ = std::make_unique<tracker::GameImportService>(
            *decks_, *gameTypes_, *games_);

        ctx_ = std::make_unique<tracker::ui::AppContext>(
            tracker::ui::AppContext{
                *config_, *dataDirectory_, *gameTitles_, *formats_, *decks_,
                *gameTypes_, *games_, *gameImport_});

        auto* frame = new tracker::ui::MainFrame(*ctx_);
        frame->Show(true);
        return true;
    }

private:
    // Declaration order matters: destruction is reverse. Dependencies before
    // dependents.
    std::unique_ptr<tracker::StdFileSystem>            fs_;
    std::unique_ptr<tracker::ConfigService>            config_;
    std::unique_ptr<tracker::SqliteDatabase>           db_;
    std::unique_ptr<tracker::DataDirectoryService>     dataDirectory_;
    std::unique_ptr<tracker::SqliteGameTitleRepository> gameTitlesRepo_;
    std::unique_ptr<tracker::GameTitleService>          gameTitles_;
    std::unique_ptr<tracker::SqliteFormatRepository>   formatsRepo_;
    std::unique_ptr<tracker::FormatService>            formats_;
    std::unique_ptr<tracker::SqliteDeckRepository>      decksRepo_;
    std::unique_ptr<tracker::DeckService>              decks_;
    std::unique_ptr<tracker::SqliteGameTypeRepository> gameTypesRepo_;
    std::unique_ptr<tracker::GameTypeService>          gameTypes_;
    std::unique_ptr<tracker::SqliteGameRepository>     gamesRepo_;
    std::unique_ptr<tracker::GameService>              games_;
    std::unique_ptr<tracker::GameImportService>        gameImport_;
    std::unique_ptr<tracker::ui::AppContext>            ctx_;
};

wxIMPLEMENT_APP(TrackerApp);
