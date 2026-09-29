#pragma once

// MainFrame: top-level window. Hosts a CCM3-style themed menu strip (File /
// Games / Formats / Data / Help) and a content area. When no format is
// selected a centered label is shown; when a format is selected a per-format
// workspace with a tab bar (Decks, ...) replaces the label.

#include "tracker/domain/Format.hpp"
#include "tracker/domain/GameTitle.hpp"
#include "tracker/ui/AppContext.hpp"

#include <vector>

#include <wx/frame.h>

class wxMenu;
class wxPanel;
class wxSizer;
class wxStaticText;
class wxWindow;

namespace tracker::ui {

class FormatWorkspace;

class MainFrame : public wxFrame {
public:
    explicit MainFrame(AppContext& ctx);

private:
    void buildMenuBar();
    void buildLayout();
    void applyTheme();

    bool rebuildGamesMenu();
    bool rebuildFormatsMenu();
    void selectGame(std::int64_t gameId);
    void selectFormat(std::int64_t formatId);
    void persistSelection();
    void updateTitleAndEmptyState();
    void showEmptyState();
    void showWorkspace();

    wxStaticText* addMenuLabel(wxSizer* sizer, const wxString& text, int border,
                               void (MainFrame::*handler)());
    void popupMenuUnder(wxWindow* label, wxMenu& menu);

    void onOpenFileMenu();
    void onOpenGamesMenu();
    void onOpenFormatsMenu();
    void onOpenDataMenu();
    void onOpenHelpMenu();

    void onSettings(wxCommandEvent&);
    void onQuit(wxCommandEvent&);
    void onAbout(wxCommandEvent&);
    void onCreateGame(wxCommandEvent&);
    void onCreateFormat(wxCommandEvent&);
    void onCreateGameType(wxCommandEvent&);
    void onCreateArchetype(wxCommandEvent&);
    void onImportGames(wxCommandEvent&);
    void onGameSelected(wxCommandEvent& evt);
    void onFormatSelected(wxCommandEvent& evt);

    AppContext& ctx_;

    wxPanel*          menuStrip_{nullptr};
    wxStaticText*     fileMenuLabel_{nullptr};
    wxStaticText*     gamesMenuLabel_{nullptr};
    wxStaticText*     formatsMenuLabel_{nullptr};
    wxStaticText*     dataMenuLabel_{nullptr};
    wxStaticText*     helpMenuLabel_{nullptr};

    wxPanel*          contentPanel_{nullptr};
    wxPanel*          emptyPanel_{nullptr};
    wxStaticText*     centerLabel_{nullptr};
    FormatWorkspace*  workspace_{nullptr};

    std::vector<GameTitle> loadedGames_;
    std::vector<Format> loadedFormats_;
    std::int64_t selectedGameId_{0};
    std::int64_t selectedFormatId_{0};

    enum Ids : int {
        IdSettings = wxID_HIGHEST + 1,
        IdAbout,
        IdCreateGame,
        IdCreateFormat,
        IdCreateGameType,
        IdCreateArchetype,
        IdImportGames,
        IdFormatBase = wxID_HIGHEST + 100,
        IdFormatMax  = IdFormatBase + 500,
        IdGameBase   = IdFormatMax + 1,
        IdGameMax    = IdGameBase + 500,
    };
};

}  // namespace tracker::ui
