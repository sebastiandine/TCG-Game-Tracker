#pragma once

// GameDialog: modal dialog for creating or editing a game record.

#include "tracker/domain/Deck.hpp"
#include "tracker/domain/Game.hpp"
#include "tracker/domain/GameType.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <wx/dialog.h>

class wxChoice;
class wxTextCtrl;

namespace tracker::ui {

struct AppContext;
class ThemedDatePickerCtrl;

class GameDialog : public wxDialog {
public:
    // Create mode: pass std::nullopt for game.
    // Edit mode: pass the existing Game to edit.
    GameDialog(wxWindow* parent, AppContext& ctx,
               std::int64_t formatId,
               const std::vector<Deck>& decks,
               const std::vector<GameType>& gameTypes,
               const std::optional<Game>& game,
               std::int64_t lastDeckId,
               std::int64_t lastGameTypeId);

    // The saved game (valid only after ShowModal() == wxID_OK).
    [[nodiscard]] const Game& savedGame() const noexcept { return saved_; }

private:
    void rebuildVariantChoice();
    void onOk(wxCommandEvent&);

    AppContext& ctx_;
    std::int64_t formatId_;
    std::vector<Deck> decks_;
    std::vector<GameType> gameTypes_;
    std::optional<Game> existing_;
    Game saved_;

    // Unique deck names for the Deck and Opponent choices.
    std::vector<std::string> deckNames_;
    // Variants grouped per deck name index.
    std::vector<std::vector<const Deck*>> variantsByName_;

    ThemedDatePickerCtrl* dateCtrl_{nullptr};
    wxChoice*         deckChoice_{nullptr};
    wxChoice*         variantChoice_{nullptr};
    wxTextCtrl*       opponentCtrl_{nullptr};
    wxChoice*         opponentChoice_{nullptr};
    wxChoice*         resultScoreChoice_{nullptr};
    wxChoice*         gameTypeChoice_{nullptr};
    wxTextCtrl*       notesCtrl_{nullptr};
};

}  // namespace tracker::ui
