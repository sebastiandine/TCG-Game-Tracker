#pragma once

// CreateGameTypeDialog: modal that collects a game type name,
// competitiveness, and play medium.

#include "tracker/services/GameTypeService.hpp"

#include <cstdint>

#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>

namespace tracker::ui {

class CreateGameTypeDialog : public wxDialog {
public:
    CreateGameTypeDialog(wxWindow* parent, GameTypeService& gameTypes,
                         std::int64_t gameId);

    // The created game type (valid only after ShowModal() == wxID_OK).
    [[nodiscard]] const GameType& createdGameType() const noexcept { return created_; }

private:
    void onOk(wxCommandEvent&);

    GameTypeService& gameTypes_;
    std::int64_t gameId_{0};
    wxTextCtrl* nameCtrl_{nullptr};
    wxChoice*   compChoice_{nullptr};
    wxChoice*   mediumChoice_{nullptr};
    GameType    created_;
};

}  // namespace tracker::ui
