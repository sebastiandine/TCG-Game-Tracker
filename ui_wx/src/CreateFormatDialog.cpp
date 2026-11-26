#include "tracker/ui/CreateFormatDialog.hpp"
#include "tracker/ui/RotationFields.hpp"
#include "tracker/ui/Theme.hpp"

#include <optional>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace tracker::ui {

CreateFormatDialog::CreateFormatDialog(wxWindow* parent, FormatService& formats,
                                       std::int64_t gameId)
    : wxDialog(parent, wxID_ANY, "Create Format",
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE),
      formats_(formats),
      gameId_(gameId) {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(this, wxID_ANY, "Format name:"),
             0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    nameCtrl_ = new wxTextCtrl(this, wxID_ANY);
    row->Add(nameCtrl_, 1, wxEXPAND);
    root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    rotatingCheck_ = new wxCheckBox(this, wxID_ANY, "Rotating format");
    root->Add(rotatingCheck_, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    rotatingCheck_->Bind(wxEVT_CHECKBOX, &CreateFormatDialog::onToggleRotating,
                         this);

    int year = 2026;
    int month = 1;
    currentYearMonth(year, month);

    rotationPanel_ = new wxPanel(this, wxID_ANY);
    auto* rotation = new wxBoxSizer(wxVERTICAL);

    auto* nameRow = new wxBoxSizer(wxHORIZONTAL);
    nameRow->Add(new wxStaticText(rotationPanel_, wxID_ANY, "Rotation name:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    rotationName_ = new wxTextCtrl(rotationPanel_, wxID_ANY);
    nameRow->Add(rotationName_, 1, wxEXPAND);
    rotation->Add(nameRow, 0, wxEXPAND | wxTOP, 8);

    auto* startRow = new wxBoxSizer(wxHORIZONTAL);
    startRow->Add(new wxStaticText(rotationPanel_, wxID_ANY, "Start:"),
                  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    startMonth_ = makeMonthChoice(rotationPanel_, month);
    startYear_ = makeYearSpin(rotationPanel_, year);
    startRow->Add(startMonth_, 1, wxEXPAND | wxRIGHT, 6);
    startRow->Add(startYear_, 0, wxEXPAND);
    rotation->Add(startRow, 0, wxEXPAND | wxTOP, 8);

    auto* endRow = new wxBoxSizer(wxHORIZONTAL);
    endRow->Add(new wxStaticText(rotationPanel_, wxID_ANY, "End:"),
                0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    endMonth_ = makeMonthChoice(rotationPanel_, month);
    endYear_ = makeYearSpin(rotationPanel_, year);
    endRow->Add(endMonth_, 1, wxEXPAND | wxRIGHT, 6);
    endRow->Add(endYear_, 0, wxEXPAND);
    rotation->Add(endRow, 0, wxEXPAND | wxTOP, 8);

    endUnknown_ = new wxCheckBox(rotationPanel_, wxID_ANY, "End unknown");
    rotation->Add(endUnknown_, 0, wxTOP, 8);
    endUnknown_->Bind(wxEVT_CHECKBOX, &CreateFormatDialog::onToggleEndUnknown,
                      this);

    rotationPanel_->SetSizer(rotation);
    root->Add(rotationPanel_, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    root->Show(rotationPanel_, false, true);

    auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
    if (btns) root->Add(btns, 0, wxALL | wxEXPAND, 10);
    Bind(wxEVT_BUTTON, &CreateFormatDialog::onOk, this, wxID_OK);

    SetSizerAndFit(root);
    nameCtrl_->SetFocus();
    CentreOnParent();
}

void CreateFormatDialog::onToggleRotating(wxCommandEvent&) {
    const bool show = rotatingCheck_->GetValue();
    GetSizer()->Show(rotationPanel_, show, true);
    Layout();
    Fit();
}

void CreateFormatDialog::onToggleEndUnknown(wxCommandEvent&) {
    const bool known = !endUnknown_->GetValue();
    endMonth_->Enable(known);
    endYear_->Enable(known);
}

void CreateFormatDialog::onOk(wxCommandEvent&) {
    const std::string name = nameCtrl_->GetValue().ToStdString(wxConvUTF8);
    if (rotatingCheck_->GetValue()) {
        const std::string rotationName =
            rotationName_->GetValue().ToStdString(wxConvUTF8);
        std::optional<int> endYear;
        std::optional<int> endMonth;
        if (!endUnknown_->GetValue()) {
            endYear = endYear_->GetValue();
            endMonth = endMonth_->GetSelection() + 1;
        }
        auto result = formats_.createRotating(
            gameId_, name, rotationName,
            startYear_->GetValue(), startMonth_->GetSelection() + 1,
            endYear, endMonth);
        if (!result) {
            showThemedMessageDialog(this,
                wxString::FromUTF8(result.error().c_str()),
                "Error", wxOK);
            return;
        }
        created_ = std::move(result.value().format);
        createdRotation_ = std::move(result.value().rotation);
    } else {
        auto result = formats_.create(gameId_, name);
        if (!result) {
            showThemedMessageDialog(this,
                wxString::FromUTF8(result.error().c_str()),
                "Error", wxOK);
            return;
        }
        created_ = std::move(result.value());
        createdRotation_ = {};
    }
    EndModal(wxID_OK);
}

}  // namespace tracker::ui
