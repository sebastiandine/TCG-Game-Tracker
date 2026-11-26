#include "tracker/ui/CreateRotationDialog.hpp"
#include "tracker/ui/RotationFields.hpp"
#include "tracker/ui/Theme.hpp"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace tracker::ui {

namespace {

void addSpanRow(wxWindow* parent, wxSizer* root, const wxString& label,
                wxChoice*& month, wxSpinCtrl*& year, int defaultYear,
                int defaultMonth) {
    auto* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(parent, wxID_ANY, label),
             0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    month = makeMonthChoice(parent, defaultMonth);
    year = makeYearSpin(parent, defaultYear);
    row->Add(month, 1, wxEXPAND | wxRIGHT, 6);
    row->Add(year, 0, wxEXPAND);
    root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
}

}  // namespace

CreateRotationDialog::CreateRotationDialog(wxWindow* parent, FormatService& formats,
                                           std::int64_t formatId, int startYear,
                                           int startMonth)
    : wxDialog(parent, wxID_ANY, "New Rotation",
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE),
      formats_(formats),
      formatId_(formatId) {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* nameRow = new wxBoxSizer(wxHORIZONTAL);
    nameRow->Add(new wxStaticText(this, wxID_ANY, "Rotation name:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    nameCtrl_ = new wxTextCtrl(this, wxID_ANY);
    nameRow->Add(nameCtrl_, 1, wxEXPAND);
    root->Add(nameRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    addSpanRow(this, root, "Start:", startMonth_, startYear_, startYear,
               startMonth);
    addSpanRow(this, root, "End:", endMonth_, endYear_, startYear, startMonth);

    auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
    if (btns) root->Add(btns, 0, wxALL | wxEXPAND, 10);
    Bind(wxEVT_BUTTON, &CreateRotationDialog::onOk, this, wxID_OK);

    SetSizerAndFit(root);
    nameCtrl_->SetFocus();
    CentreOnParent();
}

void CreateRotationDialog::onOk(wxCommandEvent&) {
    const std::string name = nameCtrl_->GetValue().ToStdString(wxConvUTF8);
    const int startMonth = startMonth_->GetSelection() + 1;
    const int endMonth = endMonth_->GetSelection() + 1;
    auto result = formats_.createRotation(
        formatId_, name, startYear_->GetValue(), startMonth,
        endYear_->GetValue(), endMonth);
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
