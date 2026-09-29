#include "tracker/ui/FormatWorkspace.hpp"
#include "tracker/ui/AppContext.hpp"
#include "tracker/ui/DecksPanel.hpp"
#include "tracker/ui/DeckStatisticsPanel.hpp"
#include "tracker/ui/GamesPanel.hpp"
#include "tracker/ui/StatisticsPanel.hpp"
#include "tracker/ui/Theme.hpp"

#include <string>

#include <wx/dcclient.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/simplebook.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace tracker::ui {

namespace {

enum {
    kTabGames = 0,
    kTabStatistics = 1,
    kTabDecks = 2,
    kTabCount = 3,
};

wxColour lighten(const wxColour& c, int amount) {
    auto lift = [amount](unsigned char ch) -> unsigned char {
        const int r = static_cast<int>(ch) + amount;
        return static_cast<unsigned char>(r > 255 ? 255 : r);
    };
    return wxColour(lift(c.Red()), lift(c.Green()), lift(c.Blue()));
}

wxColour darken(const wxColour& c, int amount) {
    auto drop = [amount](unsigned char ch) -> unsigned char {
        const int r = static_cast<int>(ch) - amount;
        return static_cast<unsigned char>(r < 0 ? 0 : r);
    };
    return wxColour(drop(c.Red()), drop(c.Green()), drop(c.Blue()));
}

wxString deckTabLabel(const DeckStatsKey& key) {
    if (key.scope == DeckStatsScope::NameAggregate) {
        return wxString::FromUTF8(key.deckName.c_str());
    }
    std::string text = key.deckName;
    if (!key.variant.empty()) {
        text += " \xE2\x80\x94 ";
        text += key.variant;
    }
    return wxString::FromUTF8(text.c_str());
}

}  // namespace

FormatWorkspace::FormatWorkspace(wxWindow* parent, AppContext& ctx)
    : wxPanel(parent),
      ctx_(ctx) {
    auto* root = new wxBoxSizer(wxVERTICAL);

    buildTabBar();
    root->Add(tabBar_, 0, wxEXPAND);

    book_ = new wxSimplebook(this, wxID_ANY);
    gamesPanel_ = new GamesPanel(book_, ctx_);
    book_->AddPage(gamesPanel_, "Games");
    statisticsPanel_ = new StatisticsPanel(
        book_, ctx_,
        [this](const DeckStatsKey& key) { openDeckStats(key); });
    book_->AddPage(statisticsPanel_, "Statistics");
    decksPanel_ = new DecksPanel(book_, ctx_);
    book_->AddPage(decksPanel_, "Decks");

    root->Add(book_, 1, wxEXPAND | wxTOP, 2);
    SetSizer(root);

    selectTab(0);
}

void FormatWorkspace::loadFormat(std::int64_t formatId) {
    formatId_ = formatId;
    closeAllDeckTabs();
    decksPanel_->loadFormat(formatId_);
    gamesPanel_->loadFormat(formatId_);
    statisticsPanel_->loadFormat(formatId_);
}

void FormatWorkspace::applyTheme() {
    const ThemePalette palette = paletteForTheme(ctx_.config.current().theme);
    applyThemeToWindowTree(this, palette, ctx_.config.current().theme);
    refreshTabBarTheme();
    decksPanel_->applyTheme();
    gamesPanel_->applyTheme();
    statisticsPanel_->applyTheme();
    for (auto& tab : tabs_) {
        if (tab.deckPanel != nullptr) tab.deckPanel->applyTheme();
    }
    Refresh();
}

void FormatWorkspace::buildTabBar() {
    tabBar_ = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize,
                                   wxHSCROLL | wxBORDER_NONE);
    tabBar_->SetScrollRate(16, 0);
    tabBar_->SetBackgroundStyle(wxBG_STYLE_PAINT);
    auto* tabSizer = new wxBoxSizer(wxHORIZONTAL);
    tabBar_->SetSizer(tabSizer);
    tabSizer->AddSpacer(4);

    addFixedTab("Games", TabKind::Games);
    addFixedTab("Statistics", TabKind::Statistics);
    addFixedTab("Decks", TabKind::Decks);

    tabBar_->Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
        wxPaintDC dc(tabBar_);
        const ThemePalette palette =
            paletteForTheme(ctx_.config.current().theme);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(palette.panelBg));
        dc.DrawRectangle(tabBar_->GetClientRect());
        dc.SetPen(wxPen(darken(palette.text, 120), 1));
        const wxRect r = tabBar_->GetClientRect();
        dc.DrawLine(r.GetLeft(), r.GetBottom(),
                    r.GetRight(), r.GetBottom());
    });
    tabBar_->Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});

    refreshTabBarTheme();
    fitTabBar();
}

void FormatWorkspace::addFixedTab(const char* label, TabKind kind) {
    auto* tabSizer = tabBar_->GetSizer();
    if (tabSizer == nullptr) return;
    auto* tab = new wxPanel(tabBar_, wxID_ANY, wxDefaultPosition,
                            wxDefaultSize, wxBORDER_NONE);
    tab->SetCursor(wxCursor(wxCURSOR_HAND));
    tab->SetBackgroundStyle(wxBG_STYLE_PAINT);
    auto* text = new wxStaticText(tab, wxID_ANY,
                                  wxString::FromUTF8(label));
    auto* inner = new wxBoxSizer(wxHORIZONTAL);
    inner->Add(text, 0,
               wxALIGN_CENTER | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 5);
    tab->SetSizer(inner);

    auto onClick = [this, tab](wxMouseEvent&) {
        selectTab(tabIndexOf(tab));
    };
    tab->Bind(wxEVT_LEFT_DOWN, onClick);
    text->Bind(wxEVT_LEFT_DOWN, onClick);
    bindTabPaint(tab);

    TabInfo info;
    info.kind = kind;
    info.panel = tab;
    info.label = text;
    tabs_.push_back(info);

    if (tabs_.size() > 1) tabSizer->AddSpacer(4);
    tabSizer->Add(tab, 0,
                  wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 3);
}

void FormatWorkspace::bindTabPaint(wxPanel* tab) {
    tab->Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
    tab->Bind(wxEVT_PAINT, [this, tab](wxPaintEvent&) {
        wxPaintDC dc(tab);
        const ThemePalette palette =
            paletteForTheme(ctx_.config.current().theme);
        const bool dark = ctx_.config.current().theme == Theme::Dark;
        const bool selected = (tabIndexOf(tab) == activeTab_);
        const wxColour bg = palette.buttonBg;
        const wxColour frame =
            dark ? lighten(palette.panelBg, 55)
                 : darken(palette.panelBg, 45);
        const wxColour frameSel =
            dark ? lighten(palette.panelBg, 85)
                 : darken(palette.panelBg, 70);
        const wxRect r = tab->GetClientRect();
        dc.SetPen(wxPen(selected ? frameSel : frame, 1));
        dc.SetBrush(wxBrush(bg));
        dc.DrawRectangle(r.x, r.y, r.width, r.height);
        if (selected) {
            dc.SetPen(wxPen(palette.text, 2));
            dc.DrawLine(r.GetLeft() + 4, r.GetBottom() - 1,
                        r.GetRight() - 4, r.GetBottom() - 1);
        }
    });
}

void FormatWorkspace::detachTabChrome(wxPanel* tab) {
    auto* tabSizer = tabBar_ != nullptr ? tabBar_->GetSizer() : nullptr;
    if (tabSizer == nullptr || tab == nullptr) return;
    for (int i = 0; i < tabSizer->GetItemCount(); ++i) {
        wxSizerItem* item = tabSizer->GetItem(i);
        if (item == nullptr || item->GetWindow() != tab) continue;
        tabSizer->Detach(i);
        if (i > 0) {
            wxSizerItem* prev = tabSizer->GetItem(i - 1);
            if (prev != nullptr && prev->IsSpacer() &&
                prev->GetSpacer().GetWidth() == 4) {
                tabSizer->Detach(i - 1);
            }
        }
        break;
    }
    tab->Destroy();
}

void FormatWorkspace::openDeckStats(const DeckStatsKey& key) {
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        if (tabs_[i].kind == TabKind::DeckDetail &&
            tabs_[i].deckKey == key) {
            selectTab(static_cast<int>(i));
            return;
        }
    }

    if (book_ == nullptr || tabBar_ == nullptr) return;
    auto* tabSizer = tabBar_->GetSizer();
    if (tabSizer == nullptr) return;

    auto* page = new DeckStatisticsPanel(book_, ctx_);
    page->load(formatId_, key);
    book_->AddPage(page, deckTabLabel(key));
    page->applyTheme();

    auto* tab = new wxPanel(tabBar_, wxID_ANY, wxDefaultPosition,
                            wxDefaultSize, wxBORDER_NONE);
    tab->SetCursor(wxCursor(wxCURSOR_HAND));
    tab->SetBackgroundStyle(wxBG_STYLE_PAINT);
    auto* text = new wxStaticText(tab, wxID_ANY, deckTabLabel(key));
    auto* closeBtn = new wxStaticText(tab, wxID_ANY, "x");
    closeBtn->SetCursor(wxCursor(wxCURSOR_HAND));
    auto* inner = new wxBoxSizer(wxHORIZONTAL);
    inner->Add(text, 0, wxALIGN_CENTER | wxLEFT | wxTOP | wxBOTTOM, 5);
    inner->Add(closeBtn, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM,
               5);
    tab->SetSizer(inner);

    auto onClick = [this, tab](wxMouseEvent&) {
        selectTab(tabIndexOf(tab));
    };
    tab->Bind(wxEVT_LEFT_DOWN, onClick);
    text->Bind(wxEVT_LEFT_DOWN, onClick);
    closeBtn->Bind(wxEVT_LEFT_DOWN, [this, tab](wxMouseEvent&) {
        closeDeckTab(tabIndexOf(tab));
    });
    bindTabPaint(tab);

    TabInfo info;
    info.kind = TabKind::DeckDetail;
    info.panel = tab;
    info.label = text;
    info.closeBtn = closeBtn;
    info.deckKey = key;
    info.deckPanel = page;
    tabs_.push_back(info);

    tabSizer->AddSpacer(4);
    tabSizer->Add(tab, 0,
                  wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 3);

    tabBar_->Layout();
    fitTabBar();
    refreshTabBarTheme();
    selectTab(static_cast<int>(tabs_.size()) - 1);
}

void FormatWorkspace::closeDeckTab(int index) {
    if (index < kTabCount || index >= static_cast<int>(tabs_.size())) return;
    if (tabs_[static_cast<std::size_t>(index)].kind != TabKind::DeckDetail) {
        return;
    }

    const bool wasActive = (index == activeTab_);
    auto* tab = tabs_[static_cast<std::size_t>(index)].panel;
    detachTabChrome(tab);

    if (book_ != nullptr) book_->DeletePage(index);
    tabs_.erase(tabs_.begin() + index);

    if (wasActive) {
        selectTab(kTabStatistics);
    } else {
        if (activeTab_ > index) --activeTab_;
        refreshTabBarTheme();
        fitTabBar();
    }
}

void FormatWorkspace::closeAllDeckTabs() {
    while (static_cast<int>(tabs_.size()) > kTabCount) {
        const int index = static_cast<int>(tabs_.size()) - 1;
        auto* tab = tabs_[static_cast<std::size_t>(index)].panel;
        detachTabChrome(tab);
        if (book_ != nullptr) book_->DeletePage(index);
        tabs_.pop_back();
    }
    if (activeTab_ >= kTabCount) {
        activeTab_ = kTabStatistics;
        if (book_ != nullptr) book_->SetSelection(kTabStatistics);
    }
    refreshTabBarTheme();
    fitTabBar();
}

void FormatWorkspace::selectTab(int index) {
    if (index < 0 || index >= static_cast<int>(tabs_.size()) ||
        book_ == nullptr)
        return;
    activeTab_ = index;
    book_->SetSelection(index);
    if (index == kTabGames && gamesPanel_ != nullptr && formatId_ > 0) {
        gamesPanel_->loadFormat(formatId_);
    }
    if (index == kTabStatistics && statisticsPanel_ != nullptr &&
        formatId_ > 0) {
        statisticsPanel_->loadFormat(formatId_);
    }
    if (index >= kTabCount) {
        auto& tab = tabs_[static_cast<std::size_t>(index)];
        if (tab.deckPanel != nullptr && formatId_ > 0) {
            tab.deckPanel->load(formatId_, tab.deckKey);
        }
    }
    refreshTabBarTheme();
}

void FormatWorkspace::refreshTabBarTheme() {
    if (tabBar_ == nullptr) return;

    const ThemePalette palette =
        paletteForTheme(ctx_.config.current().theme);
    const wxColour barBg = palette.panelBg;
    const wxColour tabBg = palette.buttonBg;

    tabBar_->SetBackgroundColour(barBg);
    tabBar_->SetOwnBackgroundColour(barBg);

    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        auto* tab = tabs_[i].panel;
        auto* label = tabs_[i].label;
        if (tab == nullptr || label == nullptr) continue;
        const bool selected = (static_cast<int>(i) == activeTab_);
        tab->SetBackgroundColour(tabBg);
        tab->SetOwnBackgroundColour(tabBg);
        label->SetBackgroundColour(tabBg);
        label->SetOwnBackgroundColour(tabBg);
        label->SetForegroundColour(palette.text);
        label->SetOwnForegroundColour(palette.text);
        wxFont font = label->GetFont();
        font.SetWeight(selected ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
        label->SetFont(font);
        if (auto* closeBtn = tabs_[i].closeBtn) {
            closeBtn->SetBackgroundColour(tabBg);
            closeBtn->SetOwnBackgroundColour(tabBg);
            closeBtn->SetForegroundColour(palette.text);
            closeBtn->SetOwnForegroundColour(palette.text);
            closeBtn->Refresh();
        }
        tab->Refresh();
        label->Refresh();
    }
    tabBar_->Layout();
    tabBar_->Refresh();
    fitTabBar();
}

void FormatWorkspace::fitTabBar() {
    if (tabBar_ == nullptr) return;
    tabBar_->Layout();
    tabBar_->FitInside();
    const wxSize best = tabBar_->GetBestSize();
    constexpr int kBarHeight = 36;
    tabBar_->SetMinSize(wxSize(-1, best.GetHeight() > 0 ? best.GetHeight()
                                                        : kBarHeight));
    if (wxWindow* parent = tabBar_->GetParent()) parent->Layout();
}

int FormatWorkspace::tabIndexOf(const wxWindow* tab) const {
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        if (tabs_[i].panel == tab) return static_cast<int>(i);
    }
    return -1;
}

}  // namespace tracker::ui
