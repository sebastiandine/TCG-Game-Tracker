#include "tracker/ui/SettingsDialog.hpp"
#include "tracker/ui/Theme.hpp"
#include "tracker/domain/Enums.hpp"

#include <wx/button.h>
#include <wx/dirdlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <filesystem>

namespace tracker::ui {

SettingsDialog::SettingsDialog(wxWindow* parent, ConfigService& config)
    : wxDialog(parent, wxID_ANY, "Settings",
               wxDefaultPosition, wxSize(560, 180),
               wxDEFAULT_DIALOG_STYLE),
      config_(config) {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* dirRow = new wxBoxSizer(wxHORIZONTAL);
    dirRow->Add(new wxStaticText(this, wxID_ANY, "Data directory:"),
                0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    dataDirCtrl_ = new wxTextCtrl(this, wxID_ANY,
        wxString::FromUTF8(config_.current().dataStorage.c_str()));
    dirRow->Add(dataDirCtrl_, 1, wxEXPAND | wxRIGHT, 6);
    auto* browse = new wxButton(this, wxID_ANY, "Browse...");
    browse->Bind(wxEVT_BUTTON, &SettingsDialog::onBrowse, this);
    dirRow->Add(browse, 0);
    root->Add(dirRow, 0, wxEXPAND | wxALL, 10);

    auto* themeRow = new wxBoxSizer(wxHORIZONTAL);
    themeRow->Add(new wxStaticText(this, wxID_ANY, "Theme:"),
                  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    themeChoice_ = new wxChoice(this, wxID_ANY);
    themeChoice_->Append("Light");
    themeChoice_->Append("Dark");
    switch (config_.current().theme) {
        case Theme::Dark:  themeChoice_->SetSelection(1); break;
        case Theme::Light:
        default:           themeChoice_->SetSelection(0); break;
    }
    themeRow->Add(themeChoice_, 0);
    root->Add(themeRow, 0, wxEXPAND | wxALL, 10);

    auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
    if (btns) root->Add(btns, 0, wxALL | wxEXPAND, 10);
    Bind(wxEVT_BUTTON, &SettingsDialog::onOk, this, wxID_OK);

    SetSizer(root);
}

void SettingsDialog::onBrowse(wxCommandEvent&) {
    wxDirDialog dlg(this, "Choose data directory",
                    dataDirCtrl_->GetValue(),
                    wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    themeModalDialog(&dlg, config_.current().theme);
    if (dlg.ShowModal() == wxID_OK) {
        dataDirCtrl_->SetValue(dlg.GetPath());
    }
}

void SettingsDialog::onOk(wxCommandEvent&) {
    wxString dirValue = dataDirCtrl_->GetValue();
    dirValue.Trim(true).Trim(false);
    if (dirValue.empty()) {
        showThemedMessageDialog(this, "Data directory must not be empty.",
                                "Error", wxOK);
        return;
    }

    auto cfg = config_.current();
    cfg.dataStorage = std::filesystem::path(
        dirValue.ToStdString(wxConvUTF8)).generic_string();
    cfg.theme = (themeChoice_->GetSelection() == 1) ? Theme::Dark : Theme::Light;

    auto result = config_.store(cfg);
    if (!result) {
        showThemedMessageDialog(this,
            wxString::Format("Failed to save settings: %s",
                wxString::FromUTF8(result.error().c_str())),
            "Error", wxOK);
        return;
    }
    EndModal(wxID_OK);
}

}  // namespace tracker::ui
