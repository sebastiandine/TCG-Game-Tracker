#pragma once

// CreateGameTitleDialog: simple modal that collects a game name.

#include "tracker/services/GameTitleService.hpp"

#include <wx/dialog.h>
#include <wx/textctrl.h>

namespace tracker::ui {

class CreateGameTitleDialog : public wxDialog {
public:
    CreateGameTitleDialog(wxWindow* parent, GameTitleService& gameTitles);

    [[nodiscard]] const GameTitle& createdGame() const noexcept { return created_; }

private:
    void onOk(wxCommandEvent&);

    GameTitleService& gameTitles_;
    wxTextCtrl* nameCtrl_{nullptr};
    GameTitle created_;
};

}  // namespace tracker::ui
