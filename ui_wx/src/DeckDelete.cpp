#include "tracker/ui/DeckDelete.hpp"
#include "tracker/ui/AppContext.hpp"
#include "tracker/ui/Theme.hpp"

#include <cctype>
#include <string>

#include <wx/window.h>

namespace tracker::ui {

namespace {

bool sameText(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

int countRelatedGames(AppContext& ctx, const Deck& deck, std::string& error) {
    auto games = ctx.games.listByFormat(deck.formatId, deck.rotationId);
    if (!games) {
        error = games.error();
        return 0;
    }
    int count = 0;
    for (const auto& game : games.value()) {
        if (game.deckId == deck.id || game.opponentDeckId == deck.id) ++count;
    }
    return count;
}

wxString relatedGamesSentence(int count) {
    if (count == 1) return "1 related game will also be deleted.";
    return wxString::Format("%d related games will also be deleted.", count);
}

wxString attachedGamesSentence(int count) {
    if (count == 1) {
        return "1 game attached to this deck will also be deleted.";
    }
    return wxString::Format(
        "%d games attached to this deck will also be deleted.", count);
}

void showError(wxWindow* parent, const std::string& message) {
    showThemedMessageDialog(parent, wxString::FromUTF8(message.c_str()),
                            "Error", wxOK | wxICON_ERROR);
}

}  // namespace

bool confirmAndDeleteDeckRow(wxWindow* parent, AppContext& ctx,
                             const Deck& deck) {
    const bool isVariant = !deck.variant.empty();
    if (!isVariant) {
        auto listed = ctx.decks.listByFormat(deck.formatId, deck.rotationId);
        if (!listed) {
            showError(parent, listed.error());
            return false;
        }
        for (const auto& other : listed.value()) {
            if (sameText(other.name, deck.name) && !other.variant.empty()) {
                const wxString message = wxString::Format(
                    "Delete the variants of \"%s\" before deleting the deck.",
                    wxString::FromUTF8(deck.name.c_str()));
                showThemedMessageDialog(parent, message, "Delete deck",
                                        wxOK | wxICON_INFORMATION);
                return false;
            }
        }
    }

    std::string listError;
    const int gameCount = countRelatedGames(ctx, deck, listError);
    if (!listError.empty()) {
        showError(parent, listError);
        return false;
    }

    const wxString message = isVariant
        ? wxString::Format(
              "Delete the variante \"%s\" of %s? %s This cannot be undone.",
              wxString::FromUTF8(deck.variant.c_str()),
              wxString::FromUTF8(deck.name.c_str()),
              relatedGamesSentence(gameCount))
        : wxString::Format(
              "Delete the deck \"%s\"? %s This cannot be undone.",
              wxString::FromUTF8(deck.name.c_str()),
              attachedGamesSentence(gameCount));
    const wxString caption = isVariant ? "Delete variante" : "Delete deck";
    if (showThemedConfirmDialog(parent, message, caption) != wxID_YES) {
        return false;
    }

    const auto result = isVariant ? ctx.decks.deleteVariant(deck.id)
                                  : ctx.decks.deleteDeck(deck.id);
    if (!result) {
        showError(parent, result.error());
        return false;
    }
    return true;
}

}  // namespace tracker::ui
