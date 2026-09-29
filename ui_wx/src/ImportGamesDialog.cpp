#include "tracker/ui/ImportGamesDialog.hpp"
#include "tracker/ui/AppContext.hpp"
#include "tracker/ui/Theme.hpp"
#include "tracker/domain/DeckArchetype.hpp"
#include "tracker/import/GameImportParse.hpp"
#include "tracker/ports/IGameImportSink.hpp"

#include <wx/arrstr.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/progdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace tracker::ui {
namespace {

std::string extensionLower(const wxString& path) {
    wxFileName fn(path);
    std::string ext = fn.GetExt().ToStdString(wxConvUTF8);
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return ext;
}

wxString summarize(const GameImportParseResult& parsed,
                   const GameImportReport& report) {
    std::string text;
    if (report.aborted) {
        text += "Import stopped.\n";
    }
    text += "Imported " + std::to_string(report.gamesImported) + " game";
    if (report.gamesImported != 1) text += "s";
    text += ".\nCreated " + std::to_string(report.decksCreated) + " deck";
    if (report.decksCreated != 1) text += "s";
    text += ".\nCreated " + std::to_string(report.gameTypesCreated) +
            " game type";
    if (report.gameTypesCreated != 1) text += "s";
    text += ".";

    std::vector<GameImportIssue> skipped = parsed.issues;
    skipped.insert(skipped.end(), report.skipped.begin(), report.skipped.end());
    if (!skipped.empty()) {
        text += "\n\nSkipped " + std::to_string(skipped.size()) + " row";
        if (skipped.size() != 1) text += "s";
        text += ":";
        const std::size_t shown = std::min<std::size_t>(skipped.size(), 15);
        for (std::size_t i = 0; i < shown; ++i) {
            text += "\n  Row " + std::to_string(skipped[i].lineNumber) + ": " +
                    skipped[i].message;
        }
        if (skipped.size() > shown) {
            text += "\n  ... and " +
                    std::to_string(skipped.size() - shown) + " more.";
        }
    }
    return wxString::FromUTF8(text.c_str());
}

class DeckArchetypePromptDialog : public wxDialog {
public:
    DeckArchetypePromptDialog(wxWindow* parent,
                              const std::string& deckName,
                              const std::vector<DeckArchetype>& archetypes)
        : wxDialog(parent, wxID_ANY, "New deck",
                   wxDefaultPosition, wxSize(420, 160),
                   wxDEFAULT_DIALOG_STYLE) {
        auto* root = new wxBoxSizer(wxVERTICAL);
        const wxString label = wxString::Format(
            "Choose an archetype for \"%s\":",
            wxString::FromUTF8(deckName.c_str()));
        root->Add(new wxStaticText(this, wxID_ANY, label),
                  0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(this, wxID_ANY, "Archetype:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        choice_ = new wxChoice(this, wxID_ANY);
        wxArrayString items;
        for (const auto& a : archetypes) {
            ids_.push_back(a.id);
            items.Add(wxString::FromUTF8(a.name.c_str()));
        }
        choice_->Append(items);
        if (!ids_.empty()) choice_->SetSelection(0);
        row->Add(choice_, 1, wxEXPAND);
        root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

        auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
        if (btns) root->Add(btns, 0, wxALL | wxEXPAND, 10);
        Bind(wxEVT_BUTTON, &DeckArchetypePromptDialog::onOk, this, wxID_OK);
        SetSizer(root);
    }

    [[nodiscard]] std::int64_t archetypeId() const noexcept { return archetypeId_; }

private:
    void onOk(wxCommandEvent&) {
        const int sel = choice_->GetSelection();
        if (sel == wxNOT_FOUND) {
            showThemedMessageDialog(this, "An archetype must be selected.",
                                    "Error", wxOK);
            return;
        }
        archetypeId_ = ids_[static_cast<std::size_t>(sel)];
        EndModal(wxID_OK);
    }

    wxChoice* choice_{nullptr};
    std::vector<std::int64_t> ids_;
    std::int64_t archetypeId_{0};
};

class GameTypeDetailsPromptDialog : public wxDialog {
public:
    GameTypeDetailsPromptDialog(wxWindow* parent, const std::string& eventName)
        : wxDialog(parent, wxID_ANY, "New event",
                   wxDefaultPosition, wxSize(420, 200),
                   wxDEFAULT_DIALOG_STYLE) {
        auto* root = new wxBoxSizer(wxVERTICAL);
        const wxString label = wxString::Format(
            "Choose details for event \"%s\":",
            wxString::FromUTF8(eventName.c_str()));
        root->Add(new wxStaticText(this, wxID_ANY, label),
                  0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

        auto* compRow = new wxBoxSizer(wxHORIZONTAL);
        compRow->Add(new wxStaticText(this, wxID_ANY, "Competitiveness:"),
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        compChoice_ = new wxChoice(this, wxID_ANY);
        compChoice_->Append("Competitive");
        compChoice_->Append("Non-Competitive");
        compChoice_->SetSelection(1);
        compRow->Add(compChoice_, 1, wxEXPAND);
        root->Add(compRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

        auto* medRow = new wxBoxSizer(wxHORIZONTAL);
        medRow->Add(new wxStaticText(this, wxID_ANY, "Medium:"),
                    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        mediumChoice_ = new wxChoice(this, wxID_ANY);
        mediumChoice_->Append("Paper");
        mediumChoice_->Append("Online");
        mediumChoice_->SetSelection(1);
        medRow->Add(mediumChoice_, 1, wxEXPAND);
        root->Add(medRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

        auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
        if (btns) root->Add(btns, 0, wxALL | wxEXPAND, 10);
        Bind(wxEVT_BUTTON, &GameTypeDetailsPromptDialog::onOk, this, wxID_OK);
        SetSizer(root);
    }

    [[nodiscard]] GameTypeChoice choice() const noexcept { return choice_; }

private:
    void onOk(wxCommandEvent&) {
        choice_.competitiveness = (compChoice_->GetSelection() == 0)
            ? Competitiveness::Competitive
            : Competitiveness::NonCompetitive;
        choice_.medium = (mediumChoice_->GetSelection() == 0)
            ? PlayMedium::Paper
            : PlayMedium::Online;
        EndModal(wxID_OK);
    }

    wxChoice* compChoice_{nullptr};
    wxChoice* mediumChoice_{nullptr};
    GameTypeChoice choice_{};
};

class WxImportSink final : public IGameImportSink {
public:
    WxImportSink(wxWindow* parent,
                 Theme theme,
                 std::vector<DeckArchetype> archetypes,
                 wxProgressDialog* progress)
        : parent_(parent),
          theme_(theme),
          archetypes_(std::move(archetypes)),
          progress_(progress) {}

    Result<std::int64_t> chooseArchetype(const std::string& deckName) override {
        DeckArchetypePromptDialog dlg(promptParent(), deckName, archetypes_);
        themeModalDialog(&dlg, theme_);
        if (dlg.ShowModal() != wxID_OK) {
            return Result<std::int64_t>::err(
                "Skipped creating deck \"" + deckName + "\".");
        }
        return Result<std::int64_t>::ok(dlg.archetypeId());
    }

    Result<GameTypeChoice> chooseGameType(const std::string& eventName) override {
        GameTypeDetailsPromptDialog dlg(promptParent(), eventName);
        themeModalDialog(&dlg, theme_);
        if (dlg.ShowModal() != wxID_OK) {
            return Result<GameTypeChoice>::err(
                "Skipped creating event \"" + eventName + "\".");
        }
        return Result<GameTypeChoice>::ok(dlg.choice());
    }

    bool onProgress(std::size_t current, std::size_t total) override {
        if (progress_ == nullptr) return true;
        const int pos = (total == 0)
            ? 0
            : static_cast<int>(current + 1 > total ? total : current + 1);
        const wxString msg = wxString::Format(
            "Importing game %d of %d...",
            static_cast<int>(current + 1),
            static_cast<int>(total));
        return progress_->Update(pos, msg);
    }

private:
    wxWindow* promptParent() {
        if (progress_ != nullptr) return progress_;
        return parent_;
    }

    wxWindow* parent_;
    Theme theme_;
    std::vector<DeckArchetype> archetypes_;
    wxProgressDialog* progress_;
};

}  // namespace

ImportGamesDialog::ImportGamesDialog(wxWindow* parent, AppContext& ctx)
    : wxDialog(parent, wxID_ANY, "Import Games",
               wxDefaultPosition, wxSize(560, 180),
               wxDEFAULT_DIALOG_STYLE),
      ctx_(ctx) {
    Freeze();

    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* fileRow = new wxBoxSizer(wxHORIZONTAL);
    fileRow->Add(new wxStaticText(this, wxID_ANY, "File:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    fileCtrl_ = new wxTextCtrl(this, wxID_ANY);
    fileRow->Add(fileCtrl_, 1, wxEXPAND | wxRIGHT, 6);
    auto* browse = new wxButton(this, wxID_ANY, "Browse...");
    browse->Bind(wxEVT_BUTTON, &ImportGamesDialog::onBrowse, this);
    fileRow->Add(browse, 0);
    root->Add(fileRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    auto* formatRow = new wxBoxSizer(wxHORIZONTAL);
    formatRow->Add(new wxStaticText(this, wxID_ANY, "Format:"),
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    formatChoice_ = new wxChoice(this, wxID_ANY);
    formatRow->Add(formatChoice_, 1, wxEXPAND);
    root->Add(formatRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    auto formats = ctx_.formats.listByGame(ctx_.config.current().selectedGameId);
    if (formats) {
        formats_ = std::move(formats.value());
        wxArrayString items;
        for (const auto& f : formats_) {
            items.Add(wxString::FromUTF8(f.name.c_str()));
        }
        formatChoice_->Append(items);
        if (!formats_.empty()) {
            std::int64_t selected = ctx_.config.current().selectedFormatId;
            int idx = 0;
            for (std::size_t i = 0; i < formats_.size(); ++i) {
                if (formats_[i].id == selected) {
                    idx = static_cast<int>(i);
                    break;
                }
            }
            formatChoice_->SetSelection(idx);
        }
    }

    auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
    if (btns) root->Add(btns, 0, wxALL | wxEXPAND, 10);
    if (auto* ok = wxDynamicCast(FindWindow(wxID_OK), wxButton)) {
        ok->SetLabel("Import");
    }
    Bind(wxEVT_BUTTON, &ImportGamesDialog::onOk, this, wxID_OK);

    SetSizer(root);
    Thaw();
    fileCtrl_->SetFocus();
}

void ImportGamesDialog::onBrowse(wxCommandEvent&) {
    wxFileDialog dlg(this, "Choose import file", wxEmptyString, wxEmptyString,
                     "CSV and Excel files (*.csv;*.xlsx)|*.csv;*.xlsx|"
                     "CSV files (*.csv)|*.csv|"
                     "Excel files (*.xlsx)|*.xlsx",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    themeModalDialog(&dlg, ctx_.config.current().theme);
    if (dlg.ShowModal() == wxID_OK) {
        fileCtrl_->SetValue(dlg.GetPath());
    }
}

void ImportGamesDialog::onOk(wxCommandEvent&) {
    if (formats_.empty() || formatChoice_->GetSelection() == wxNOT_FOUND) {
        showThemedMessageDialog(this, "Create a format first.", "Error", wxOK);
        return;
    }

    const std::string path = fileCtrl_->GetValue().ToStdString(wxConvUTF8);
    if (path.empty()) {
        showThemedMessageDialog(this, "Choose a CSV or XLSX file.", "Error",
                                wxOK);
        return;
    }

    wxFile file;
    if (!file.Open(wxString::FromUTF8(path.c_str()), wxFile::read)) {
        showThemedMessageDialog(this, "Could not open the selected file.",
                                "Error", wxOK);
        return;
    }
    const wxFileOffset len = file.Length();
    if (len < 0) {
        showThemedMessageDialog(this, "Could not read the selected file.",
                                "Error", wxOK);
        return;
    }
    std::string bytes(static_cast<std::size_t>(len), '\0');
    if (len > 0 &&
        file.Read(bytes.data(), static_cast<std::size_t>(len)) != len) {
        showThemedMessageDialog(this, "Could not read the selected file.",
                                "Error", wxOK);
        return;
    }

    const std::string ext = extensionLower(wxString::FromUTF8(path.c_str()));
    Result<std::vector<std::vector<std::string>>> grid =
        Result<std::vector<std::vector<std::string>>>::err(
            "Unsupported file type. Use .csv or .xlsx.");
    if (ext == "csv") {
        grid = parseCsv(bytes);
    } else if (ext == "xlsx") {
        grid = parseXlsx(bytes);
    }
    if (!grid) {
        showThemedMessageDialog(this,
            wxString::FromUTF8(grid.error().c_str()), "Error", wxOK);
        return;
    }

    auto parsed = mapGameImportTable(grid.value());
    if (!parsed) {
        showThemedMessageDialog(this,
            wxString::FromUTF8(parsed.error().c_str()), "Error", wxOK);
        return;
    }

    auto archetypes = ctx_.decks.listByGame(ctx_.config.current().selectedGameId);
    if (!archetypes || archetypes.value().empty()) {
        showThemedMessageDialog(this, "No deck archetypes are available.",
                                "Error", wxOK);
        return;
    }

    const int formatIdx = formatChoice_->GetSelection();
    const std::int64_t formatId = formats_[static_cast<std::size_t>(formatIdx)].id;
    const Theme theme = ctx_.config.current().theme;
    const auto& rows = parsed.value().rows;

    wxProgressDialog progress(
        "Importing games",
        "Starting import...",
        rows.empty() ? 1 : static_cast<int>(rows.size()),
        this,
        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT | wxPD_SMOOTH);
    themeModalDialog(&progress, theme);

    WxImportSink sink(this, theme, archetypes.value(), &progress);
    auto imported = ctx_.gameImport.import(
        formatId, ctx_.config.current().selectedGameId, rows, sink);
    progress.Update(progress.GetRange());
    if (!imported) {
        showThemedMessageDialog(this,
            wxString::FromUTF8(imported.error().c_str()), "Error", wxOK);
        return;
    }

    importedFormatId_ = formatId;
    report_ = std::move(imported.value());
    showThemedMessageDialog(this, summarize(parsed.value(), report_),
                            "Import complete", wxOK);
    EndModal(wxID_OK);
}

}  // namespace tracker::ui
