#pragma once

// StatisticsPanel: per-format dashboard with KPI cards, extra format stats,
// ranked best/worst/most-played decks, and a filterable deck tree. Parent
// rows aggregate core + variants and stay collapsed by default. Clicking a
// row opens a closeable per-deck statistics tab.

#include "tracker/domain/DeckStatistics.hpp"
#include "tracker/ui/StatsWidgets.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <wx/panel.h>

class wxStaticText;
class wxTextCtrl;
class wxTreeListCtrl;
class wxTreeListEvent;

namespace tracker::ui {

struct AppContext;

class StatisticsPanel : public wxPanel {
public:
    StatisticsPanel(wxWindow* parent, AppContext& ctx,
                    std::function<void(const DeckStatsKey&)> onOpenDeckStats);

    void loadFormat(std::int64_t formatId);
    void applyTheme();

private:
    void refresh();
    void rebuildTree();
    void applyTreePalette();
    void applyDashboardTheme();
    void onItemSelected(wxTreeListEvent& evt);
    void onFilterChanged();

    AppContext& ctx_;
    std::int64_t formatId_{0};
    std::function<void(const DeckStatsKey&)> onOpenDeckStats_;
    std::vector<DeckNameStats> rows_;
    FormatStatistics formatStats_{};
    std::string filter_;
    bool rebuilding_{false};

    wxStaticText* titleLabel_{nullptr};
    StatCard      gamesCard_{};
    StatCard      winsCard_{};
    StatCard      lossesCard_{};
    StatCard      drawsCard_{};
    StatCard      winPctCard_{};
    StatCard      decksCard_{};
    StatCard      opponentsCard_{};
    StatCard      streakCard_{};
    RankedList    best_{};
    RankedList    worst_{};
    RankedList    mostPlayed_{};
    wxStaticText* decksHeading_{nullptr};
    wxTextCtrl*   filterInput_{nullptr};
    wxTreeListCtrl* tree_{nullptr};
};

}  // namespace tracker::ui
