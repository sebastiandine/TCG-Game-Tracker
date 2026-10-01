#include "tracker/ui/DeckStatisticsPanel.hpp"
#include "tracker/ui/AppContext.hpp"
#include "tracker/ui/Theme.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include <wx/button.h>
#include <wx/cursor.h>
#include <wx/listctrl.h>
#include <wx/scrolwin.h>
#include <wx/simplebook.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tglbtn.h>

namespace tracker::ui {

namespace {

enum {
    kColName = 0,
    kColGames,
    kColWins,
    kColLosses,
    kColDraws,
    kColWinPct
};

std::string toLowerAscii(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s)
        out.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return out;
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

wxListCtrl* makeMatchupList(wxWindow* parent, const char* firstColumn) {
    auto* list = new wxListCtrl(parent, wxID_ANY, wxDefaultPosition,
                                wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL);
    list->AppendColumn(wxString::FromUTF8(firstColumn), wxLIST_FORMAT_LEFT,
                       220);
    list->AppendColumn("Games", wxLIST_FORMAT_RIGHT, 70);
    list->AppendColumn("Wins", wxLIST_FORMAT_RIGHT, 70);
    list->AppendColumn("Losses", wxLIST_FORMAT_RIGHT, 70);
    list->AppendColumn("Draws", wxLIST_FORMAT_RIGHT, 70);
    list->AppendColumn("Win %", wxLIST_FORMAT_RIGHT, 80);
    return list;
}

int compareMatchup(const MatchupStats& a, const MatchupStats& b, int col) {
    switch (col) {
        case kColName: {
            const auto la = toLowerAscii(a.label);
            const auto lb = toLowerAscii(b.label);
            if (la < lb) return -1;
            if (la > lb) return 1;
            return 0;
        }
        case kColGames:
            if (a.record.total() < b.record.total()) return -1;
            if (a.record.total() > b.record.total()) return 1;
            return 0;
        case kColWins:
            if (a.record.wins < b.record.wins) return -1;
            if (a.record.wins > b.record.wins) return 1;
            return 0;
        case kColLosses:
            if (a.record.losses < b.record.losses) return -1;
            if (a.record.losses > b.record.losses) return 1;
            return 0;
        case kColDraws:
            if (a.record.draws < b.record.draws) return -1;
            if (a.record.draws > b.record.draws) return 1;
            return 0;
        case kColWinPct:
        default: {
            const double pa = a.record.winPercentage().value_or(-1.0);
            const double pb = b.record.winPercentage().value_or(-1.0);
            if (pa < pb) return -1;
            if (pa > pb) return 1;
            if (a.record.total() < b.record.total()) return -1;
            if (a.record.total() > b.record.total()) return 1;
            return 0;
        }
    }
}

void sortMatchupRows(std::vector<MatchupStats>& rows, int col, bool asc) {
    std::stable_sort(rows.begin(), rows.end(),
                     [col, asc](const MatchupStats& a, const MatchupStats& b) {
                         const int cmp = compareMatchup(a, b, col);
                         if (cmp == 0)
                             return toLowerAscii(a.label) < toLowerAscii(b.label);
                         return asc ? cmp < 0 : cmp > 0;
                     });
}

void fillMatchupList(wxListCtrl* list, const std::vector<MatchupStats>& rows,
                     const ThemePalette& palette) {
    if (list == nullptr) return;
    list->Freeze();
    list->DeleteAllItems();
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        const long idx = list->InsertItem(
            static_cast<long>(i), wxString::FromUTF8(row.label.c_str()));
        list->SetItem(idx, kColGames,
                      wxString::Format("%d", row.record.total()));
        list->SetItem(idx, kColWins, wxString::Format("%d", row.record.wins));
        list->SetItem(idx, kColLosses,
                      wxString::Format("%d", row.record.losses));
        list->SetItem(idx, kColDraws, wxString::Format("%d", row.record.draws));
        list->SetItem(idx, kColWinPct, winPctLabel(row.record));
        list->SetItemTextColour(idx, palette.inputText);
        list->SetItemBackgroundColour(idx, palette.inputBg);
    }
    list->Thaw();
}

void paintList(wxListCtrl* list, const ThemePalette& palette) {
    if (list == nullptr) return;
    list->SetBackgroundColour(palette.inputBg);
    list->SetForegroundColour(palette.inputText);
    list->SetOwnBackgroundColour(palette.inputBg);
    list->SetOwnForegroundColour(palette.inputText);
    const long n = list->GetItemCount();
    for (long i = 0; i < n; ++i) {
        list->SetItemTextColour(i, palette.inputText);
        list->SetItemBackgroundColour(i, palette.inputBg);
    }
}

void restoreSort(wxListCtrl* list, int sortCol, bool sortAsc) {
    if (list == nullptr) return;
    list->ShowSortIndicator(sortCol, sortAsc);
    list->Refresh();
}

wxString notePreviewLine(const std::string& notes) {
    std::string line;
    line.reserve(notes.size());
    bool pendingSpace = false;
    for (char c : notes) {
        if (c == '\n' || c == '\r' || c == '\t') {
            pendingSpace = !line.empty();
            continue;
        }
        if (c == ' ' && (line.empty() || pendingSpace)) {
            pendingSpace = true;
            continue;
        }
        if (pendingSpace) {
            line.push_back(' ');
            pendingSpace = false;
        }
        line.push_back(c);
    }
    return wxString::FromUTF8(line.c_str());
}

wxColour notesRowFill(bool selected, Theme theme, const ThemePalette& palette) {
    const wxColour base = dashboardCardFill(theme, palette);
    const int delta = 22;
    auto clamp = [](int v) -> unsigned char {
        if (v < 0) return 0;
        if (v > 255) return 255;
        return static_cast<unsigned char>(v);
    };
    if (!selected) return base;
    if (theme == Theme::Dark) {
        return wxColour(clamp(base.Red() + delta), clamp(base.Green() + delta),
                        clamp(base.Blue() + delta));
    }
    return wxColour(clamp(base.Red() - delta), clamp(base.Green() - delta),
                    clamp(base.Blue() - delta));
}

void bindNoteRowCursor(wxWindow* window) {
    if (window == nullptr) return;
    window->SetCursor(wxCursor(wxCURSOR_HAND));
}

}  // namespace

DeckStatisticsPanel::DeckStatisticsPanel(wxWindow* parent, AppContext& ctx)
    : wxPanel(parent),
      ctx_(ctx) {
    Freeze();

    auto* root = new wxBoxSizer(wxVERTICAL);

    titleLabel_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
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

    auto* highlights = new wxBoxSizer(wxHORIZONTAL);
    buildRankedList(this, best_, "Best matchups", "No matchups yet");
    buildRankedList(this, worst_, "Worst matchups", "No matchups yet");
    addDashboardItems(highlights, {best_.panel, worst_.panel});
    root->Add(highlights, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    notesPanel_ = new wxPanel(this);
    bindDashboardCardChrome(notesPanel_);
    notesPanel_->SetMinSize(wxSize(-1, 240));
    auto* notesSizer = new wxBoxSizer(wxVERTICAL);
    notesHeading_ = new wxStaticText(notesPanel_, wxID_ANY, "Open Notes");
    wxFont notesHeadingFont = notesHeading_->GetFont();
    notesHeadingFont.MakeBold();
    notesHeading_->SetFont(notesHeadingFont);
    notesSizer->Add(notesHeading_, 0, wxLEFT | wxRIGHT | wxTOP, 10);

    notesEmpty_ = new wxStaticText(notesPanel_, wxID_ANY, "No open notes");
    notesSizer->Add(notesEmpty_, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 10);

    notesContent_ = new wxPanel(notesPanel_);
    auto* split = new wxBoxSizer(wxHORIZONTAL);

    notesListScroll_ = new wxScrolledWindow(notesContent_, wxID_ANY, wxDefaultPosition,
                                            wxDefaultSize, wxVSCROLL | wxBORDER_NONE);
    notesListScroll_->SetScrollRate(0, 10);
    notesListScroll_->SetMinSize(wxSize(220, 200));
    notesListHost_ = new wxPanel(notesListScroll_);
    notesListHost_->SetSizer(new wxBoxSizer(wxVERTICAL));
    auto* listScrollSizer = new wxBoxSizer(wxVERTICAL);
    listScrollSizer->Add(notesListHost_, 1, wxEXPAND);
    notesListScroll_->SetSizer(listScrollSizer);

    notesDetail_ = new wxPanel(notesContent_);
    auto* detailSizer = new wxBoxSizer(wxVERTICAL);
    auto* detailHeader = new wxBoxSizer(wxHORIZONTAL);
    detailOpponent_ = new wxStaticText(notesDetail_, wxID_ANY, wxEmptyString,
                                       wxDefaultPosition, wxDefaultSize,
                                       wxST_ELLIPSIZE_END);
    wxFont detailNameFont = detailOpponent_->GetFont();
    detailNameFont.MakeBold();
    detailOpponent_->SetFont(detailNameFont);
    detailDate_ = new wxStaticText(notesDetail_, wxID_ANY, wxEmptyString);
    detailApprove_ = new wxButton(notesDetail_, wxID_ANY, "Approve");
    detailApprove_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (selectedNoteIndex_ < 0 ||
            static_cast<std::size_t>(selectedNoteIndex_) >= openNotes_.size()) {
            return;
        }
        const OpenNote& note = openNotes_[static_cast<std::size_t>(selectedNoteIndex_)];
        onApproveNote(note.game, note.opponentDeck);
    });
    detailHeader->Add(detailOpponent_, 1, wxALIGN_CENTER_VERTICAL);
    detailHeader->Add(detailDate_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    detailHeader->Add(detailApprove_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    detailSizer->Add(detailHeader, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    detailNotes_ = new wxTextCtrl(notesDetail_, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                  wxDefaultSize,
                                  wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    detailSizer->Add(detailNotes_, 1, wxEXPAND | wxALL, 8);
    notesDetail_->SetSizer(detailSizer);

    split->Add(notesListScroll_, 1, wxEXPAND);
    split->Add(notesDetail_, 2, wxEXPAND | wxLEFT, 8);
    notesContent_->SetSizer(split);
    notesContent_->Hide();
    notesSizer->Add(notesContent_, 1, wxEXPAND | wxBOTTOM, 8);
    notesPanel_->SetSizer(notesSizer);
    root->Add(notesPanel_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
    archetypesBtn_ = new wxToggleButton(this, wxID_ANY, "Archetypes");
    decksBtn_ = new wxToggleButton(this, wxID_ANY, "Decks");
    archetypesBtn_->SetValue(true);
    decksBtn_->SetValue(false);
    toolbar->Add(archetypesBtn_, 0, wxALIGN_CENTER_VERTICAL);
    toolbar->Add(decksBtn_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    toolbar->AddStretchSpacer(1);
    filterInput_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                  wxSize(220, -1), wxTE_RICH2);
    installTextCtrlPlaceholder(filterInput_, "Filter matchups...");
    toolbar->Add(filterInput_, 0,
                 wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM, 0);
    root->Add(toolbar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    matchupBook_ = new wxSimplebook(this, wxID_ANY);

    auto* archPage = new wxPanel(matchupBook_);
    auto* archSizer = new wxBoxSizer(wxVERTICAL);
    vsArchetypes_.list = makeMatchupList(archPage, "Archetype");
    vsArchetypes_.sortCol = kColWinPct;
    vsArchetypes_.sortAsc = false;
    bindTable(vsArchetypes_);
    archSizer->Add(vsArchetypes_.list, 1, wxEXPAND);
    archPage->SetSizer(archSizer);

    auto* deckPage = new wxPanel(matchupBook_);
    auto* deckSizer = new wxBoxSizer(wxVERTICAL);
    vsDecks_.list = makeMatchupList(deckPage, "Deck");
    vsDecks_.sortCol = kColWinPct;
    vsDecks_.sortAsc = false;
    bindTable(vsDecks_);
    deckSizer->Add(vsDecks_.list, 1, wxEXPAND);
    deckPage->SetSizer(deckSizer);

    matchupBook_->AddPage(archPage, "Archetypes");
    matchupBook_->AddPage(deckPage, "Decks");
    root->Add(matchupBook_, 1, wxEXPAND | wxALL, 12);

    SetSizer(root);

    archetypesBtn_->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent&) {
        selectMatchupView(true);
    });
    decksBtn_->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent&) {
        selectMatchupView(false);
    });
    filterInput_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
        onFilterChanged();
    });

    applyTheme();
    Thaw();
}

void DeckStatisticsPanel::applyDashboardTheme() {
    const Theme theme = ctx_.config.current().theme;
    const ThemePalette palette = paletteForTheme(theme);
    paintStatCard(gamesCard_, palette.text, theme, palette);
    paintStatCard(winsCard_, dashboardPositiveText(theme), theme, palette);
    paintStatCard(lossesCard_, dashboardNegativeText(theme), theme, palette);
    paintStatCard(drawsCard_, palette.text, theme, palette);
    paintStatCard(winPctCard_, dashboardWinPctAccent(theme, overview_), theme,
                  palette);
    paintRankedList(best_, dashboardPositiveText(theme), theme, palette);
    paintRankedList(worst_, dashboardNegativeText(theme), theme, palette);
    applyOpenNotesTheme();
}

void DeckStatisticsPanel::applyOpenNotesTheme() {
    const Theme theme = ctx_.config.current().theme;
    const ThemePalette palette = paletteForTheme(theme);
    const wxColour fill = dashboardCardFill(theme, palette);
    const wxColour muted = dashboardMutedText(theme);
    paintDashboardSurface(notesPanel_, fill, palette.text);
    paintDashboardSurface(notesHeading_, fill, palette.text);
    paintDashboardSurface(notesEmpty_, fill, muted);
    paintDashboardSurface(notesContent_, fill, palette.text);
    paintDashboardSurface(notesListScroll_, fill, palette.text);
    paintDashboardSurface(notesListHost_, fill, palette.text);
    paintDashboardSurface(notesDetail_, fill, palette.text);
    paintDashboardSurface(detailOpponent_, fill, palette.text);
    paintDashboardSurface(detailDate_, fill, muted);
    if (detailNotes_ != nullptr) {
        applyPaletteToTextCtrl(detailNotes_, palette, theme);
    }
    for (std::size_t i = 0; i < noteRows_.size(); ++i) {
        auto& row = noteRows_[i];
        const bool selected = static_cast<int>(i) == selectedNoteIndex_;
        const wxColour rowFill = notesRowFill(selected, theme, palette);
        paintDashboardSurface(row.row, rowFill, palette.text);
        paintDashboardSurface(row.opponent, rowFill, palette.text);
        paintDashboardSurface(row.date, rowFill, muted);
        paintDashboardSurface(row.preview, rowFill, muted);
    }
    if (notesPanel_ != nullptr) notesPanel_->Refresh();
}

void DeckStatisticsPanel::selectOpenNote(int index) {
    if (openNotes_.empty()) {
        selectedNoteIndex_ = -1;
        if (detailOpponent_ != nullptr) detailOpponent_->SetLabel(wxEmptyString);
        if (detailDate_ != nullptr) detailDate_->SetLabel(wxEmptyString);
        if (detailNotes_ != nullptr) detailNotes_->ChangeValue(wxEmptyString);
        applyOpenNotesTheme();
        return;
    }
    if (index < 0) index = 0;
    if (index >= static_cast<int>(openNotes_.size())) {
        index = static_cast<int>(openNotes_.size()) - 1;
    }
    selectedNoteIndex_ = index;
    const OpenNote& note = openNotes_[static_cast<std::size_t>(index)];
    if (detailOpponent_ != nullptr) {
        detailOpponent_->SetLabel(wxString::FromUTF8(note.opponentDeck.c_str()));
    }
    if (detailDate_ != nullptr) {
        detailDate_->SetLabel(wxString::FromUTF8(note.game.playedOn.c_str()));
    }
    if (detailNotes_ != nullptr) {
        detailNotes_->ChangeValue(wxString::FromUTF8(note.game.notes.c_str()));
    }
    applyOpenNotesTheme();
    if (notesContent_ != nullptr) notesContent_->Layout();
}

void DeckStatisticsPanel::refillOpenNotes(const std::vector<OpenNote>& notes) {
    noteRows_.clear();
    openNotes_ = notes;
    if (notesListHost_ == nullptr || notesListScroll_ == nullptr ||
        notesEmpty_ == nullptr || notesContent_ == nullptr ||
        notesHeading_ == nullptr) {
        return;
    }

    notesListHost_->Freeze();
    auto* hostSizer = notesListHost_->GetSizer();
    if (hostSizer == nullptr) {
        hostSizer = new wxBoxSizer(wxVERTICAL);
        notesListHost_->SetSizer(hostSizer);
    } else {
        hostSizer->Clear(true);
    }

    const bool empty = notes.empty();
    if (empty) {
        notesHeading_->SetLabel("Open Notes");
    } else {
        notesHeading_->SetLabel(
            wxString::Format("Open Notes (%zu)", notes.size()));
    }
    notesEmpty_->Show(empty);
    notesContent_->Show(!empty);
    notesPanel_->SetMinSize(wxSize(-1, empty ? -1 : 240));

    if (!empty) {
        for (std::size_t i = 0; i < notes.size(); ++i) {
            const auto& note = notes[i];
            auto* row = new wxPanel(notesListHost_);
            bindNoteRowCursor(row);
            auto* col = new wxBoxSizer(wxVERTICAL);

            auto* opponent = new wxStaticText(
                row, wxID_ANY, wxString::FromUTF8(note.opponentDeck.c_str()),
                wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
            wxFont nameFont = opponent->GetFont();
            nameFont.MakeBold();
            opponent->SetFont(nameFont);
            bindNoteRowCursor(opponent);

            auto* date = new wxStaticText(
                row, wxID_ANY, wxString::FromUTF8(note.game.playedOn.c_str()));
            bindNoteRowCursor(date);

            auto* preview = new wxStaticText(
                row, wxID_ANY, notePreviewLine(note.game.notes),
                wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
            bindNoteRowCursor(preview);

            const int index = static_cast<int>(i);
            auto bindSelect = [this, index](wxWindow* window) {
                window->Bind(wxEVT_LEFT_DOWN, [this, index](wxMouseEvent&) {
                    selectOpenNote(index);
                });
            };
            bindSelect(row);
            bindSelect(opponent);
            bindSelect(date);
            bindSelect(preview);

            col->Add(opponent, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
            col->Add(date, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
            col->Add(preview, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
            row->SetSizer(col);
            hostSizer->Add(row, 0, wxEXPAND | wxBOTTOM, 4);
            noteRows_.push_back(OpenNoteRow{row, opponent, date, preview});
        }
    }

    notesListHost_->Fit();
    notesListScroll_->FitInside();
    notesListHost_->Thaw();

    int select = 0;
    if (approvedNoteIndex_ >= 0) {
        if (static_cast<std::size_t>(approvedNoteIndex_) < notes.size()) {
            select = approvedNoteIndex_;
        } else if (!notes.empty()) {
            select = static_cast<int>(notes.size()) - 1;
        }
        approvedNoteIndex_ = -1;
    } else if (selectedNoteIndex_ >= 0 &&
               static_cast<std::size_t>(selectedNoteIndex_) < notes.size()) {
        select = selectedNoteIndex_;
    }

    if (empty) {
        selectedNoteIndex_ = -1;
        applyOpenNotesTheme();
    } else {
        selectOpenNote(select);
    }
    if (notesPanel_ != nullptr) notesPanel_->Layout();
}

void DeckStatisticsPanel::onApproveNote(Game game,
                                        const std::string& opponentDeck) {
    const wxString message = wxString::Format(
        "Clear this note from the game vs %s? You can copy the text first. "
        "This cannot be undone.",
        wxString::FromUTF8(opponentDeck.c_str()));
    if (showThemedConfirmDialog(this, message, "Approve note") != wxID_YES) {
        return;
    }

    approvedNoteIndex_ = selectedNoteIndex_;

    auto result = ctx_.games.clearNotes(game);
    if (!result) {
        approvedNoteIndex_ = -1;
        showThemedMessageDialog(this,
                                wxString::FromUTF8(result.error().c_str()),
                                "Error", wxOK);
        return;
    }
    CallAfter([this] { refresh(); });
}

void DeckStatisticsPanel::bindTable(MatchupTable& table) {
    if (table.list == nullptr) return;
    table.list->Bind(wxEVT_LIST_COL_CLICK, &DeckStatisticsPanel::onColumnClick,
                     this);
}

void DeckStatisticsPanel::refillTable(MatchupTable& table) {
    sortMatchupRows(table.rows, table.sortCol, table.sortAsc);
    std::vector<MatchupStats> visible;
    visible.reserve(table.rows.size());
    for (const auto& row : table.rows) {
        if (containsCI(row.label, filter_)) visible.push_back(row);
    }
    const ThemePalette palette =
        paletteForTheme(ctx_.config.current().theme);
    fillMatchupList(table.list, visible, palette);
    restoreSort(table.list, table.sortCol, table.sortAsc);
}

DeckStatisticsPanel::MatchupTable* DeckStatisticsPanel::tableFor(
    wxListCtrl* list) {
    if (list == vsArchetypes_.list) return &vsArchetypes_;
    if (list == vsDecks_.list) return &vsDecks_;
    return nullptr;
}

void DeckStatisticsPanel::selectMatchupView(bool archetypes) {
    if (archetypesBtn_ != nullptr) archetypesBtn_->SetValue(archetypes);
    if (decksBtn_ != nullptr) decksBtn_->SetValue(!archetypes);
    if (matchupBook_ != nullptr) {
        matchupBook_->ChangeSelection(archetypes ? 0 : 1);
    }
    const MatchupTable& table = archetypes ? vsArchetypes_ : vsDecks_;
    restoreSort(table.list, table.sortCol, table.sortAsc);
}

void DeckStatisticsPanel::onFilterChanged() {
    if (filterInput_ == nullptr) return;
    filter_ = filterInput_->GetValue().ToStdString(wxConvUTF8);
    refillTable(vsArchetypes_);
    refillTable(vsDecks_);
}

void DeckStatisticsPanel::onColumnClick(wxListEvent& evt) {
    auto* list = dynamic_cast<wxListCtrl*>(evt.GetEventObject());
    MatchupTable* table = tableFor(list);
    if (table == nullptr) return;

    const int col = evt.GetColumn();
    if (col < 0) return;
    if (col == table->sortCol) {
        table->sortAsc = !table->sortAsc;
    } else {
        table->sortCol = col;
        table->sortAsc = (col == kColName);
    }
    refillTable(*table);
}

void DeckStatisticsPanel::load(std::int64_t formatId, const DeckStatsKey& key) {
    formatId_ = formatId;
    key_ = key;
    refresh();
}

void DeckStatisticsPanel::refresh() {
    DeckDetailStatistics stats;
    if (formatId_ > 0) {
        auto deckResult = ctx_.decks.listByFormat(formatId_);
        auto gameResult = ctx_.games.listByFormat(formatId_);
        auto archResult = ctx_.decks.listByGame(
            ctx_.config.current().selectedGameId);
        if (deckResult && gameResult && archResult) {
            stats = computeDeckDetailStatistics(deckResult.value(),
                                                archResult.value(),
                                                gameResult.value(), key_);
        }
    }

    title_ = stats.title;
    if (titleLabel_ != nullptr) {
        titleLabel_->SetLabel(wxString::FromUTF8(title_.c_str()));
    }

    overview_ = stats.overview;
    setStatCardValue(gamesCard_, wxString::Format("%d", overview_.total()));
    setStatCardValue(winsCard_, wxString::Format("%d", overview_.wins));
    setStatCardValue(lossesCard_, wxString::Format("%d", overview_.losses));
    setStatCardValue(drawsCard_, wxString::Format("%d", overview_.draws));
    setStatCardValue(winPctCard_, winPctLabel(overview_));

    fillRankedList(best_, stats.bestMatchups);
    fillRankedList(worst_, stats.worstMatchups);
    refillOpenNotes(stats.openNotes);

    vsArchetypes_.rows = stats.vsArchetypes;
    vsDecks_.rows = stats.vsDecks;
    refillTable(vsArchetypes_);
    refillTable(vsDecks_);

    applyTheme();
    Layout();
}

void DeckStatisticsPanel::applyListPalette() {
    const ThemePalette palette =
        paletteForTheme(ctx_.config.current().theme);
    paintList(vsArchetypes_.list, palette);
    paintList(vsDecks_.list, palette);
}

void DeckStatisticsPanel::applyTheme() {
    const Theme theme = ctx_.config.current().theme;
    const ThemePalette palette = paletteForTheme(theme);
    applyThemeToWindowTree(this, palette, theme);
    if (filterInput_ != nullptr) {
        applyPaletteToTextCtrl(filterInput_, palette, theme);
    }
    if (detailNotes_ != nullptr) {
        applyPaletteToTextCtrl(detailNotes_, palette, theme);
    }
    applyListPalette();
    applyDashboardTheme();
    restoreSort(vsArchetypes_.list, vsArchetypes_.sortCol, vsArchetypes_.sortAsc);
    restoreSort(vsDecks_.list, vsDecks_.sortCol, vsDecks_.sortAsc);
    Refresh();
    Layout();
}

}  // namespace tracker::ui
