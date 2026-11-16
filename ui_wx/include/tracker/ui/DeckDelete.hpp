#pragma once

// Shared confirm-and-delete flow for a deck row, used by the Decks tree
// and the edit dialog.

#include "tracker/domain/Deck.hpp"

class wxWindow;

namespace tracker::ui {

struct AppContext;

// Asks for confirmation, then deletes the row.
// A variante delete also removes games that reference that row.
// A main-deck delete is refused while any variante of that name remains;
// otherwise games attached to the main deck are removed with it.
// Returns true when the row was deleted.
bool confirmAndDeleteDeckRow(wxWindow* parent, AppContext& ctx, const Deck& deck);

}  // namespace tracker::ui
