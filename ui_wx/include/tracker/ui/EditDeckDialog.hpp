#pragma once

// EditDeckDialog: modal dialog for editing an existing deck record.

#include "tracker/domain/Deck.hpp"
#include "tracker/domain/DeckArchetype.hpp"

#include <cstdint>
#include <vector>

#include <wx/dialog.h>

class wxChoice;
class wxTextCtrl;

namespace tracker::ui {

struct AppContext;

class EditDeckDialog : public wxDialog {
public:
    EditDeckDialog(wxWindow* parent, AppContext& ctx, const Deck& deck,
                   const std::vector<DeckArchetype>& archetypes);

    // The updated deck (valid only after ShowModal() == wxID_OK).
    [[nodiscard]] const Deck& updatedDeck() const noexcept { return updated_; }

private:
    void onOk(wxCommandEvent&);

    AppContext& ctx_;
    Deck original_;
    Deck updated_;
    std::vector<DeckArchetype> archetypes_;

    wxTextCtrl* nameCtrl_{nullptr};
    wxChoice*   archetypeChoice_{nullptr};
    wxTextCtrl* variantCtrl_{nullptr};
    wxTextCtrl* variantNoteCtrl_{nullptr};
};

}  // namespace tracker::ui
