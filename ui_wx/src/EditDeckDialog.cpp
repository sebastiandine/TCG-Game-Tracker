#include "tracker/ui/EditDeckDialog.hpp"
#include "tracker/ui/AppContext.hpp"
#include "tracker/ui/Theme.hpp"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace tracker::ui {

EditDeckDialog::EditDeckDialog(wxWindow* parent, AppContext& ctx,
                               const Deck& deck,
                               const std::vector<DeckArchetype>& archetypes)
    : wxDialog(parent, wxID_ANY, "Edit Deck",
               wxDefaultPosition, wxSize(480, 280),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      ctx_(ctx),
      original_(deck),
      archetypes_(archetypes) {

    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* grid = new wxFlexGridSizer(2, 8, 10);
    grid->AddGrowableCol(1, 1);

    // Name
    grid->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    nameCtrl_ = new wxTextCtrl(this, wxID_ANY,
                               wxString::FromUTF8(deck.name.c_str()));
    grid->Add(nameCtrl_, 1, wxEXPAND);

    // Archetype
    grid->Add(new wxStaticText(this, wxID_ANY, "Archetype:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    archetypeChoice_ = new wxChoice(this, wxID_ANY);
    {
        wxArrayString items;
        int selectedIdx = 0;
        for (std::size_t i = 0; i < archetypes_.size(); ++i) {
            items.Add(wxString::FromUTF8(archetypes_[i].name.c_str()));
            if (archetypes_[i].id == deck.archetypeId) {
                selectedIdx = static_cast<int>(i);
            }
        }
        archetypeChoice_->Append(items);
        archetypeChoice_->SetSelection(selectedIdx);
    }
    grid->Add(archetypeChoice_, 1, wxEXPAND);

    // Variante
    grid->Add(new wxStaticText(this, wxID_ANY, "Variante:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    variantCtrl_ = new wxTextCtrl(this, wxID_ANY,
                                  wxString::FromUTF8(deck.variant.c_str()));
    grid->Add(variantCtrl_, 1, wxEXPAND);

    // Variante Note
    grid->Add(new wxStaticText(this, wxID_ANY, "Variante Note:"), 0,
              wxALIGN_RIGHT | wxTOP, 2);
    variantNoteCtrl_ = new wxTextCtrl(
        this, wxID_ANY,
        wxString::FromUTF8(deck.variantNote.c_str()),
        wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE);
    grid->Add(variantNoteCtrl_, 1, wxEXPAND);

    root->Add(grid, 1, wxEXPAND | wxALL, 12);

    auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
    if (btns) root->Add(btns, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    Bind(wxEVT_BUTTON, &EditDeckDialog::onOk, this, wxID_OK);

    SetSizer(root);
    nameCtrl_->SetFocus();
    CentreOnParent();
}

void EditDeckDialog::onOk(wxCommandEvent&) {
    const std::string name =
        nameCtrl_->GetValue().ToStdString(wxConvUTF8);
    const std::string variant =
        variantCtrl_->GetValue().ToStdString(wxConvUTF8);
    const std::string note =
        variantNoteCtrl_->GetValue().ToStdString(wxConvUTF8);

    const int archSel = archetypeChoice_->GetSelection();
    if (archSel == wxNOT_FOUND || archSel < 0 ||
        static_cast<std::size_t>(archSel) >= archetypes_.size()) {
        showThemedMessageDialog(this, "Please select an archetype.",
                                "Validation Error", wxOK | wxICON_WARNING);
        return;
    }
    const std::int64_t archetypeId = archetypes_[archSel].id;

    auto result = ctx_.decks.update(original_.id, archetypeId, name, variant,
                                    note);
    if (!result) {
        showThemedMessageDialog(
            this, wxString::FromUTF8(result.error().c_str()),
            "Error", wxOK | wxICON_ERROR);
        return;
    }

    updated_ = std::move(result.value());
    EndModal(wxID_OK);
}

}  // namespace tracker::ui
