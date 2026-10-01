#include "tracker/ui/GamesPanel.hpp"
#include "tracker/ui/AppContext.hpp"
#include "tracker/ui/GameDialog.hpp"
#include "tracker/ui/Theme.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <map>
#include <optional>
#include <string>

#include <wx/bmpbuttn.h>
#include <wx/bmpbndl.h>
#include <wx/image.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>

namespace tracker::ui {

namespace {

constexpr int kToolbarIconPx = 16;
constexpr int kColumnCount = 7;

// VscAdd SVG from vscode-codicons (MIT). @FILL@ is replaced at runtime.
const char* const kSvgAdd = R"SVG(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16">
  <path fill="@FILL@" d="M8 1.5C8 1.22386 7.77614 1 7.5 1C7.22386 1 7 1.22386 7 1.5V7H1.5C1.22386 7 1 7.22386 1 7.5C1 7.77614 1.22386 8 1.5 8H7V13.5C7 13.7761 7.22386 14 7.5 14C7.77614 14 8 13.7761 8 13.5V8H13.5C13.7761 8 14 7.77614 14 7.5C14 7.22386 13.7761 7 13.5 7H8V1.5Z"/>
</svg>)SVG";

wxBitmap renderSvg(const char* svgTemplate, int size, const char* fillHex) {
    std::string s = svgTemplate;
    constexpr std::string_view kPlaceholder = "@FILL@";
    for (auto pos = s.find(kPlaceholder);
         pos != std::string::npos;
         pos = s.find(kPlaceholder, pos + std::strlen(fillHex))) {
        s.replace(pos, kPlaceholder.size(), fillHex);
    }
    auto bundle = wxBitmapBundle::FromSVG(
        reinterpret_cast<const wxByte*>(s.data()),
        s.size(), wxSize(size, size));
    if (bundle.IsOk()) return bundle.GetBitmap(wxSize(size, size));
    wxImage img(size, size);
    img.SetAlpha();
    if (auto* a = img.GetAlpha()) std::fill(a, a + size * size, static_cast<unsigned char>(0));
    return wxBitmap(img);
}

bool containsCI(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a)) ==
                   std::tolower(static_cast<unsigned char>(b));
        });
    return it != haystack.end();
}

// Resolve a deckId to its display name, with variant in parentheses if present.
std::string deckDisplayName(const std::vector<Deck>& decks, std::int64_t id) {
    for (const auto& d : decks) {
        if (d.id == id) {
            if (d.variant.empty()) return d.name;
            return d.name + " (" + d.variant + ")";
        }
    }
    return "?";
}

std::string gameTypeName(const std::vector<GameType>& types, std::int64_t id) {
    for (const auto& gt : types) {
        if (gt.id == id) return gt.name;
    }
    return "?";
}

}  // namespace

GamesPanel::GamesPanel(wxWindow* parent, AppContext& ctx)
    : wxPanel(parent),
      ctx_(ctx) {
    auto* root = new wxBoxSizer(wxVERTICAL);
    buildToolbar(root);
    buildList(root);
    SetSizer(root);
}

void GamesPanel::buildToolbar(wxSizer* parent) {
    auto* toolbar = new wxBoxSizer(wxHORIZONTAL);

    addButton_ = new wxBitmapButton(
        this, wxID_ANY,
        renderSvg(kSvgAdd, kToolbarIconPx, "#000000"),
        wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    addButton_->SetToolTip("Add Game");
    toolbar->AddSpacer(4);
    toolbar->Add(addButton_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    toolbar->AddStretchSpacer(1);

    filterInput_ = new wxTextCtrl(this, wxID_ANY, "",
                                  wxDefaultPosition, wxSize(260, -1),
                                  wxTE_RICH2);
    installTextCtrlPlaceholder(filterInput_, "Filter games...");
    toolbar->Add(filterInput_, 0,
                 wxALIGN_CENTER_VERTICAL | wxRIGHT | wxTOP | wxBOTTOM, 4);

    parent->Add(toolbar, 0, wxEXPAND);

    addButton_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onAdd(); });
    filterInput_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
        if (filterInput_ == nullptr) return;
        filter_ = filterInput_->GetValue().ToStdString(wxConvUTF8);
        rebuildRows();
    });
}

void GamesPanel::buildList(wxSizer* parent) {
    list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxLC_REPORT | wxLC_SINGLE_SEL);

    list_->AppendColumn("Date", wxLIST_FORMAT_LEFT, 90);
    list_->AppendColumn("Deck", wxLIST_FORMAT_LEFT, 150);
    list_->AppendColumn("Opponent Deck", wxLIST_FORMAT_LEFT, 130);
    list_->AppendColumn("Result", wxLIST_FORMAT_LEFT, 90);
    list_->AppendColumn("Game Type", wxLIST_FORMAT_LEFT, 100);
    list_->AppendColumn("Opponent", wxLIST_FORMAT_LEFT, 110);
    list_->AppendColumn("Notes", wxLIST_FORMAT_LEFT, 160);

    parent->Add(list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

    list_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &GamesPanel::onRowActivated, this);
    list_->Bind(wxEVT_LIST_COL_CLICK, &GamesPanel::onColumnClick, this);
    installActionableRowCursor(list_, [this](const wxPoint& pos) {
        int flags = 0;
        const long item = list_->HitTest(pos, flags);
        return item != wxNOT_FOUND && (flags & wxLIST_HITTEST_ONITEM) != 0;
    });
}

void GamesPanel::onAdd() {
    if (formatId_ <= 0) return;

    // Refresh decks so any newly created decks are available.
    auto deckResult = ctx_.decks.listByFormat(formatId_);
    if (!deckResult) return;
    decks_ = std::move(deckResult.value());

    if (decks_.empty()) {
        showThemedMessageDialog(
            this, "Please add at least one deck first.",
            "No Decks", wxOK | wxICON_INFORMATION);
        return;
    }

    // Refresh game types.
    auto gtResult = ctx_.gameTypes.listByGame(ctx_.config.current().selectedGameId);
    if (!gtResult) return;
    gameTypes_ = std::move(gtResult.value());

    if (gameTypes_.empty()) {
        showThemedMessageDialog(
            this, "Please add at least one game type first.",
            "No Game Types", wxOK | wxICON_INFORMATION);
        return;
    }

    GameDialog dlg(this, ctx_, formatId_, decks_, gameTypes_,
                   std::nullopt, lastDeckId_, lastGameTypeId_);
    themeModalDialog(&dlg, ctx_.config.current().theme);
    if (dlg.ShowModal() == wxID_OK) {
        lastDeckId_ = dlg.savedGame().deckId;
        lastGameTypeId_ = dlg.savedGame().gameTypeId;
        refreshList();
    }
}

void GamesPanel::onRowActivated(wxListEvent& evt) {
    if (rebuildingList_) return;
    const long idx = evt.GetIndex();
    if (idx < 0 || static_cast<std::size_t>(idx) >= visible_.size()) return;

    const Game& game = *visible_[idx];

    auto deckResult = ctx_.decks.listByFormat(formatId_);
    if (!deckResult) return;
    decks_ = std::move(deckResult.value());

    auto gtResult = ctx_.gameTypes.listByGame(ctx_.config.current().selectedGameId);
    if (!gtResult) return;
    gameTypes_ = std::move(gtResult.value());

    GameDialog dlg(this, ctx_, formatId_, decks_, gameTypes_,
                   game, lastDeckId_, lastGameTypeId_);
    themeModalDialog(&dlg, ctx_.config.current().theme);
    if (dlg.ShowModal() == wxID_OK) {
        lastDeckId_ = dlg.savedGame().deckId;
        lastGameTypeId_ = dlg.savedGame().gameTypeId;
        refreshList();
    }
}

void GamesPanel::onColumnClick(wxListEvent& evt) {
    const int col = evt.GetColumn();
    if (col == sortCol_) {
        sortAsc_ = !sortAsc_;
    } else {
        sortCol_ = col;
        sortAsc_ = true;
    }
    list_->ShowSortIndicator(sortCol_, sortAsc_);
    rebuildRows();
}

wxString GamesPanel::cellText(const Game& game, int col) const {
    switch (col) {
        case 0: return wxString::FromUTF8(game.playedOn.c_str());
        case 1: return wxString::FromUTF8(deckDisplayName(decks_, game.deckId).c_str());
        case 2: return wxString::FromUTF8(deckDisplayName(decks_, game.opponentDeckId).c_str());
        case 3: {
            std::string combined = std::string(to_string(game.result))
                + " (" + std::string(to_string(game.score)) + ")";
            return wxString::FromUTF8(combined.c_str());
        }
        case 4: return wxString::FromUTF8(gameTypeName(gameTypes_, game.gameTypeId).c_str());
        case 5: return wxString::FromUTF8(game.opponent.c_str());
        case 6: return wxString::FromUTF8(game.notes.c_str());
        default: return "";
    }
}

bool GamesPanel::matchesFilter(const Game& game) const {
    if (filter_.empty()) return true;
    for (int col = 0; col < kColumnCount; ++col) {
        const std::string text = cellText(game, col).ToStdString(wxConvUTF8);
        if (containsCI(text, filter_)) return true;
    }
    return false;
}

void GamesPanel::refreshList() {
    auto result = ctx_.games.listByFormat(formatId_);
    if (!result) return;
    allGames_ = std::move(result.value());

    auto deckResult = ctx_.decks.listByFormat(formatId_);
    if (deckResult) {
        decks_ = std::move(deckResult.value());
    }

    auto gtResult = ctx_.gameTypes.listByGame(ctx_.config.current().selectedGameId);
    if (gtResult) {
        gameTypes_ = std::move(gtResult.value());
    }

    // Seed lastDeckId_ from the newest game if we haven't set one yet.
    if (lastDeckId_ == 0 && !allGames_.empty()) {
        lastDeckId_ = allGames_.front().deckId;
    }
    if (lastGameTypeId_ == 0 && !allGames_.empty()) {
        lastGameTypeId_ = allGames_.front().gameTypeId;
    }

    rebuildRows();
}

void GamesPanel::rebuildRows() {
    if (list_ == nullptr) return;

    rebuildingList_ = true;
    list_->Freeze();
    list_->DeleteAllItems();

    // Filter.
    visible_.clear();
    for (auto& g : allGames_) {
        if (matchesFilter(g)) visible_.push_back(&g);
    }

    // Sort.
    if (sortCol_ >= 0) {
        std::stable_sort(visible_.begin(), visible_.end(),
                         [this](const Game* a, const Game* b) {
                             const wxString ta = cellText(*a, sortCol_);
                             const wxString tb = cellText(*b, sortCol_);
                             const int cmp = ta.CmpNoCase(tb);
                             return sortAsc_ ? cmp < 0 : cmp > 0;
                         });
    }

    // Insert rows.
    for (std::size_t row = 0; row < visible_.size(); ++row) {
        const auto& g = *visible_[row];
        const long item = list_->InsertItem(static_cast<long>(row),
                                            cellText(g, 0));
        for (int col = 1; col < kColumnCount; ++col) {
            list_->SetItem(item, col, cellText(g, col));
        }
    }

    list_->Thaw();
    rebuildingList_ = false;
}

void GamesPanel::loadFormat(std::int64_t formatId) {
    formatId_ = formatId;
    lastDeckId_ = 0;
    lastGameTypeId_ = 0;
    sortCol_ = -1;
    sortAsc_ = false;
    filter_.clear();
    if (filterInput_) filterInput_->ChangeValue("");
    if (list_) list_->RemoveSortIndicator();
    refreshList();
}

void GamesPanel::applyTheme() {
    const Theme theme = ctx_.config.current().theme;
    const ThemePalette palette = paletteForTheme(theme);
    applyThemeToWindowTree(this, palette, theme);

    // Re-render the SVG add button with the current text color.
    if (addButton_) {
        const std::string hex =
            palette.buttonText.GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
        addButton_->SetBitmap(renderSvg(kSvgAdd, kToolbarIconPx, hex.c_str()));
    }

    // Filter input.
    if (filterInput_) {
        applyPaletteToTextCtrl(filterInput_, palette, theme);
    }

    // List.
    if (list_) {
        list_->SetBackgroundColour(palette.inputBg);
        list_->SetForegroundColour(palette.inputText);
        list_->Refresh();
    }

    Refresh();
}

}  // namespace tracker::ui
