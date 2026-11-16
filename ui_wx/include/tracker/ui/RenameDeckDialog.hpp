#pragma once

// RenameDeckDialog: renames a deck and every variante that shares its name.

#include "tracker/services/DeckService.hpp"

#include <cstdint>
#include <string>

#include <wx/dialog.h>

class wxTextCtrl;

namespace tracker::ui {

class RenameDeckDialog : public wxDialog {
public:
    RenameDeckDialog(wxWindow* parent, DeckService& decks,
                     std::int64_t formatId, const std::string& currentName);

private:
    void onOk(wxCommandEvent&);

    DeckService& decks_;
    std::int64_t formatId_{0};
    std::string currentName_;
    wxTextCtrl* nameCtrl_{nullptr};
};

}  // namespace tracker::ui
