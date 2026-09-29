#include "tracker/ui/CreateGameTypeDialog.hpp"
#include "tracker/ui/Theme.hpp"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace tracker::ui {

CreateGameTypeDialog::CreateGameTypeDialog(wxWindow* parent,
                                           GameTypeService& gameTypes,
                                           std::int64_t gameId)
    : wxDialog(parent, wxID_ANY, "Create Game Type",
               wxDefaultPosition, wxSize(400, 220),
               wxDEFAULT_DIALOG_STYLE),
      gameTypes_(gameTypes),
      gameId_(gameId) {
    auto* root = new wxBoxSizer(wxVERTICAL);

    // Type name row.
    auto* nameRow = new wxBoxSizer(wxHORIZONTAL);
    nameRow->Add(new wxStaticText(this, wxID_ANY, "Type name:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    nameCtrl_ = new wxTextCtrl(this, wxID_ANY);
    nameRow->Add(nameCtrl_, 1, wxEXPAND);
    root->Add(nameRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    // Competitiveness row.
    auto* compRow = new wxBoxSizer(wxHORIZONTAL);
    compRow->Add(new wxStaticText(this, wxID_ANY, "Competitiveness:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    compChoice_ = new wxChoice(this, wxID_ANY);
    compChoice_->Append("Competitive");
    compChoice_->Append("Non-Competitive");
    compChoice_->SetSelection(0);
    compRow->Add(compChoice_, 0);
    root->Add(compRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    // Play medium row.
    auto* medRow = new wxBoxSizer(wxHORIZONTAL);
    medRow->Add(new wxStaticText(this, wxID_ANY, "Medium:"),
                0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    mediumChoice_ = new wxChoice(this, wxID_ANY);
    mediumChoice_->Append("Paper");
    mediumChoice_->Append("Online");
    mediumChoice_->SetSelection(0);
    medRow->Add(mediumChoice_, 0);
    root->Add(medRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
    if (btns) root->Add(btns, 0, wxALL | wxEXPAND, 10);
    Bind(wxEVT_BUTTON, &CreateGameTypeDialog::onOk, this, wxID_OK);

    SetSizer(root);
    nameCtrl_->SetFocus();
}

void CreateGameTypeDialog::onOk(wxCommandEvent&) {
    const std::string name = nameCtrl_->GetValue().ToStdString(wxConvUTF8);

    const Competitiveness comp = (compChoice_->GetSelection() == 0)
        ? Competitiveness::Competitive
        : Competitiveness::NonCompetitive;

    const PlayMedium medium = (mediumChoice_->GetSelection() == 0)
        ? PlayMedium::Paper
        : PlayMedium::Online;

    auto result = gameTypes_.create(gameId_, name, comp, medium);
    if (!result) {
        showThemedMessageDialog(this,
            wxString::FromUTF8(result.error().c_str()),
            "Error", wxOK);
        return;
    }
    created_ = std::move(result.value());
    EndModal(wxID_OK);
}

}  // namespace tracker::ui
