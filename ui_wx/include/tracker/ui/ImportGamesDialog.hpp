#pragma once

// ImportGamesDialog: pick a CSV/XLSX file and format, then import game records.

#include "tracker/domain/Format.hpp"
#include "tracker/domain/Rotation.hpp"
#include "tracker/services/GameImportService.hpp"

#include <cstdint>
#include <vector>

#include <wx/dialog.h>

class wxChoice;
class wxStaticText;
class wxTextCtrl;

namespace tracker::ui {

struct AppContext;

class ImportGamesDialog : public wxDialog {
public:
    ImportGamesDialog(wxWindow* parent, AppContext& ctx);

    [[nodiscard]] std::int64_t importedFormatId() const noexcept {
        return importedFormatId_;
    }
    [[nodiscard]] std::int64_t importedRotationId() const noexcept {
        return importedRotationId_;
    }
    [[nodiscard]] const GameImportReport& report() const noexcept {
        return report_;
    }

private:
    void onBrowse(wxCommandEvent&);
    void onOk(wxCommandEvent&);
    void refreshRotations();

    AppContext& ctx_;
    std::vector<Format> formats_;
    std::vector<Rotation> rotations_;
    std::int64_t        importedFormatId_{0};
    std::int64_t        importedRotationId_{0};
    GameImportReport    report_{};

    wxTextCtrl*   fileCtrl_{nullptr};
    wxChoice*     formatChoice_{nullptr};
    wxStaticText* rotationLabel_{nullptr};
    wxChoice*     rotationChoice_{nullptr};
};

}  // namespace tracker::ui
