#pragma once

// ExportGamesDialog: pick a format and CSV/XLSX, then write game records.

#include "tracker/domain/Format.hpp"
#include "tracker/domain/Rotation.hpp"

#include <vector>

#include <wx/dialog.h>

class wxChoice;
class wxStaticText;

namespace tracker::ui {

struct AppContext;

class ExportGamesDialog : public wxDialog {
public:
    ExportGamesDialog(wxWindow* parent, AppContext& ctx);

private:
    void onOk(wxCommandEvent&);
    void refreshRotations();

    AppContext& ctx_;
    std::vector<Format> formats_;
    std::vector<Rotation> rotations_;

    wxChoice*     formatChoice_{nullptr};
    wxStaticText* rotationLabel_{nullptr};
    wxChoice*     rotationChoice_{nullptr};
    wxChoice*     fileTypeChoice_{nullptr};
};

}  // namespace tracker::ui
