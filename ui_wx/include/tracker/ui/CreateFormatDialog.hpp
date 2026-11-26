#pragma once

// CreateFormatDialog: modal that collects a format name and, when requested,
// the first rotation of a rotating format.

#include "tracker/domain/Rotation.hpp"
#include "tracker/services/FormatService.hpp"

#include <cstdint>

#include <wx/dialog.h>

class wxCheckBox;
class wxChoice;
class wxPanel;
class wxSpinCtrl;
class wxTextCtrl;

namespace tracker::ui {

class CreateFormatDialog : public wxDialog {
public:
    CreateFormatDialog(wxWindow* parent, FormatService& formats,
                       std::int64_t gameId);

    // The created format (valid only after ShowModal() == wxID_OK).
    [[nodiscard]] const Format& createdFormat() const noexcept { return created_; }

    // Set when the format is rotating. Id is 0 otherwise.
    [[nodiscard]] const Rotation& createdRotation() const noexcept {
        return createdRotation_;
    }

private:
    void onToggleRotating(wxCommandEvent&);
    void onToggleEndUnknown(wxCommandEvent&);
    void onOk(wxCommandEvent&);

    FormatService& formats_;
    std::int64_t gameId_{0};
    wxTextCtrl* nameCtrl_{nullptr};
    wxCheckBox* rotatingCheck_{nullptr};
    wxPanel*    rotationPanel_{nullptr};
    wxTextCtrl* rotationName_{nullptr};
    wxChoice*   startMonth_{nullptr};
    wxSpinCtrl* startYear_{nullptr};
    wxChoice*   endMonth_{nullptr};
    wxSpinCtrl* endYear_{nullptr};
    wxCheckBox* endUnknown_{nullptr};
    Format created_;
    Rotation createdRotation_;
};

}  // namespace tracker::ui
