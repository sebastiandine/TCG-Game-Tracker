#include "tracker/ui/GameDialog.hpp"
#include "tracker/ui/AppContext.hpp"
#include "tracker/ui/Theme.hpp"
#include "tracker/domain/DeckGroup.hpp"

#include <algorithm>
#include <map>
#include <string>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/datectrl.h>
#include <wx/dateevt.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace tracker::ui {

namespace {

wxString dateToWxString(const wxDateTime& dt) {
    return dt.FormatISODate();
}

wxDateTime wxDateFromString(const std::string& iso) {
    wxDateTime dt;
    dt.ParseISODate(wxString::FromUTF8(iso.c_str()));
    return dt;
}

// Combined result/score entries shown in the single dropdown.
struct ResultScoreEntry {
    MatchResult result;
    MatchScore  score;
    std::string label;  // e.g. "Win (2-1)"
};

std::vector<ResultScoreEntry> buildResultScoreEntries() {
    std::vector<ResultScoreEntry> entries;
    for (const auto& s : allMatchScores()) {
        const MatchResult r = resultForScore(s);
        entries.push_back({r, s,
            std::string(to_string(r)) + " (" + std::string(to_string(s)) + ")"});
    }
    return entries;
}

}  // namespace

GameDialog::GameDialog(wxWindow* parent, AppContext& ctx,
                       std::int64_t formatId,
                       const std::vector<Deck>& decks,
                       const std::vector<GameType>& gameTypes,
                       const std::optional<Game>& game,
                       std::int64_t lastDeckId,
                       std::int64_t lastGameTypeId)
    : wxDialog(parent, wxID_ANY,
               game.has_value() ? "Edit Game" : "Add Game",
               wxDefaultPosition, wxSize(520, 460),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      ctx_(ctx),
      formatId_(formatId),
      decks_(decks),
      gameTypes_(gameTypes),
      existing_(game) {

    Freeze();

    // Build unique name list, preserving insertion order from grouped decks.
    const auto groups = groupDecksByName(decks_);
    for (const auto& group : groups) {
        deckNames_.push_back(group.name);
    }

    // Build variantsByName_ from the actual decks_ vector.
    for (const auto& group : groups) {
        std::vector<const Deck*> vars;
        for (const auto& d : decks_) {
            if (d.name.size() == group.name.size()) {
                bool match = true;
                for (std::size_t c = 0; c < d.name.size(); ++c) {
                    if (std::tolower(static_cast<unsigned char>(d.name[c])) !=
                        std::tolower(static_cast<unsigned char>(group.name[c]))) {
                        match = false;
                        break;
                    }
                }
                if (match) vars.push_back(&d);
            }
        }
        variantsByName_.push_back(std::move(vars));
    }

    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* grid = new wxFlexGridSizer(2, 8, 10);
    grid->AddGrowableCol(1, 1);

    // Date
    grid->Add(new wxStaticText(this, wxID_ANY, "Date:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    dateCtrl_ = new wxDatePickerCtrl(this, wxID_ANY);
    if (existing_.has_value()) {
        dateCtrl_->SetValue(wxDateFromString(existing_->playedOn));
    }
    grid->Add(dateCtrl_, 1, wxEXPAND);

    // Deck (unique names)
    grid->Add(new wxStaticText(this, wxID_ANY, "Deck:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    deckChoice_ = new wxChoice(this, wxID_ANY);
    {
        wxArrayString items;
        for (const auto& name : deckNames_) {
            items.Add(wxString::FromUTF8(name.c_str()));
        }
        deckChoice_->Append(items);

        int defaultIdx = 0;
        if (existing_.has_value()) {
            for (std::size_t i = 0; i < variantsByName_.size(); ++i) {
                for (const auto* d : variantsByName_[i]) {
                    if (d->id == existing_->deckId) {
                        defaultIdx = static_cast<int>(i);
                        goto deckFound;
                    }
                }
            }
        } else if (lastDeckId > 0) {
            for (std::size_t i = 0; i < variantsByName_.size(); ++i) {
                for (const auto* d : variantsByName_[i]) {
                    if (d->id == lastDeckId) {
                        defaultIdx = static_cast<int>(i);
                        goto deckFound;
                    }
                }
            }
        }
        deckFound:
        if (!deckNames_.empty()) {
            deckChoice_->SetSelection(defaultIdx);
        }
    }
    grid->Add(deckChoice_, 1, wxEXPAND);

    // Variante
    grid->Add(new wxStaticText(this, wxID_ANY, "Variante:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    variantChoice_ = new wxChoice(this, wxID_ANY);
    grid->Add(variantChoice_, 1, wxEXPAND);

    deckChoice_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
        rebuildVariantChoice();
    });
    rebuildVariantChoice();

    // Select the correct variant for edit mode or last-used.
    if (existing_.has_value()) {
        for (const int nameIdx = deckChoice_->GetSelection();
             nameIdx >= 0 && nameIdx < static_cast<int>(variantsByName_.size());) {
            for (std::size_t vi = 0; vi < variantsByName_[nameIdx].size(); ++vi) {
                if (variantsByName_[nameIdx][vi]->id == existing_->deckId) {
                    variantChoice_->SetSelection(static_cast<int>(vi));
                    break;
                }
            }
            break;
        }
    } else if (lastDeckId > 0) {
        const int nameIdx = deckChoice_->GetSelection();
        if (nameIdx >= 0 && nameIdx < static_cast<int>(variantsByName_.size())) {
            for (std::size_t vi = 0; vi < variantsByName_[nameIdx].size(); ++vi) {
                if (variantsByName_[nameIdx][vi]->id == lastDeckId) {
                    variantChoice_->SetSelection(static_cast<int>(vi));
                    break;
                }
            }
        }
    }

    // Opponent deck (unique names only)
    grid->Add(new wxStaticText(this, wxID_ANY, "Opponent Deck:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    opponentChoice_ = new wxChoice(this, wxID_ANY);
    {
        wxArrayString items;
        for (const auto& name : deckNames_) {
            items.Add(wxString::FromUTF8(name.c_str()));
        }
        opponentChoice_->Append(items);

        int oppIdx = 0;
        if (existing_.has_value()) {
            for (std::size_t i = 0; i < variantsByName_.size(); ++i) {
                for (const auto* d : variantsByName_[i]) {
                    if (d->id == existing_->opponentDeckId) {
                        oppIdx = static_cast<int>(i);
                        goto oppFound;
                    }
                }
            }
        }
        oppFound:
        if (!deckNames_.empty()) {
            opponentChoice_->SetSelection(oppIdx);
        }
    }
    grid->Add(opponentChoice_, 1, wxEXPAND);

    // Result (combined result + score, e.g. "Win (2-1)")
    grid->Add(new wxStaticText(this, wxID_ANY, "Result:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    resultScoreChoice_ = new wxChoice(this, wxID_ANY);
    {
        const auto entries = buildResultScoreEntries();
        wxArrayString items;
        for (const auto& e : entries) {
            items.Add(wxString::FromUTF8(e.label.c_str()));
        }
        resultScoreChoice_->Append(items);

        int selIdx = 0;
        if (existing_.has_value()) {
            for (std::size_t i = 0; i < entries.size(); ++i) {
                if (entries[i].result == existing_->result &&
                    entries[i].score == existing_->score) {
                    selIdx = static_cast<int>(i);
                    break;
                }
            }
        }
        resultScoreChoice_->SetSelection(selIdx);
    }
    grid->Add(resultScoreChoice_, 1, wxEXPAND);

    // Game Type
    grid->Add(new wxStaticText(this, wxID_ANY, "Game Type:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    gameTypeChoice_ = new wxChoice(this, wxID_ANY);
    {
        wxArrayString items;
        for (const auto& gt : gameTypes_) {
            items.Add(wxString::FromUTF8(gt.name.c_str()));
        }
        gameTypeChoice_->Append(items);

        int gtIdx = 0;
        if (existing_.has_value()) {
            for (std::size_t i = 0; i < gameTypes_.size(); ++i) {
                if (gameTypes_[i].id == existing_->gameTypeId) {
                    gtIdx = static_cast<int>(i);
                    break;
                }
            }
        } else if (lastGameTypeId > 0) {
            for (std::size_t i = 0; i < gameTypes_.size(); ++i) {
                if (gameTypes_[i].id == lastGameTypeId) {
                    gtIdx = static_cast<int>(i);
                    break;
                }
            }
        }
        if (!gameTypes_.empty()) {
            gameTypeChoice_->SetSelection(gtIdx);
        }
    }
    grid->Add(gameTypeChoice_, 1, wxEXPAND);

    // Opponent name
    grid->Add(new wxStaticText(this, wxID_ANY, "Opponent:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    opponentCtrl_ = new wxTextCtrl(
        this, wxID_ANY,
        existing_.has_value()
            ? wxString::FromUTF8(existing_->opponent.c_str())
            : wxString(""));
    grid->Add(opponentCtrl_, 1, wxEXPAND);

    // Notes
    grid->Add(new wxStaticText(this, wxID_ANY, "Notes:"), 0,
              wxALIGN_RIGHT | wxTOP, 2);
    notesCtrl_ = new wxTextCtrl(
        this, wxID_ANY,
        existing_.has_value()
            ? wxString::FromUTF8(existing_->notes.c_str())
            : wxString(""),
        wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE);
    grid->Add(notesCtrl_, 1, wxEXPAND);

    root->Add(grid, 1, wxEXPAND | wxALL, 12);

    auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
    if (btns) root->Add(btns, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    Bind(wxEVT_BUTTON, &GameDialog::onOk, this, wxID_OK);

    SetSizer(root);
    CentreOnParent();

    Thaw();
}

void GameDialog::rebuildVariantChoice() {
    if (variantChoice_ == nullptr || deckChoice_ == nullptr) return;

    variantChoice_->Clear();
    const int nameIdx = deckChoice_->GetSelection();
    if (nameIdx < 0 || nameIdx >= static_cast<int>(variantsByName_.size()))
        return;

    wxArrayString items;
    for (const auto* d : variantsByName_[nameIdx]) {
        if (d->variant.empty()) {
            items.Add(wxString::Format(
                "%s (default)", wxString::FromUTF8(d->name.c_str())));
        } else {
            items.Add(wxString::Format(
                "%s (variante)", wxString::FromUTF8(d->variant.c_str())));
        }
    }
    variantChoice_->Append(items);
    if (!items.empty()) {
        variantChoice_->SetSelection(0);
    }
}

void GameDialog::onOk(wxCommandEvent&) {
    // Date
    const wxDateTime dt = dateCtrl_->GetValue();
    const std::string playedOn =
        dateToWxString(dt).ToStdString(wxConvUTF8);

    // Deck
    const int nameIdx = deckChoice_->GetSelection();
    if (nameIdx < 0 || nameIdx >= static_cast<int>(variantsByName_.size())) {
        showThemedMessageDialog(this, "Please select a deck.",
                                "Validation Error", wxOK | wxICON_WARNING);
        return;
    }
    const int varIdx = variantChoice_->GetSelection();
    if (varIdx < 0 || varIdx >= static_cast<int>(variantsByName_[nameIdx].size())) {
        showThemedMessageDialog(this, "Please select a variante.",
                                "Validation Error", wxOK | wxICON_WARNING);
        return;
    }
    const std::int64_t deckId = variantsByName_[nameIdx][varIdx]->id;

    // Opponent name
    const std::string opponent =
        opponentCtrl_->GetValue().ToStdString(wxConvUTF8);

    // Opponent deck: pick the empty-variant row or first row for that name.
    const int oppIdx = opponentChoice_->GetSelection();
    if (oppIdx < 0 || oppIdx >= static_cast<int>(variantsByName_.size())) {
        showThemedMessageDialog(this, "Please select an opponent deck.",
                                "Validation Error", wxOK | wxICON_WARNING);
        return;
    }
    std::int64_t opponentDeckId = 0;
    for (const auto* d : variantsByName_[oppIdx]) {
        if (d->variant.empty()) {
            opponentDeckId = d->id;
            break;
        }
    }
    if (opponentDeckId == 0 && !variantsByName_[oppIdx].empty()) {
        opponentDeckId = variantsByName_[oppIdx][0]->id;
    }

    // Result + Score (combined)
    const int rsIdx = resultScoreChoice_->GetSelection();
    if (rsIdx < 0) {
        showThemedMessageDialog(this, "Please select a result.",
                                "Validation Error", wxOK | wxICON_WARNING);
        return;
    }
    const auto entries = buildResultScoreEntries();
    if (static_cast<std::size_t>(rsIdx) >= entries.size()) {
        showThemedMessageDialog(this, "Please select a result.",
                                "Validation Error", wxOK | wxICON_WARNING);
        return;
    }
    const MatchResult result = entries[rsIdx].result;
    const MatchScore score = entries[rsIdx].score;

    // Game Type
    const int gtIdx = gameTypeChoice_->GetSelection();
    if (gtIdx < 0 || static_cast<std::size_t>(gtIdx) >= gameTypes_.size()) {
        showThemedMessageDialog(this, "Please select a game type.",
                                "Validation Error", wxOK | wxICON_WARNING);
        return;
    }
    const std::int64_t gameTypeId = gameTypes_[gtIdx].id;

    // Notes
    const std::string notes =
        notesCtrl_->GetValue().ToStdString(wxConvUTF8);

    if (existing_.has_value()) {
        auto res = ctx_.games.update(existing_->id, playedOn, deckId,
                                     opponentDeckId, opponent, result, score,
                                     gameTypeId, notes);
        if (!res) {
            showThemedMessageDialog(
                this, wxString::FromUTF8(res.error().c_str()),
                "Error", wxOK | wxICON_ERROR);
            return;
        }
        saved_ = std::move(res.value());
    } else {
        auto res = ctx_.games.create(formatId_, playedOn, deckId,
                                     opponentDeckId, opponent, result, score,
                                     gameTypeId, notes);
        if (!res) {
            showThemedMessageDialog(
                this, wxString::FromUTF8(res.error().c_str()),
                "Error", wxOK | wxICON_ERROR);
            return;
        }
        saved_ = std::move(res.value());
    }

    EndModal(wxID_OK);
}

}  // namespace tracker::ui
