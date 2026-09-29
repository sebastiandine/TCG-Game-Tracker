#include "tracker/ui/MainFrame.hpp"
#include "tracker/ui/AppVersion.hpp"
#include "tracker/ui/CreateArchetypeDialog.hpp"
#include "tracker/ui/CreateFormatDialog.hpp"
#include "tracker/ui/CreateGameTitleDialog.hpp"
#include "tracker/ui/CreateGameTypeDialog.hpp"
#include "tracker/ui/FormatWorkspace.hpp"
#include "tracker/ui/ImportGamesDialog.hpp"
#include "tracker/ui/SettingsDialog.hpp"
#include "tracker/ui/Theme.hpp"

#include <wx/cursor.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace tracker::ui {

MainFrame::MainFrame(AppContext& ctx)
    : wxFrame(nullptr, wxID_ANY, kAppName,
              wxDefaultPosition, wxSize(900, 600)),
      ctx_(ctx) {
    buildMenuBar();
    buildLayout();
    applyTheme();

    CallAfter([this] {
        selectedGameId_ = ctx_.config.current().selectedGameId;
        selectedFormatId_ = ctx_.config.current().selectedFormatId;
        rebuildGamesMenu();
        rebuildFormatsMenu();
        if (selectedGameId_ != 0) {
            selectGame(selectedGameId_);
        } else {
            updateTitleAndEmptyState();
        }
    });
}

void MainFrame::buildMenuBar() {
    Bind(wxEVT_MENU, &MainFrame::onSettings, this, IdSettings);
    Bind(wxEVT_MENU, &MainFrame::onQuit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainFrame::onAbout, this, IdAbout);
    Bind(wxEVT_MENU, &MainFrame::onCreateGame, this, IdCreateGame);
    Bind(wxEVT_MENU, &MainFrame::onCreateFormat, this, IdCreateFormat);
    Bind(wxEVT_MENU, &MainFrame::onCreateGameType, this, IdCreateGameType);
    Bind(wxEVT_MENU, &MainFrame::onCreateArchetype, this, IdCreateArchetype);
    Bind(wxEVT_MENU, &MainFrame::onImportGames, this, IdImportGames);
    Bind(wxEVT_MENU, &MainFrame::onFormatSelected, this,
         IdFormatBase, IdFormatMax);
    Bind(wxEVT_MENU, &MainFrame::onGameSelected, this,
         IdGameBase, IdGameMax);
}

wxStaticText* MainFrame::addMenuLabel(wxSizer* sizer, const wxString& text, int border,
                                      void (MainFrame::*handler)()) {
    auto* label = new wxStaticText(menuStrip_, wxID_ANY, text);
    label->SetCursor(wxCursor(wxCURSOR_HAND));
    label->Bind(wxEVT_LEFT_DOWN, [this, handler](wxMouseEvent&) { (this->*handler)(); });
    sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM | wxRIGHT, border);
    return label;
}

void MainFrame::popupMenuUnder(wxWindow* label, wxMenu& menu) {
    if (menuStrip_ == nullptr || label == nullptr) return;
    const wxPoint pos = label->GetPosition();
    menuStrip_->PopupMenu(&menu, pos.x, menuStrip_->GetSize().GetHeight());
}

void MainFrame::buildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    menuStrip_ = new wxPanel(this, wxID_ANY);
    auto* menuSizer = new wxBoxSizer(wxHORIZONTAL);
    fileMenuLabel_ = addMenuLabel(menuSizer, "File", 4, &MainFrame::onOpenFileMenu);
    gamesMenuLabel_ = addMenuLabel(menuSizer, "Games", 8, &MainFrame::onOpenGamesMenu);
    formatsMenuLabel_ = addMenuLabel(menuSizer, "Formats", 8, &MainFrame::onOpenFormatsMenu);
    dataMenuLabel_ = addMenuLabel(menuSizer, "Data", 8, &MainFrame::onOpenDataMenu);
    helpMenuLabel_ = addMenuLabel(menuSizer, "Help", 8, &MainFrame::onOpenHelpMenu);
    menuStrip_->SetSizer(menuSizer);
    root->Add(menuStrip_, 0, wxEXPAND);

    contentPanel_ = new wxPanel(this);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    emptyPanel_ = new wxPanel(contentPanel_);
    auto* emptySizer = new wxBoxSizer(wxVERTICAL);
    centerLabel_ = new wxStaticText(emptyPanel_, wxID_ANY,
        kAppName,
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    wxFont font = centerLabel_->GetFont();
    font.MakeBold().MakeLarger().MakeLarger();
    centerLabel_->SetFont(font);
    emptySizer->AddStretchSpacer(1);
    emptySizer->Add(centerLabel_, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 20);
    emptySizer->AddStretchSpacer(1);
    emptyPanel_->SetSizer(emptySizer);

    workspace_ = new FormatWorkspace(contentPanel_, ctx_);
    workspace_->Hide();

    sizer->Add(emptyPanel_, 1, wxEXPAND);
    sizer->Add(workspace_, 1, wxEXPAND);

    contentPanel_->SetSizer(sizer);
    root->Add(contentPanel_, 1, wxEXPAND);

    SetSizer(root);
}

void MainFrame::onOpenFileMenu() {
    wxMenu menu;
    menu.Append(IdSettings, "Settings...");
    menu.AppendSeparator();
    menu.Append(wxID_EXIT, "Exit");
    popupMenuUnder(fileMenuLabel_, menu);
}

void MainFrame::onOpenGamesMenu() {
    const bool loaded = rebuildGamesMenu();

    wxMenu menu;
    if (!loaded) {
        menu.Append(IdGameBase, "(Error loading games)");
        menu.Enable(IdGameBase, false);
    } else if (loadedGames_.empty()) {
        menu.Append(IdGameBase, "(No games yet)");
        menu.Enable(IdGameBase, false);
    } else {
        for (std::size_t i = 0; i < loadedGames_.size(); ++i) {
            const int menuId = IdGameBase + static_cast<int>(i);
            if (menuId > IdGameMax) break;
            menu.AppendRadioItem(menuId,
                wxString::FromUTF8(loadedGames_[i].name.c_str()));
            if (loadedGames_[i].id == selectedGameId_) {
                menu.Check(menuId, true);
            }
        }
    }
    menu.AppendSeparator();
    menu.Append(IdCreateGame, "Create Game...");
    popupMenuUnder(gamesMenuLabel_, menu);
}

void MainFrame::onOpenFormatsMenu() {
    const bool loaded = rebuildFormatsMenu();

    wxMenu menu;
    if (selectedGameId_ == 0) {
        menu.Append(IdFormatBase, "(Select a game first)");
        menu.Enable(IdFormatBase, false);
    } else if (!loaded) {
        menu.Append(IdFormatBase, "(Error loading formats)");
        menu.Enable(IdFormatBase, false);
    } else if (loadedFormats_.empty()) {
        menu.Append(IdFormatBase, "(No formats yet)");
        menu.Enable(IdFormatBase, false);
    } else {
        for (std::size_t i = 0; i < loadedFormats_.size(); ++i) {
            const int menuId = IdFormatBase + static_cast<int>(i);
            if (menuId > IdFormatMax) break;
            menu.AppendRadioItem(menuId,
                wxString::FromUTF8(loadedFormats_[i].name.c_str()));
            if (loadedFormats_[i].id == selectedFormatId_) {
                menu.Check(menuId, true);
            }
        }
    }
    popupMenuUnder(formatsMenuLabel_, menu);
}

void MainFrame::onOpenDataMenu() {
    wxMenu menu;
    menu.Append(IdImportGames, "Import...");
    menu.Append(IdCreateFormat, "Create Format...");
    menu.Append(IdCreateGameType, "Create Game Type...");
    menu.Append(IdCreateArchetype, "Create Archetype...");
    popupMenuUnder(dataMenuLabel_, menu);
}

void MainFrame::onOpenHelpMenu() {
    wxMenu menu;
    menu.Append(IdAbout, "About...");
    popupMenuUnder(helpMenuLabel_, menu);
}

void MainFrame::showEmptyState() {
    workspace_->Hide();
    emptyPanel_->Show();
    contentPanel_->GetSizer()->Layout();
}

void MainFrame::showWorkspace() {
    emptyPanel_->Hide();
    workspace_->Show();
    contentPanel_->GetSizer()->Layout();
}

void MainFrame::applyTheme() {
    const Theme theme = ctx_.config.current().theme;
    const ThemePalette palette = paletteForTheme(theme);
    applyThemeToWindowTree(this, palette, theme);
    if (workspace_) {
        workspace_->applyTheme();
    }
    Refresh();
}

bool MainFrame::rebuildGamesMenu() {
    auto result = ctx_.gameTitles.listAll();
    if (!result) {
        loadedGames_.clear();
        return false;
    }

    loadedGames_ = std::move(result.value());

    bool found = false;
    for (const auto& g : loadedGames_) {
        if (g.id == selectedGameId_) { found = true; break; }
    }
    if (!found && selectedGameId_ != 0) {
        selectedGameId_ = 0;
        selectedFormatId_ = 0;
        persistSelection();
    }
    return true;
}

bool MainFrame::rebuildFormatsMenu() {
    if (selectedGameId_ == 0) {
        loadedFormats_.clear();
        if (selectedFormatId_ != 0) {
            selectedFormatId_ = 0;
            persistSelection();
        }
        return true;
    }

    auto result = ctx_.formats.listByGame(selectedGameId_);
    if (!result) {
        loadedFormats_.clear();
        return false;
    }

    loadedFormats_ = std::move(result.value());

    bool found = false;
    for (const auto& f : loadedFormats_) {
        if (f.id == selectedFormatId_) { found = true; break; }
    }
    if (!found && selectedFormatId_ != 0) {
        selectedFormatId_ = 0;
        persistSelection();
    }
    return true;
}

void MainFrame::updateTitleAndEmptyState() {
    wxString gameName;
    for (const auto& g : loadedGames_) {
        if (g.id == selectedGameId_) {
            gameName = wxString::FromUTF8(g.name.c_str());
            break;
        }
    }
    wxString formatName;
    for (const auto& f : loadedFormats_) {
        if (f.id == selectedFormatId_) {
            formatName = wxString::FromUTF8(f.name.c_str());
            break;
        }
    }

    if (selectedGameId_ == 0) {
        SetTitle(kAppName);
        centerLabel_->SetLabel("Create or select a game to get started.");
        showEmptyState();
        return;
    }

    if (selectedFormatId_ == 0) {
        SetTitle(wxString::Format("%s - %s", kAppName, gameName));
        centerLabel_->SetLabel(
            wxString::Format("%s\nCreate or select a format.", gameName));
        showEmptyState();
        return;
    }

    SetTitle(wxString::Format("%s - %s - %s", kAppName, gameName, formatName));
}

void MainFrame::selectGame(std::int64_t gameId) {
    selectedGameId_ = gameId;
    rebuildFormatsMenu();

    bool formatOk = false;
    for (const auto& f : loadedFormats_) {
        if (f.id == selectedFormatId_) { formatOk = true; break; }
    }
    if (formatOk) {
        selectFormat(selectedFormatId_);
    } else {
        selectedFormatId_ = 0;
        updateTitleAndEmptyState();
        persistSelection();
    }
}

void MainFrame::selectFormat(std::int64_t formatId) {
    selectedFormatId_ = formatId;
    for (const auto& f : loadedFormats_) {
        if (f.id == formatId) {
            updateTitleAndEmptyState();
            showWorkspace();
            workspace_->loadFormat(formatId);
            persistSelection();
            return;
        }
    }
    updateTitleAndEmptyState();
}

void MainFrame::persistSelection() {
    auto cfg = ctx_.config.current();
    if (cfg.selectedGameId == selectedGameId_ &&
        cfg.selectedFormatId == selectedFormatId_) {
        return;
    }
    cfg.selectedGameId = selectedGameId_;
    cfg.selectedFormatId = selectedFormatId_;
    ctx_.config.store(std::move(cfg));
}

void MainFrame::onSettings(wxCommandEvent&) {
    SettingsDialog dlg(this, ctx_.config);
    themeModalDialog(&dlg, ctx_.config.current().theme);
    if (dlg.ShowModal() == wxID_OK) {
        applyTheme();
    }
}

void MainFrame::onQuit(wxCommandEvent&) {
    Close(true);
}

void MainFrame::onAbout(wxCommandEvent&) {
    const wxString msg = wxString::Format(
        "%s\nVersion: %s",
        kAppName,
        wxString::FromUTF8(kAppVersion));
    showThemedMessageDialog(this, msg, "About", wxOK);
}

void MainFrame::onCreateGame(wxCommandEvent&) {
    CreateGameTitleDialog dlg(this, ctx_.gameTitles);
    themeModalDialog(&dlg, ctx_.config.current().theme);
    if (dlg.ShowModal() == wxID_OK) {
        rebuildGamesMenu();
        selectedFormatId_ = 0;
        selectGame(dlg.createdGame().id);
    }
}

void MainFrame::onCreateFormat(wxCommandEvent&) {
    if (selectedGameId_ == 0) {
        showThemedMessageDialog(this, "Create or select a game first.",
                                "No Game", wxOK);
        return;
    }
    CreateFormatDialog dlg(this, ctx_.formats, selectedGameId_);
    themeModalDialog(&dlg, ctx_.config.current().theme);
    if (dlg.ShowModal() == wxID_OK) {
        rebuildFormatsMenu();
        selectFormat(dlg.createdFormat().id);
    }
}

void MainFrame::onCreateGameType(wxCommandEvent&) {
    if (selectedGameId_ == 0) {
        showThemedMessageDialog(this, "Create or select a game first.",
                                "No Game", wxOK);
        return;
    }
    CreateGameTypeDialog dlg(this, ctx_.gameTypes, selectedGameId_);
    themeModalDialog(&dlg, ctx_.config.current().theme);
    dlg.ShowModal();
}

void MainFrame::onCreateArchetype(wxCommandEvent&) {
    if (selectedGameId_ == 0) {
        showThemedMessageDialog(this, "Create or select a game first.",
                                "No Game", wxOK);
        return;
    }
    CreateArchetypeDialog dlg(this, ctx_.decks, selectedGameId_);
    themeModalDialog(&dlg, ctx_.config.current().theme);
    if (dlg.ShowModal() == wxID_OK && selectedFormatId_ != 0) {
        workspace_->loadFormat(selectedFormatId_);
    }
}

void MainFrame::onImportGames(wxCommandEvent&) {
    if (selectedGameId_ == 0) {
        showThemedMessageDialog(this, "Create or select a game first.",
                                "No Game", wxOK);
        return;
    }
    ImportGamesDialog dlg(this, ctx_);
    themeModalDialog(&dlg, ctx_.config.current().theme);
    if (dlg.ShowModal() == wxID_OK && dlg.importedFormatId() != 0) {
        rebuildFormatsMenu();
        selectFormat(dlg.importedFormatId());
    }
}

void MainFrame::onGameSelected(wxCommandEvent& evt) {
    const int idx = evt.GetId() - IdGameBase;
    if (idx >= 0 && static_cast<std::size_t>(idx) < loadedGames_.size()) {
        selectGame(loadedGames_[idx].id);
    }
}

void MainFrame::onFormatSelected(wxCommandEvent& evt) {
    const int idx = evt.GetId() - IdFormatBase;
    if (idx >= 0 && static_cast<std::size_t>(idx) < loadedFormats_.size()) {
        selectFormat(loadedFormats_[idx].id);
    }
}

}  // namespace tracker::ui
