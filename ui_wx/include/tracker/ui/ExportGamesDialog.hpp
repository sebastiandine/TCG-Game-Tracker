#pragma once

// ExportGamesDialog: pick a format and CSV/XLSX, then write game records.

#include "tracker/domain/Format.hpp"

#include <vector>

#include <wx/dialog.h>

class wxChoice;

namespace tracker::ui {

struct AppContext;

class ExportGamesDialog : public wxDialog {
public:
    ExportGamesDialog(wxWindow* parent, AppContext& ctx);

private:
    void onOk(wxCommandEvent&);

    AppContext& ctx_;
    std::vector<Format> formats_;

    wxChoice* formatChoice_{nullptr};
    wxChoice* fileTypeChoice_{nullptr};
};

}  // namespace tracker::ui
