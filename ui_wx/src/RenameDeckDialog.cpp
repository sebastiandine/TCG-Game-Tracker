#include "tracker/ui/RenameDeckDialog.hpp"
#include "tracker/ui/Theme.hpp"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace tracker::ui {

RenameDeckDialog::RenameDeckDialog(wxWindow* parent, DeckService& decks,
                                   std::int64_t formatId,
                                   const std::string& currentName)
    : wxDialog(parent, wxID_ANY, "Rename Deck",
               wxDefaultPosition, wxSize(420, 150),
               wxDEFAULT_DIALOG_STYLE),
      decks_(decks),
      formatId_(formatId),
      currentName_(currentName) {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(this, wxID_ANY, "Deck name:"),
             0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    nameCtrl_ = new wxTextCtrl(this, wxID_ANY,
                               wxString::FromUTF8(currentName.c_str()));
    row->Add(nameCtrl_, 1, wxEXPAND);
    root->Add(row, 0, wxEXPAND | wxALL, 10);

    auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
    if (btns) root->Add(btns, 0, wxALL | wxEXPAND, 10);
    Bind(wxEVT_BUTTON, &RenameDeckDialog::onOk, this, wxID_OK);

    SetSizer(root);
    nameCtrl_->SetFocus();
    nameCtrl_->SelectAll();
    CentreOnParent();
}

void RenameDeckDialog::onOk(wxCommandEvent&) {
    const std::string name = nameCtrl_->GetValue().ToStdString(wxConvUTF8);
    auto result = decks_.renameFamily(formatId_, currentName_, name);
    if (!result) {
        showThemedMessageDialog(
            this, wxString::FromUTF8(result.error().c_str()),
            "Error", wxOK | wxICON_ERROR);
        return;
    }
    EndModal(wxID_OK);
}

}  // namespace tracker::ui
