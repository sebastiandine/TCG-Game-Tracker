#pragma once

// SettingsDialog: edits the live Configuration via ConfigService.

#include "tracker/services/ConfigService.hpp"

#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>

namespace tracker::ui {

class SettingsDialog : public wxDialog {
public:
    SettingsDialog(wxWindow* parent, ConfigService& config);

private:
    void onBrowse(wxCommandEvent&);
    void onOk(wxCommandEvent&);

    ConfigService& config_;

    wxTextCtrl* dataDirCtrl_{nullptr};
    wxChoice*   themeChoice_{nullptr};
};

}  // namespace tracker::ui
