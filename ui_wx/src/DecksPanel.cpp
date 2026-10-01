#include "tracker/ui/DecksPanel.hpp"
#include "tracker/ui/AppContext.hpp"
#include "tracker/ui/EditDeckDialog.hpp"
#include "tracker/ui/Theme.hpp"
#include "tracker/domain/DeckGroup.hpp"

#include <string>
#include <vector>

#include <wx/arrstr.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/treectrl.h>

namespace tracker::ui {

namespace {

// Stores the full Deck record on a tree child item.
class DeckTreeItemData : public wxTreeItemData {
public:
    explicit DeckTreeItemData(const Deck& deck) : deck_(deck) {}
    [[nodiscard]] const Deck& deck() const noexcept { return deck_; }
private:
    Deck deck_;
};

wxString deckItemLabel(const Deck& deck) {
    if (deck.variant.empty()) {
        return wxString::FromUTF8(deck.name.c_str());
    }
    return wxString::Format(
        "%s / Variante: %s",
        wxString::FromUTF8(deck.name.c_str()),
        wxString::FromUTF8(deck.variant.c_str()));
}

// Parent row for a name that has variants: "Landstill (3)".
// Count includes the main (empty-variant) deck.
wxString groupHeaderLabel(const DeckGroup& group) {
    const auto count = std::to_string(group.variants.size());
    return wxString::FromUTF8(group.name.c_str()) + " (" +
           wxString::FromUTF8(count.c_str()) + ")";
}

void colourTreeItems(wxTreeCtrl* tree, const wxColour& fg) {
    if (tree == nullptr) return;
    const auto root = tree->GetRootItem();
    if (!root.IsOk()) return;

    wxTreeItemIdValue cookie;
    for (auto item = tree->GetFirstChild(root, cookie); item.IsOk();
         item = tree->GetNextChild(root, cookie)) {
        tree->SetItemTextColour(item, fg);
        wxTreeItemIdValue inner;
        for (auto child = tree->GetFirstChild(item, inner); child.IsOk();
             child = tree->GetNextChild(item, inner)) {
            tree->SetItemTextColour(child, fg);
        }
    }
}

}  // namespace

DecksPanel::DecksPanel(wxWindow* parent, AppContext& ctx)
    : wxPanel(parent),
      ctx_(ctx) {
    auto* root = new wxBoxSizer(wxVERTICAL);

    buildForm(root);
    root->AddSpacer(10);
    buildTree(root);

    SetSizer(root);
}

void DecksPanel::buildForm(wxSizer* parent) {
    Freeze();

    auto* formBox = new wxStaticBoxSizer(wxVERTICAL, this, "New Deck");

    // 2-column grid: labels right-aligned, controls stretch to fill.
    auto* grid = new wxFlexGridSizer(2, 6, 8);
    grid->AddGrowableCol(1, 1);

    // Row 1: Name
    grid->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    nameCtrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(nameCtrl_, 1, wxEXPAND);

    // Row 2: Archetype
    grid->Add(new wxStaticText(this, wxID_ANY, "Archetype:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    archetypeChoice_ = new wxChoice(this, wxID_ANY);
    grid->Add(archetypeChoice_, 1, wxEXPAND);

    // Row 3: Variante
    grid->Add(new wxStaticText(this, wxID_ANY, "Variante:"), 0,
              wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    variantCtrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(variantCtrl_, 1, wxEXPAND);

    // Row 4: Variante Note
    grid->Add(new wxStaticText(this, wxID_ANY, "Variante Note:"), 0,
              wxALIGN_RIGHT | wxTOP, 2);
    variantNoteCtrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                      wxSize(-1, 52), wxTE_MULTILINE);
    grid->Add(variantNoteCtrl_, 1, wxEXPAND);

    formBox->Add(grid, 0, wxEXPAND | wxALL, 8);

    // Add button — right-aligned for a cleaner look.
    addButton_ = new wxButton(this, wxID_ANY, "Add Deck");
    formBox->Add(addButton_, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    addButton_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onAdd(); });

    parent->Add(formBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    Thaw();
}

void DecksPanel::buildTree(wxSizer* parent) {
    // wxTR_LINES_AT_ROOT is required on MSW so first-level items get
    // expand/collapse buttons when the root is hidden. wxTR_NO_LINES would
    // also suppress those buttons (TVS_LINESATROOT needs TVS_HASLINES).
    tree_ = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT |
                               wxTR_LINES_AT_ROOT);
    // Reserve space for +/- buttons so they don't paint over the label
    // on the first native layout pass.
    tree_->SetIndent(18);
    parent->Add(tree_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Tooltip: show variant note on hover over child items.
    tree_->Bind(wxEVT_MOTION, [this](wxMouseEvent& evt) {
        int flags = 0;
        auto item = tree_->HitTest(evt.GetPosition(), flags);
        if (item.IsOk()) {
            auto* data =
                dynamic_cast<DeckTreeItemData*>(tree_->GetItemData(item));
            if (data && !data->deck().variantNote.empty()) {
                tree_->SetToolTip(
                    wxString::FromUTF8(data->deck().variantNote.c_str()));
                evt.Skip();
                return;
            }
        }
        tree_->UnsetToolTip();
        evt.Skip();
    });
    installActionableRowCursor(tree_, [this](const wxPoint& pos) {
        int flags = 0;
        const auto item = tree_->HitTest(pos, flags);
        if (!item.IsOk()) return false;
        return dynamic_cast<DeckTreeItemData*>(tree_->GetItemData(item)) != nullptr;
    });

    // Double-click: open edit dialog for child items (deck records).
    tree_->Bind(wxEVT_TREE_ITEM_ACTIVATED, [this](wxTreeEvent& evt) {
        auto item = evt.GetItem();
        if (!item.IsOk()) return;
        auto* data =
            dynamic_cast<DeckTreeItemData*>(tree_->GetItemData(item));
        if (data == nullptr) return;  // parent group row, not a deck

        onEdit(data->deck());
    });
}

void DecksPanel::onAdd() {
    if (formatId_ <= 0) return;

    const std::string name =
        nameCtrl_->GetValue().ToStdString(wxConvUTF8);
    const std::string variant =
        variantCtrl_->GetValue().ToStdString(wxConvUTF8);
    const std::string note =
        variantNoteCtrl_->GetValue().ToStdString(wxConvUTF8);

    if (archetypes_.empty()) {
        showThemedMessageDialog(
            this, "Please add at least one archetype first.",
            "No Archetypes", wxOK | wxICON_INFORMATION);
        return;
    }

    const int archSel = archetypeChoice_->GetSelection();
    if (archSel == wxNOT_FOUND || archSel < 0 ||
        static_cast<std::size_t>(archSel) >= archetypes_.size()) {
        showThemedMessageDialog(this, "Please select an archetype.",
                                "Validation Error", wxOK | wxICON_WARNING);
        return;
    }
    const std::int64_t archetypeId = archetypes_[archSel].id;

    auto result = ctx_.decks.create(formatId_, archetypeId, name, variant, note);
    if (!result) {
        showThemedMessageDialog(
            this, wxString::FromUTF8(result.error().c_str()),
            "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Clear form on success.
    nameCtrl_->Clear();
    variantCtrl_->Clear();
    variantNoteCtrl_->Clear();

    refreshTree();
}

void DecksPanel::onEdit(const Deck& deck) {
    EditDeckDialog dlg(this, ctx_, deck, archetypes_);
    themeModalDialog(&dlg, ctx_.config.current().theme);
    if (dlg.ShowModal() == wxID_OK) {
        refreshTree();
    }
}

void DecksPanel::fillArchetypeChoice() {
    if (archetypeChoice_ == nullptr) return;
    const int previous = archetypeChoice_->GetSelection();
    archetypeChoice_->Clear();
    archetypes_.clear();

    const auto gameId = ctx_.config.current().selectedGameId;
    auto archResult = ctx_.decks.listByGame(gameId);
    if (!archResult) return;
    archetypes_ = std::move(archResult.value());
    wxArrayString items;
    for (const auto& a : archetypes_) {
        items.Add(wxString::FromUTF8(a.name.c_str()));
    }
    archetypeChoice_->Append(items);
    if (!archetypes_.empty()) {
        if (previous != wxNOT_FOUND &&
            static_cast<std::size_t>(previous) < archetypes_.size()) {
            archetypeChoice_->SetSelection(previous);
        } else {
            archetypeChoice_->SetSelection(0);
        }
    }
}

void DecksPanel::loadFormat(std::int64_t formatId) {
    formatId_ = formatId;
    fillArchetypeChoice();
    refreshTree();
}

void DecksPanel::refreshTree() {
    if (tree_ == nullptr) return;

    tree_->Freeze();
    tree_->DeleteAllItems();

    auto root = tree_->AddRoot("root");

    if (formatId_ <= 0) {
        tree_->Thaw();
        return;
    }

    auto result = ctx_.decks.listByFormat(formatId_);
    if (!result) {
        tree_->Thaw();
        return;
    }

    const auto groups = groupDecksByName(result.value());
    std::vector<wxTreeItemId> toExpand;

    for (const auto& group : groups) {
        const bool hasSubdecks = group.variants.size() > 1;
        const wxTreeItemId parentItem =
            hasSubdecks ? tree_->AppendItem(root, groupHeaderLabel(group))
                        : root;

        for (const auto& deck : group.variants) {
            auto* itemData = new DeckTreeItemData(deck);
            tree_->AppendItem(parentItem, deckItemLabel(deck), -1, -1,
                              itemData);
        }

        if (hasSubdecks) {
            toExpand.push_back(parentItem);
        }
    }

    tree_->Thaw();

    // Expand after Thaw: doing it while frozen (or while hidden) leaves the
    // group row with a stale text rect until the user clicks.
    for (const auto& item : toExpand) {
        if (item.IsOk()) {
            tree_->Expand(item);
        }
    }

    const ThemePalette palette = paletteForTheme(ctx_.config.current().theme);
    colourTreeItems(tree_, palette.inputText);
    tree_->UnselectAll();
    tree_->Refresh();

    CallAfter([this] {
        if (tree_ == nullptr) return;
        const unsigned indent = tree_->GetIndent();
        tree_->SetIndent(indent == 0 ? 18u : indent);
        tree_->UnselectAll();
        tree_->Refresh();
    });
}

void DecksPanel::applyTheme() {
    const Theme theme = ctx_.config.current().theme;
    const ThemePalette palette = paletteForTheme(theme);
    applyThemeToWindowTree(this, palette, theme);

    // Keep tree item text on the input palette after native class theming.
    if (tree_) {
        tree_->SetBackgroundColour(palette.inputBg);
        tree_->SetForegroundColour(palette.inputText);
        tree_->SetOwnBackgroundColour(palette.inputBg);
        tree_->SetOwnForegroundColour(palette.inputText);
        colourTreeItems(tree_, palette.inputText);
        tree_->Refresh();
    }

    Refresh();
}

}  // namespace tracker::ui
