#pragma once

// Shared dashboard chrome for Statistics and DeckStatistics: KPI cards and
// compact ranked lists. Colors are derived from the current theme at paint
// time so Light/Dark switches stay live.

#include "tracker/domain/DeckStatistics.hpp"
#include "tracker/domain/Enums.hpp"
#include "tracker/ui/Theme.hpp"

#include <array>
#include <initializer_list>
#include <vector>

#include <wx/colour.h>
#include <wx/string.h>

class wxPanel;
class wxSizer;
class wxStaticText;
class wxWindow;

namespace tracker::ui {

struct StatCard {
    wxPanel*      panel{nullptr};
    wxStaticText* caption{nullptr};
    wxStaticText* value{nullptr};
};

struct RankedRow {
    wxPanel*      row{nullptr};
    wxStaticText* rank{nullptr};
    wxStaticText* name{nullptr};
    wxStaticText* record{nullptr};
    wxStaticText* pct{nullptr};
};

struct RankedList {
    wxPanel*                 panel{nullptr};
    wxStaticText*            heading{nullptr};
    wxStaticText*            emptyLabel{nullptr};
    std::array<RankedRow, 5> rows{};
};

wxString winPctLabel(const MatchRecord& rec);

wxColour dashboardCardFill(Theme theme, const ThemePalette& palette);
wxColour dashboardMutedText(Theme theme);
wxColour dashboardPositiveText(Theme theme);
wxColour dashboardNegativeText(Theme theme);
wxColour dashboardWinPctAccent(Theme theme, const MatchRecord& rec);

void bindDashboardCardChrome(wxPanel* panel);
void paintDashboardSurface(wxWindow* window, const wxColour& bg,
                           const wxColour& fg);

void buildStatCard(wxWindow* parent, StatCard& card, const char* caption);
void setStatCardValue(StatCard& card, const wxString& value);
void paintStatCard(StatCard& card, const wxColour& valueColour, Theme theme,
                   const ThemePalette& palette);

void buildRankedList(wxWindow* parent, RankedList& list, const char* heading,
                     const char* emptyText);
void fillRankedList(RankedList& list, const std::vector<MatchupStats>& data);
void paintRankedList(RankedList& list, const wxColour& pctColour, Theme theme,
                     const ThemePalette& palette);

// Add cards/lists that grow equally up to their max width, then leave spare
// window space empty instead of stretching chrome into wide empty bars.
void addDashboardItems(wxSizer* row, std::initializer_list<wxWindow*> items);

}  // namespace tracker::ui
