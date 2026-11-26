#include "tracker/ui/CreateRotationDialog.hpp"
#include "tracker/ui/RotationFields.hpp"
#include "tracker/ui/Theme.hpp"

#include <optional>

#include <wx/button.h>
#include <wx/checkbox.h>
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
    build("", startYear, startMonth, startYear, startMonth, false);
}

CreateRotationDialog::CreateRotationDialog(wxWindow* parent, FormatService& formats,
                                           const Rotation& existing)
    : wxDialog(parent, wxID_ANY, "Edit Rotation",
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE),
      formats_(formats),
      formatId_(existing.formatId),
      rotationId_(existing.id),
      editing_(true) {
    const bool unknown = !existing.endYear || !existing.endMonth;
    build(wxString::FromUTF8(existing.name.c_str()),
          existing.startYear, existing.startMonth,
          existing.endYear.value_or(existing.startYear),
          existing.endMonth.value_or(existing.startMonth),
          unknown);
}

void CreateRotationDialog::build(const wxString& name, int startYear,
                                 int startMonth, int endYear, int endMonth,
                                 bool endUnknown) {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* nameRow = new wxBoxSizer(wxHORIZONTAL);
    nameRow->Add(new wxStaticText(this, wxID_ANY, "Rotation name:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    nameCtrl_ = new wxTextCtrl(this, wxID_ANY, name);
    nameRow->Add(nameCtrl_, 1, wxEXPAND);
    root->Add(nameRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    addSpanRow(this, root, "Start:", startMonth_, startYear_, startYear,
               startMonth);
    addSpanRow(this, root, "End:", endMonth_, endYear_, endYear, endMonth);

    endUnknown_ = new wxCheckBox(this, wxID_ANY, "End unknown");
    endUnknown_->SetValue(endUnknown);
    endMonth_->Enable(!endUnknown);
    endYear_->Enable(!endUnknown);
    root->Add(endUnknown_, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    endUnknown_->Bind(wxEVT_CHECKBOX, &CreateRotationDialog::onToggleEndUnknown,
                      this);

    auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
    if (btns) root->Add(btns, 0, wxALL | wxEXPAND, 10);
    Bind(wxEVT_BUTTON, &CreateRotationDialog::onOk, this, wxID_OK);

    SetSizerAndFit(root);
    nameCtrl_->SetFocus();
    CentreOnParent();
}

void CreateRotationDialog::onToggleEndUnknown(wxCommandEvent&) {
    const bool known = !endUnknown_->GetValue();
    endMonth_->Enable(known);
    endYear_->Enable(known);
}

void CreateRotationDialog::onOk(wxCommandEvent&) {
    const std::string name = nameCtrl_->GetValue().ToStdString(wxConvUTF8);
    const int startMonth = startMonth_->GetSelection() + 1;
    std::optional<int> endYear;
    std::optional<int> endMonth;
    if (!endUnknown_->GetValue()) {
        endYear = endYear_->GetValue();
        endMonth = endMonth_->GetSelection() + 1;
    }
    auto result = editing_
        ? formats_.updateRotation(
              formatId_, rotationId_, name, startYear_->GetValue(), startMonth,
              endYear, endMonth)
        : formats_.createRotation(
              formatId_, name, startYear_->GetValue(), startMonth,
              endYear, endMonth);
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
