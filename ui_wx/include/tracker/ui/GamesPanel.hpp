#pragma once

// GamesPanel: per-format list of recorded games with a plus button and
// filter / sort. Opens GameDialog for create and edit.

#include "tracker/domain/Deck.hpp"
#include "tracker/domain/Game.hpp"
#include "tracker/domain/GameType.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include <wx/panel.h>

class wxBitmapButton;
class wxListCtrl;
class wxListEvent;
class wxTextCtrl;

namespace tracker::ui {

struct AppContext;

class GamesPanel : public wxPanel {
public:
    GamesPanel(wxWindow* parent, AppContext& ctx);

    void loadFormat(std::int64_t formatId);
    void applyTheme();

private:
    void buildToolbar(wxSizer* parent);
    void buildList(wxSizer* parent);
    void onAdd();
    void onRowActivated(wxListEvent& evt);
    void onColumnClick(wxListEvent& evt);

    void refreshList();
    void rebuildRows();
    wxString cellText(const Game& game, int col) const;
    bool matchesFilter(const Game& game) const;

    AppContext& ctx_;
    std::int64_t formatId_{0};
    std::int64_t lastDeckId_{0};
    std::int64_t lastGameTypeId_{0};

    std::vector<Game> allGames_;
    std::vector<const Game*> visible_;
    std::vector<Deck> decks_;
    std::vector<GameType> gameTypes_;

    int sortCol_{-1};
    bool sortAsc_{false};
    std::string filter_;
    bool rebuildingList_{false};

    wxBitmapButton* addButton_{nullptr};
    wxTextCtrl*     filterInput_{nullptr};
    wxListCtrl*     list_{nullptr};
};

}  // namespace tracker::ui
