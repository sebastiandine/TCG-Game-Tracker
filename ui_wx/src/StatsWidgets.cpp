#include "tracker/ui/StatsWidgets.hpp"

#include <wx/dcclient.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace tracker::ui {

namespace {

constexpr int kRankedCount = 5;
constexpr int kStatCardMinWidth = 128;
constexpr int kStatCardMaxWidth = 168;
constexpr int kRankedListMinWidth = 260;
constexpr int kRankedListMaxWidth = 380;

}  // namespace

wxString winPctLabel(const MatchRecord& rec) {
    const auto pct = rec.winPercentage();
    if (!pct) return wxString::FromUTF8("\xE2\x80\x94");
    return wxString::Format("%.1f%%", *pct);
}

wxColour dashboardCardFill(Theme theme, const ThemePalette& palette) {
    if (theme == Theme::Dark) return palette.inputBg;
    return wxColour(242, 242, 242);
}

wxColour dashboardMutedText(Theme theme) {
    if (theme == Theme::Dark) return wxColour(170, 170, 170);
    return wxColour(96, 96, 96);
}

wxColour dashboardPositiveText(Theme theme) {
    if (theme == Theme::Dark) return wxColour(129, 199, 132);
    return wxColour(46, 125, 50);
}

wxColour dashboardNegativeText(Theme theme) {
    if (theme == Theme::Dark) return wxColour(239, 154, 154);
    return wxColour(198, 40, 40);
}

wxColour dashboardWinPctAccent(Theme theme, const MatchRecord& rec) {
    const auto pct = rec.winPercentage();
    if (!pct) return dashboardMutedText(theme);
    return *pct >= 50.0 ? dashboardPositiveText(theme)
                        : dashboardNegativeText(theme);
}

wxColour dashboardCardBorder(Theme theme) {
    if (theme == Theme::Dark) return wxColour(80, 80, 80);
    return wxColour(214, 214, 214);
}

void paintDashboardSurface(wxWindow* window, const wxColour& bg,
                           const wxColour& fg) {
    if (window == nullptr) return;
    window->SetBackgroundColour(bg);
    window->SetForegroundColour(fg);
    window->SetOwnBackgroundColour(bg);
    window->SetOwnForegroundColour(fg);
}

void bindDashboardCardChrome(wxPanel* panel) {
    if (panel == nullptr) return;
    panel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    panel->Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
    panel->Bind(wxEVT_PAINT, [panel](wxPaintEvent&) {
        wxPaintDC dc(panel);
        const Theme theme = inferThemeFromWindow(panel);
        const ThemePalette palette = paletteForTheme(theme);
        dc.SetBrush(wxBrush(dashboardCardFill(theme, palette)));
        dc.SetPen(wxPen(dashboardCardBorder(theme)));
        dc.DrawRoundedRectangle(panel->GetClientRect(), 4);
    });
}

void buildStatCard(wxWindow* parent, StatCard& card, const char* caption) {
    card.panel = new wxPanel(parent);
    bindDashboardCardChrome(card.panel);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    card.caption = new wxStaticText(card.panel, wxID_ANY,
                                    wxString::FromUTF8(caption));
    card.value = new wxStaticText(card.panel, wxID_ANY, "0");
    wxFont valueFont = card.value->GetFont();
    valueFont.MakeBold().MakeLarger().MakeLarger();
    card.value->SetFont(valueFont);
    sizer->Add(card.caption, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    sizer->Add(card.value, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    card.panel->SetSizer(sizer);
    card.panel->SetMinSize(wxSize(kStatCardMinWidth, -1));
    card.panel->SetMaxSize(wxSize(kStatCardMaxWidth, -1));
}

void setStatCardValue(StatCard& card, const wxString& value) {
    if (card.value != nullptr) card.value->SetLabel(value);
}

void paintStatCard(StatCard& card, const wxColour& valueColour, Theme theme,
                   const ThemePalette& palette) {
    if (card.panel == nullptr) return;
    const wxColour fill = dashboardCardFill(theme, palette);
    paintDashboardSurface(card.panel, fill, palette.text);
    paintDashboardSurface(card.caption, fill, dashboardMutedText(theme));
    paintDashboardSurface(card.value, fill, valueColour);
    card.panel->Refresh();
}

void buildRankedList(wxWindow* parent, RankedList& list, const char* heading,
                     const char* emptyText) {
    list.panel = new wxPanel(parent);
    bindDashboardCardChrome(list.panel);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    list.heading = new wxStaticText(list.panel, wxID_ANY,
                                    wxString::FromUTF8(heading));
    wxFont headingFont = list.heading->GetFont();
    headingFont.MakeBold();
    list.heading->SetFont(headingFont);
    sizer->Add(list.heading, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    list.emptyLabel = new wxStaticText(list.panel, wxID_ANY,
                                       wxString::FromUTF8(emptyText));
    sizer->Add(list.emptyLabel, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 10);
    for (int i = 0; i < kRankedCount; ++i) {
        auto& row = list.rows[static_cast<std::size_t>(i)];
        row.row = new wxPanel(list.panel);
        auto* hs = new wxBoxSizer(wxHORIZONTAL);
        row.rank = new wxStaticText(row.row, wxID_ANY,
                                    wxString::Format("%d.", i + 1));
        wxFont rankFont = row.rank->GetFont();
        rankFont.MakeBold();
        row.rank->SetFont(rankFont);
        row.name = new wxStaticText(row.row, wxID_ANY, wxEmptyString,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxST_ELLIPSIZE_END);
        row.record = new wxStaticText(row.row, wxID_ANY, wxEmptyString);
        row.pct = new wxStaticText(row.row, wxID_ANY, wxEmptyString);
        wxFont pctFont = row.pct->GetFont();
        pctFont.MakeBold();
        row.pct->SetFont(pctFont);
        hs->Add(row.rank, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        hs->Add(row.name, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
        hs->Add(row.record, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
        hs->Add(row.pct, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 10);
        row.row->SetSizer(hs);
        sizer->Add(row.row, 0, wxEXPAND | wxTOP, 4);
        row.row->Hide();
    }
    sizer->AddSpacer(8);
    list.panel->SetSizer(sizer);
    list.panel->SetMinSize(wxSize(kRankedListMinWidth, 168));
    list.panel->SetMaxSize(wxSize(kRankedListMaxWidth, -1));
}

void addDashboardItems(wxSizer* row, std::initializer_list<wxWindow*> items) {
    if (row == nullptr) return;
    bool first = true;
    for (wxWindow* item : items) {
        if (item == nullptr) continue;
        if (!first) row->AddSpacer(8);
        first = false;
        row->Add(item, 0, wxEXPAND);
    }
    row->AddStretchSpacer(1);
}

void fillRankedList(RankedList& list, const std::vector<MatchupStats>& data) {
    const bool empty = data.empty();
    if (list.emptyLabel != nullptr) list.emptyLabel->Show(empty);
    for (int i = 0; i < kRankedCount; ++i) {
        auto& row = list.rows[static_cast<std::size_t>(i)];
        if (row.row == nullptr) continue;
        if (static_cast<std::size_t>(i) >= data.size()) {
            row.row->Hide();
            continue;
        }
        const auto& item = data[static_cast<std::size_t>(i)];
        row.rank->SetLabel(wxString::Format("%d.", i + 1));
        row.name->SetLabel(wxString::FromUTF8(item.label.c_str()));
        row.record->SetLabel(wxString::Format("%dW %dL %dD", item.record.wins,
                                              item.record.losses,
                                              item.record.draws));
        row.pct->SetLabel(winPctLabel(item.record));
        row.row->Show();
    }
    if (list.panel != nullptr) list.panel->Layout();
}

void paintRankedList(RankedList& list, const wxColour& pctColour, Theme theme,
                     const ThemePalette& palette) {
    if (list.panel == nullptr) return;
    const wxColour fill = dashboardCardFill(theme, palette);
    const wxColour muted = dashboardMutedText(theme);
    paintDashboardSurface(list.panel, fill, palette.text);
    paintDashboardSurface(list.heading, fill, palette.text);
    paintDashboardSurface(list.emptyLabel, fill, muted);
    for (auto& row : list.rows) {
        paintDashboardSurface(row.row, fill, palette.text);
        paintDashboardSurface(row.rank, fill, muted);
        paintDashboardSurface(row.name, fill, palette.text);
        paintDashboardSurface(row.record, fill, muted);
        paintDashboardSurface(row.pct, fill, pctColour);
    }
    list.panel->Refresh();
}

}  // namespace tracker::ui
