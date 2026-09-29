#pragma once

// ImportGamesDialog: pick a CSV/XLSX file and format, then import game records.

#include "tracker/domain/Format.hpp"
#include "tracker/services/GameImportService.hpp"

#include <cstdint>
#include <vector>

#include <wx/dialog.h>

class wxChoice;
class wxTextCtrl;

namespace tracker::ui {

struct AppContext;

class ImportGamesDialog : public wxDialog {
public:
    ImportGamesDialog(wxWindow* parent, AppContext& ctx);

    [[nodiscard]] std::int64_t importedFormatId() const noexcept {
        return importedFormatId_;
    }
    [[nodiscard]] const GameImportReport& report() const noexcept {
        return report_;
    }

private:
    void onBrowse(wxCommandEvent&);
    void onOk(wxCommandEvent&);

    AppContext& ctx_;
    std::vector<Format> formats_;
    std::int64_t        importedFormatId_{0};
    GameImportReport    report_{};

    wxTextCtrl* fileCtrl_{nullptr};
    wxChoice*   formatChoice_{nullptr};
};

}  // namespace tracker::ui
