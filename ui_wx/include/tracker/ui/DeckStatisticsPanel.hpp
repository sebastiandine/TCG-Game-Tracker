#pragma once

// DeckStatisticsPanel: per-deck (family or variant) statistics dashboard in a
// closeable FormatWorkspace tab. KPI overview, ranked best/worst matchups, an
// Open Notes list (approve clears Game::notes), and a filterable vs-archetype
// / vs-deck table (column-sortable, default Win %).

#include "tracker/domain/DeckStatistics.hpp"
#include "tracker/domain/Game.hpp"
#include "tracker/ui/StatsWidgets.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include <wx/panel.h>

class wxButton;
class wxListCtrl;
class wxListEvent;
class wxScrolledWindow;
class wxSimplebook;
class wxStaticText;
class wxTextCtrl;
class wxToggleButton;

namespace tracker::ui {

struct AppContext;

class DeckStatisticsPanel : public wxPanel {
public:
    DeckStatisticsPanel(wxWindow* parent, AppContext& ctx);

    void load(std::int64_t formatId, const DeckStatsKey& key);
    void applyTheme();

    [[nodiscard]] const std::string& title() const { return title_; }

private:
    struct MatchupTable {
        wxListCtrl*               list{nullptr};
        std::vector<MatchupStats> rows;
        int                       sortCol{5};  // Win %
        bool                      sortAsc{false};
    };

    struct OpenNoteRow {
        wxPanel*      row{nullptr};
        wxStaticText* opponent{nullptr};
        wxStaticText* date{nullptr};
        wxTextCtrl*   notes{nullptr};
        wxButton*     approve{nullptr};
    };

    void applyDashboardTheme();
    void applyOpenNotesTheme();
    void refresh();
    void applyListPalette();
    void bindTable(MatchupTable& table);
    void refillTable(MatchupTable& table);
    void refillOpenNotes(const std::vector<OpenNote>& notes);
    void onApproveNote(Game game, const std::string& opponentDeck);
    void selectMatchupView(bool archetypes);
    void onColumnClick(wxListEvent& evt);
    void onFilterChanged();
    MatchupTable* tableFor(wxListCtrl* list);

    AppContext& ctx_;
    std::int64_t formatId_{0};
    DeckStatsKey key_{};
    std::string title_;
    MatchRecord overview_{};
    std::string filter_;

    wxStaticText* titleLabel_{nullptr};
    StatCard      gamesCard_{};
    StatCard      winsCard_{};
    StatCard      lossesCard_{};
    StatCard      drawsCard_{};
    StatCard      winPctCard_{};
    RankedList    best_{};
    RankedList    worst_{};
    wxPanel*          notesPanel_{nullptr};
    wxStaticText*     notesHeading_{nullptr};
    wxStaticText*     notesEmpty_{nullptr};
    wxScrolledWindow* notesScroll_{nullptr};
    wxPanel*          notesHost_{nullptr};
    std::vector<OpenNoteRow> noteRows_;
    wxToggleButton* archetypesBtn_{nullptr};
    wxToggleButton* decksBtn_{nullptr};
    wxTextCtrl*     filterInput_{nullptr};
    wxSimplebook*   matchupBook_{nullptr};
    MatchupTable    vsArchetypes_{};
    MatchupTable    vsDecks_{};
};

}  // namespace tracker::ui
