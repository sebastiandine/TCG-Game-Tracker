#pragma once

// CreateArchetypeDialog: simple modal that collects an archetype name.

#include "tracker/services/DeckService.hpp"

#include <cstdint>

#include <wx/dialog.h>
#include <wx/textctrl.h>

namespace tracker::ui {

class CreateArchetypeDialog : public wxDialog {
public:
    CreateArchetypeDialog(wxWindow* parent, DeckService& decks,
                          std::int64_t gameId);

    [[nodiscard]] const DeckArchetype& createdArchetype() const noexcept {
        return created_;
    }

private:
    void onOk(wxCommandEvent&);

    DeckService& decks_;
    std::int64_t gameId_{0};
    wxTextCtrl* nameCtrl_{nullptr};
    DeckArchetype created_;
};

}  // namespace tracker::ui
