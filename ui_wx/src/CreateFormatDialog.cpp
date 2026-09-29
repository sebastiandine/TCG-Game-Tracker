#include "tracker/ui/CreateFormatDialog.hpp"
#include "tracker/ui/Theme.hpp"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace tracker::ui {

CreateFormatDialog::CreateFormatDialog(wxWindow* parent, FormatService& formats,
                                       std::int64_t gameId)
    : wxDialog(parent, wxID_ANY, "Create Format",
               wxDefaultPosition, wxSize(400, 140),
               wxDEFAULT_DIALOG_STYLE),
      formats_(formats),
      gameId_(gameId) {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(this, wxID_ANY, "Format name:"),
             0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    nameCtrl_ = new wxTextCtrl(this, wxID_ANY);
    row->Add(nameCtrl_, 1, wxEXPAND);
    root->Add(row, 0, wxEXPAND | wxALL, 10);

    auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
    if (btns) root->Add(btns, 0, wxALL | wxEXPAND, 10);
    Bind(wxEVT_BUTTON, &CreateFormatDialog::onOk, this, wxID_OK);

    SetSizer(root);
    nameCtrl_->SetFocus();
}

void CreateFormatDialog::onOk(wxCommandEvent&) {
    const std::string name = nameCtrl_->GetValue().ToStdString(wxConvUTF8);
    auto result = formats_.create(gameId_, name);
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
