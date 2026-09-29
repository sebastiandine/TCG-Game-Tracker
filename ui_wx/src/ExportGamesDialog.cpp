#include "tracker/ui/ExportGamesDialog.hpp"
#include "tracker/ui/AppContext.hpp"
#include "tracker/ui/Theme.hpp"
#include "tracker/export/GameExportWrite.hpp"

#include <wx/arrstr.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <cstdint>
#include <string>
#include <vector>

namespace tracker::ui {
namespace {

std::string sanitizeFileStem(std::string name) {
    for (char& c : name) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    if (name.empty()) return "games";
    return name;
}

wxString summarize(std::size_t n) {
    std::string text = "Exported " + std::to_string(n) + " game";
    if (n != 1) text += "s";
    text += ".";
    return wxString::FromUTF8(text.c_str());
}

}  // namespace

ExportGamesDialog::ExportGamesDialog(wxWindow* parent, AppContext& ctx)
    : wxDialog(parent, wxID_ANY, "Export Games",
               wxDefaultPosition, wxSize(560, 200),
               wxDEFAULT_DIALOG_STYLE),
      ctx_(ctx) {
    Freeze();

    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* formatRow = new wxBoxSizer(wxHORIZONTAL);
    formatRow->Add(new wxStaticText(this, wxID_ANY, "Format:"),
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    formatChoice_ = new wxChoice(this, wxID_ANY);
    formatRow->Add(formatChoice_, 1, wxEXPAND);
    root->Add(formatRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    auto* typeRow = new wxBoxSizer(wxHORIZONTAL);
    typeRow->Add(new wxStaticText(this, wxID_ANY, "File type:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    fileTypeChoice_ = new wxChoice(this, wxID_ANY);
    typeRow->Add(fileTypeChoice_, 1, wxEXPAND);
    root->Add(typeRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

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

    wxArrayString kinds;
    kinds.Add("CSV");
    kinds.Add("Excel (.xlsx)");
    fileTypeChoice_->Append(kinds);
    fileTypeChoice_->SetSelection(0);

    auto* btns = CreateButtonSizer(wxOK | wxCANCEL);
    if (btns) root->Add(btns, 0, wxALL | wxEXPAND, 10);
    if (auto* ok = wxDynamicCast(FindWindow(wxID_OK), wxButton)) {
        ok->SetLabel("Export");
    }
    Bind(wxEVT_BUTTON, &ExportGamesDialog::onOk, this, wxID_OK);

    SetSizer(root);
    Thaw();
    formatChoice_->SetFocus();
}

void ExportGamesDialog::onOk(wxCommandEvent&) {
    if (formats_.empty() || formatChoice_->GetSelection() == wxNOT_FOUND) {
        showThemedMessageDialog(this, "Create a format first.", "Error", wxOK);
        return;
    }
    if (fileTypeChoice_->GetSelection() == wxNOT_FOUND) {
        showThemedMessageDialog(this, "Choose a file type.", "Error", wxOK);
        return;
    }

    const int formatIdx = formatChoice_->GetSelection();
    const Format& format = formats_[static_cast<std::size_t>(formatIdx)];
    const bool xlsx = fileTypeChoice_->GetSelection() == 1;
    const Theme theme = ctx_.config.current().theme;

    auto games = ctx_.games.listByFormat(format.id);
    if (!games) {
        showThemedMessageDialog(this,
            wxString::FromUTF8(games.error().c_str()), "Error", wxOK);
        return;
    }
    auto decks = ctx_.decks.listByFormat(format.id);
    if (!decks) {
        showThemedMessageDialog(this,
            wxString::FromUTF8(decks.error().c_str()), "Error", wxOK);
        return;
    }
    auto types = ctx_.gameTypes.listByGame(ctx_.config.current().selectedGameId);
    if (!types) {
        showThemedMessageDialog(this,
            wxString::FromUTF8(types.error().c_str()), "Error", wxOK);
        return;
    }

    auto rows = mapGamesForExport(games.value(), decks.value(), types.value());
    if (!rows) {
        showThemedMessageDialog(this,
            wxString::FromUTF8(rows.error().c_str()), "Error", wxOK);
        return;
    }

    auto bytes = xlsx ? writeGameExportXlsx(rows.value())
                      : writeGameExportCsv(rows.value());
    if (!bytes) {
        showThemedMessageDialog(this,
            wxString::FromUTF8(bytes.error().c_str()), "Error", wxOK);
        return;
    }

    const wxString ext = xlsx ? "xlsx" : "csv";
    const wxString wildcard = xlsx
        ? "Excel files (*.xlsx)|*.xlsx"
        : "CSV files (*.csv)|*.csv";
    const wxString suggested = wxString::FromUTF8(
        (sanitizeFileStem(format.name) + "." +
         (xlsx ? "xlsx" : "csv")).c_str());

    wxFileDialog dlg(this, "Save export file", wxEmptyString, suggested,
                     wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    themeModalDialog(&dlg, theme);
    if (dlg.ShowModal() != wxID_OK) return;

    wxFileName fn(dlg.GetPath());
    fn.SetExt(ext);
    const wxString path = fn.GetFullPath();

    wxFile file;
    if (!file.Open(path, wxFile::write)) {
        showThemedMessageDialog(this, "Could not write the selected file.",
                                "Error", wxOK);
        return;
    }
    const std::string& data = bytes.value();
    if (!data.empty() &&
        file.Write(data.data(), data.size()) != data.size()) {
        showThemedMessageDialog(this, "Could not write the selected file.",
                                "Error", wxOK);
        return;
    }

    showThemedMessageDialog(this, summarize(rows.value().size()),
                            "Export complete", wxOK);
    EndModal(wxID_OK);
}

}  // namespace tracker::ui
