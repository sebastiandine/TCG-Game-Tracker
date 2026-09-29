#pragma once

// DecksPanel: inline form for creating decks and a grouped tree list of
// existing decks, scoped to the currently selected format. Names with
// variants are collapsible groups labeled "Name (N)" (N includes the
// main deck); names with a single record are leaves.

#include "tracker/domain/Deck.hpp"
#include "tracker/domain/DeckArchetype.hpp"

#include <cstdint>
#include <vector>

#include <wx/panel.h>

class wxButton;
class wxChoice;
class wxTextCtrl;
class wxTreeCtrl;

namespace tracker::ui {

struct AppContext;

class DecksPanel : public wxPanel {
public:
    DecksPanel(wxWindow* parent, AppContext& ctx);

    // Load decks for the given format and refresh the tree and archetype list.
    void loadFormat(std::int64_t formatId);

    // Re-apply the current theme palette.
    void applyTheme();

private:
    void buildForm(wxSizer* parent);
    void buildTree(wxSizer* parent);
    void onAdd();
    void onEdit(const Deck& deck);
    void refreshTree();
    void fillArchetypeChoice();

    AppContext& ctx_;
    std::int64_t formatId_{0};
    std::vector<DeckArchetype> archetypes_;

    wxTextCtrl* nameCtrl_{nullptr};
    wxChoice*   archetypeChoice_{nullptr};
    wxTextCtrl* variantCtrl_{nullptr};
    wxTextCtrl* variantNoteCtrl_{nullptr};
    wxButton*   addButton_{nullptr};
    wxTreeCtrl* tree_{nullptr};
};

}  // namespace tracker::ui
