#pragma once

// CreateFormatDialog: simple modal that collects a format name.

#include "tracker/services/FormatService.hpp"

#include <cstdint>

#include <wx/dialog.h>
#include <wx/textctrl.h>

namespace tracker::ui {

class CreateFormatDialog : public wxDialog {
public:
    CreateFormatDialog(wxWindow* parent, FormatService& formats,
                       std::int64_t gameId);

    // The created format (valid only after ShowModal() == wxID_OK).
    [[nodiscard]] const Format& createdFormat() const noexcept { return created_; }

private:
    void onOk(wxCommandEvent&);

    FormatService& formats_;
    std::int64_t gameId_{0};
    wxTextCtrl* nameCtrl_{nullptr};
    Format created_;
};

}  // namespace tracker::ui
