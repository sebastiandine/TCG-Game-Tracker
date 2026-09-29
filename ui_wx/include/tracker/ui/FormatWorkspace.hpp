#pragma once

// FormatWorkspace: per-format content area shown when a format is selected.
// Hosts a CCM3-style custom tab bar over a wxSimplebook.
// Fixed tabs: Games, Statistics, Decks. Clicking a statistics row opens a
// closeable per-deck statistics tab after those.

#include "tracker/domain/DeckStatistics.hpp"

#include <cstdint>
#include <vector>

#include <wx/panel.h>

class wxScrolledWindow;
class wxSimplebook;
class wxStaticText;

namespace tracker::ui {

struct AppContext;
class DecksPanel;
class DeckStatisticsPanel;
class GamesPanel;
class StatisticsPanel;

class FormatWorkspace : public wxPanel {
public:
    FormatWorkspace(wxWindow* parent, AppContext& ctx);

    // Switch to a new format: reload all tabs for the given format id.
    void loadFormat(std::int64_t formatId);

    // Re-apply the current theme palette.
    void applyTheme();

private:
    enum class TabKind { Games, Statistics, Decks, DeckDetail };

    struct TabInfo {
        TabKind              kind{TabKind::Games};
        wxPanel*             panel{nullptr};
        wxStaticText*        label{nullptr};
        wxStaticText*        closeBtn{nullptr};
        DeckStatsKey         deckKey;
        DeckStatisticsPanel* deckPanel{nullptr};
    };

    void buildTabBar();
    void addFixedTab(const char* label, TabKind kind);
    void bindTabPaint(wxPanel* tab);
    void detachTabChrome(wxPanel* tab);
    void openDeckStats(const DeckStatsKey& key);
    void closeDeckTab(int index);
    void closeAllDeckTabs();
    void selectTab(int index);
    void refreshTabBarTheme();
    void fitTabBar();
    [[nodiscard]] int tabIndexOf(const wxWindow* tab) const;

    AppContext& ctx_;
    std::int64_t formatId_{0};

    wxScrolledWindow*           tabBar_{nullptr};
    wxSimplebook*               book_{nullptr};
    DecksPanel*                 decksPanel_{nullptr};
    GamesPanel*                 gamesPanel_{nullptr};
    StatisticsPanel*            statisticsPanel_{nullptr};
    std::vector<TabInfo>        tabs_;
    int                         activeTab_{0};
};

}  // namespace tracker::ui
