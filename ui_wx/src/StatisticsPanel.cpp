#include "tracker/ui/StatisticsPanel.hpp"
#include "tracker/ui/AppContext.hpp"
#include "tracker/ui/Theme.hpp"
#include "tracker/domain/DeckStatistics.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include <wx/clntdata.h>
#include <wx/dataview.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/treelist.h>

namespace tracker::ui {

namespace {

enum {
    kColDeck = 0,
    kColGames,
    kColWins,
    kColLosses,
    kColDraws,
    kColWinPct
};

class DeckStatsItemData : public wxClientData {
public:
    explicit DeckStatsItemData(DeckStatsKey key) : key_(std::move(key)) {}

    [[nodiscard]] const DeckStatsKey& key() const { return key_; }

private:
    DeckStatsKey key_;
};

void installTreeListRowCursor(wxTreeListCtrl* tree, wxWindow* window) {
    if (tree == nullptr || window == nullptr) return;
    installActionableRowCursor(window, [tree, window](const wxPoint& pos) {
        wxDataViewCtrl* view = tree->GetDataView();
        if (view == nullptr) return false;
        const wxPoint viewPos =
            view->ScreenToClient(window->ClientToScreen(pos));
        wxDataViewItem item;
        wxDataViewColumn* column = nullptr;
        view->HitTest(viewPos, item, column);
        return item.IsOk();
    });
    const wxWindowList& children = window->GetChildren();
    for (wxWindowList::compatibility_iterator it = children.GetFirst(); it;
         it = it->GetNext()) {
        installTreeListRowCursor(tree, it->GetData());
    }
}

bool containsCI(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto it = std::search(
        haystack.begin(), haystack.end(), needle.begin(), needle.end(),
        [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a)) ==
                   std::tolower(static_cast<unsigned char>(b));
        });
    return it != haystack.end();
}

wxString childLabel(const Deck& deck) {
    if (deck.variant.empty()) {
        return wxString::Format(
            "%s (default)", wxString::FromUTF8(deck.name.c_str()));
    }
    return wxString::Format(
        "%s (variante)", wxString::FromUTF8(deck.variant.c_str()));
}

void setRecordColumns(wxTreeListCtrl* tree, const wxTreeListItem& item,
                      const MatchRecord& rec) {
    tree->SetItemText(item, kColGames,
                      wxString::Format("%d", rec.total()));
    tree->SetItemText(item, kColWins,
                      wxString::Format("%d", rec.wins));
    tree->SetItemText(item, kColLosses,
                      wxString::Format("%d", rec.losses));
    tree->SetItemText(item, kColDraws,
                      wxString::Format("%d", rec.draws));
    tree->SetItemText(item, kColWinPct, winPctLabel(rec));
}

wxString streakLabel(int streak) {
    if (streak > 0) return wxString::Format("%dW", streak);
    if (streak < 0) return wxString::Format("%dL", -streak);
    return wxString::FromUTF8("\xE2\x80\x94");
}

wxColour streakColour(Theme theme, int streak) {
    if (streak > 0) return dashboardPositiveText(theme);
    if (streak < 0) return dashboardNegativeText(theme);
    return dashboardMutedText(theme);
}

bool rowMatchesFilter(const DeckNameStats& row, const std::string& filter) {
    if (containsCI(row.name, filter)) return true;
    for (const auto& variant : row.variants) {
        if (containsCI(variant.deck.name, filter)) return true;
        if (containsCI(variant.deck.variant, filter)) return true;
    }
    return false;
}

}  // namespace

StatisticsPanel::StatisticsPanel(
    wxWindow* parent, AppContext& ctx,
    std::function<void(const DeckStatsKey&)> onOpenDeckStats)
    : wxPanel(parent),
      ctx_(ctx),
      onOpenDeckStats_(std::move(onOpenDeckStats)) {
    Freeze();

    auto* root = new wxBoxSizer(wxVERTICAL);

    titleLabel_ = new wxStaticText(this, wxID_ANY, "Statistics");
    wxFont titleFont = titleLabel_->GetFont();
    titleFont.MakeBold().MakeLarger();
    titleLabel_->SetFont(titleFont);
    root->Add(titleLabel_, 0, wxLEFT | wxRIGHT | wxTOP, 12);

    auto* kpi = new wxBoxSizer(wxHORIZONTAL);
    buildStatCard(this, gamesCard_, "Games");
    buildStatCard(this, winsCard_, "Wins");
    buildStatCard(this, lossesCard_, "Losses");
    buildStatCard(this, drawsCard_, "Draws");
    buildStatCard(this, winPctCard_, "Win %");
    addDashboardItems(kpi, {gamesCard_.panel, winsCard_.panel, lossesCard_.panel,
                            drawsCard_.panel, winPctCard_.panel});
    root->Add(kpi, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    auto* extra = new wxBoxSizer(wxHORIZONTAL);
    buildStatCard(this, decksCard_, "Decks");
    buildStatCard(this, opponentsCard_, "Opponents");
    buildStatCard(this, streakCard_, "Streak");
    addDashboardItems(extra, {decksCard_.panel, opponentsCard_.panel,
                              streakCard_.panel});
    root->Add(extra, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    auto* highlights = new wxBoxSizer(wxHORIZONTAL);
    buildRankedList(this, best_, "Best decks", "No decks yet");
    buildRankedList(this, worst_, "Worst decks", "No decks yet");
    buildRankedList(this, mostPlayed_, "Most played", "No decks yet");
    addDashboardItems(highlights,
                      {best_.panel, worst_.panel, mostPlayed_.panel});
    root->Add(highlights, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
    decksHeading_ = new wxStaticText(this, wxID_ANY, "Deck records");
    wxFont headingFont = decksHeading_->GetFont();
    headingFont.MakeBold();
    decksHeading_->SetFont(headingFont);
    toolbar->Add(decksHeading_, 0, wxALIGN_CENTER_VERTICAL);
    toolbar->AddStretchSpacer(1);
    filterInput_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                  wxSize(220, -1), wxTE_RICH2);
    installTextCtrlPlaceholder(filterInput_, "Filter decks...");
    toolbar->Add(filterInput_, 0, wxALIGN_CENTER_VERTICAL);
    root->Add(toolbar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    tree_ = new wxTreeListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                               wxTL_SINGLE);
    tree_->AppendColumn("Deck", 220, wxALIGN_LEFT, wxCOL_RESIZABLE);
    tree_->AppendColumn("Games", 70, wxALIGN_RIGHT, wxCOL_RESIZABLE);
    tree_->AppendColumn("Wins", 70, wxALIGN_RIGHT, wxCOL_RESIZABLE);
    tree_->AppendColumn("Losses", 70, wxALIGN_RIGHT, wxCOL_RESIZABLE);
    tree_->AppendColumn("Draws", 70, wxALIGN_RIGHT, wxCOL_RESIZABLE);
    tree_->AppendColumn("Win %", 80, wxALIGN_RIGHT, wxCOL_RESIZABLE);

    tree_->Bind(wxEVT_TREELIST_SELECTION_CHANGED,
                &StatisticsPanel::onItemSelected, this);
    tree_->Bind(wxEVT_TREELIST_ITEM_ACTIVATED,
                &StatisticsPanel::onItemSelected, this);
    filterInput_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
        onFilterChanged();
    });

    wxWindow* motionTarget = tree_->GetView();
    if (motionTarget == nullptr) motionTarget = tree_;
    installTreeListRowCursor(tree_, motionTarget);

    root->Add(tree_, 1, wxEXPAND | wxALL, 12);
    SetSizer(root);
    applyTheme();
    Thaw();
}

void StatisticsPanel::loadFormat(std::int64_t formatId) {
    formatId_ = formatId;
    filter_.clear();
    if (filterInput_ != nullptr) filterInput_->ChangeValue("");
    refresh();
}

void StatisticsPanel::refresh() {
    rows_.clear();
    formatStats_ = {};

    std::string title = "Statistics";
    if (formatId_ > 0) {
        auto formatResult = ctx_.formats.listByGame(
            ctx_.config.current().selectedGameId);
        if (formatResult) {
            for (const auto& format : formatResult.value()) {
                if (format.id != formatId_) continue;
                title = format.name;
                break;
            }
        }

        auto deckResult = ctx_.decks.listByFormat(formatId_);
        auto gameResult = ctx_.games.listByFormat(formatId_);
        if (deckResult && gameResult) {
            rows_ = computeDeckStatistics(deckResult.value(),
                                          gameResult.value());
            formatStats_ = computeFormatStatistics(deckResult.value(),
                                                   gameResult.value());
        }
    }

    if (titleLabel_ != nullptr) {
        titleLabel_->SetLabel(wxString::FromUTF8(title.c_str()));
    }

    const auto& rec = formatStats_.overview;
    setStatCardValue(gamesCard_, wxString::Format("%d", rec.total()));
    setStatCardValue(winsCard_, wxString::Format("%d", rec.wins));
    setStatCardValue(lossesCard_, wxString::Format("%d", rec.losses));
    setStatCardValue(drawsCard_, wxString::Format("%d", rec.draws));
    setStatCardValue(winPctCard_, winPctLabel(rec));
    setStatCardValue(decksCard_,
                     wxString::Format("%d", formatStats_.decksPlayed));
    setStatCardValue(opponentsCard_,
                     wxString::Format("%d", formatStats_.uniqueOpponents));
    setStatCardValue(streakCard_, streakLabel(formatStats_.streak));

    fillRankedList(best_, formatStats_.bestDecks);
    fillRankedList(worst_, formatStats_.worstDecks);
    fillRankedList(mostPlayed_, formatStats_.mostPlayed);

    rebuildTree();
    applyTheme();
    Layout();
}

void StatisticsPanel::rebuildTree() {
    if (tree_ == nullptr) return;

    rebuilding_ = true;
    tree_->Freeze();
    tree_->DeleteAllItems();

    const auto root = tree_->GetRootItem();
    for (const auto& row : rows_) {
        if (!rowMatchesFilter(row, filter_)) continue;

        const bool hasChildren = row.variants.size() > 1;
        const wxTreeListItem parent = tree_->AppendItem(
            root, wxString::FromUTF8(row.name.c_str()));
        setRecordColumns(tree_, parent, row.aggregate);
        tree_->SetItemData(
            parent, new DeckStatsItemData{DeckStatsKey{
                .scope = DeckStatsScope::NameAggregate,
                .deckName = row.name}});

        if (!hasChildren) continue;

        for (const auto& variant : row.variants) {
            if (!filter_.empty() && !containsCI(row.name, filter_) &&
                !containsCI(variant.deck.name, filter_) &&
                !containsCI(variant.deck.variant, filter_)) {
                continue;
            }
            const wxTreeListItem child =
                tree_->AppendItem(parent, childLabel(variant.deck));
            setRecordColumns(tree_, child, variant.record);
            if (variant.deck.variant.empty()) {
                tree_->SetItemData(
                    child, new DeckStatsItemData{DeckStatsKey{
                        .scope = DeckStatsScope::NameAggregate,
                        .deckName = row.name}});
            } else {
                tree_->SetItemData(
                    child, new DeckStatsItemData{DeckStatsKey{
                        .scope = DeckStatsScope::Variant,
                        .deckName = row.name,
                        .variantDeckId = variant.deck.id,
                        .variant = variant.deck.variant}});
            }
        }
        tree_->Collapse(parent);
    }

    tree_->Thaw();
    rebuilding_ = false;
}

void StatisticsPanel::onFilterChanged() {
    if (filterInput_ == nullptr) return;
    filter_ = filterInput_->GetValue().ToStdString(wxConvUTF8);
    rebuildTree();
}

void StatisticsPanel::onItemSelected(wxTreeListEvent& evt) {
    if (rebuilding_ || !onOpenDeckStats_ || tree_ == nullptr) return;
    const wxTreeListItem item = evt.GetItem();
    if (!item.IsOk()) return;
    auto* data =
        static_cast<DeckStatsItemData*>(tree_->GetItemData(item));
    if (data == nullptr) return;
    onOpenDeckStats_(data->key());
}

void StatisticsPanel::applyTreePalette() {
    if (tree_ == nullptr) return;

    const ThemePalette palette =
        paletteForTheme(ctx_.config.current().theme);
    tree_->SetBackgroundColour(palette.inputBg);
    tree_->SetForegroundColour(palette.inputText);
    tree_->SetOwnBackgroundColour(palette.inputBg);
    tree_->SetOwnForegroundColour(palette.inputText);
    if (wxWindow* view = tree_->GetView()) {
        view->SetBackgroundColour(palette.inputBg);
        view->SetForegroundColour(palette.inputText);
        view->SetOwnBackgroundColour(palette.inputBg);
        view->SetOwnForegroundColour(palette.inputText);
    }
}

void StatisticsPanel::applyDashboardTheme() {
    const Theme theme = ctx_.config.current().theme;
    const ThemePalette palette = paletteForTheme(theme);
    paintStatCard(gamesCard_, palette.text, theme, palette);
    paintStatCard(winsCard_, dashboardPositiveText(theme), theme, palette);
    paintStatCard(lossesCard_, dashboardNegativeText(theme), theme, palette);
    paintStatCard(drawsCard_, palette.text, theme, palette);
    paintStatCard(winPctCard_,
                  dashboardWinPctAccent(theme, formatStats_.overview), theme,
                  palette);
    paintStatCard(decksCard_, palette.text, theme, palette);
    paintStatCard(opponentsCard_, palette.text, theme, palette);
    paintStatCard(streakCard_, streakColour(theme, formatStats_.streak), theme,
                  palette);
    paintRankedList(best_, dashboardPositiveText(theme), theme, palette);
    paintRankedList(worst_, dashboardNegativeText(theme), theme, palette);
    paintRankedList(mostPlayed_, palette.text, theme, palette);
}

void StatisticsPanel::applyTheme() {
    const Theme theme = ctx_.config.current().theme;
    const ThemePalette palette = paletteForTheme(theme);
    applyThemeToWindowTree(this, palette, theme);
    if (filterInput_ != nullptr) {
        applyPaletteToTextCtrl(filterInput_, palette, theme);
    }
    applyTreePalette();
    applyDashboardTheme();
    if (tree_ != nullptr) tree_->Refresh();
    Refresh();
    Layout();
}

}  // namespace tracker::ui
