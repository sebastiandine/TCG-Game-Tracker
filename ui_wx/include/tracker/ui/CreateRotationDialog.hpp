#pragma once

// CreateRotationDialog: name and month-span for a new rotation, or an edit
// of an existing one. The end may be left unknown.

#include "tracker/domain/Rotation.hpp"
#include "tracker/services/FormatService.hpp"

#include <cstdint>

#include <wx/dialog.h>

class wxCheckBox;
class wxChoice;
class wxSpinCtrl;
class wxTextCtrl;

namespace tracker::ui {

class CreateRotationDialog : public wxDialog {
public:
    // `startYear`/`startMonth` pre-fill the span. The end defaults to the
    // same month so the span is valid until the user changes it.
    CreateRotationDialog(wxWindow* parent, FormatService& formats,
                         std::int64_t formatId, int startYear, int startMonth);

    // Edits `existing`. createdRotation() returns the saved row.
    CreateRotationDialog(wxWindow* parent, FormatService& formats,
                         const Rotation& existing);

    [[nodiscard]] const Rotation& createdRotation() const noexcept {
        return created_;
    }

private:
    void build(const wxString& name, int startYear, int startMonth,
               int endYear, int endMonth, bool endUnknown);
    void onToggleEndUnknown(wxCommandEvent&);
    void onOk(wxCommandEvent&);

    FormatService& formats_;
    std::int64_t formatId_{0};
    std::int64_t rotationId_{0};
    bool editing_{false};
    Rotation created_;

    wxTextCtrl* nameCtrl_{nullptr};
    wxChoice*   startMonth_{nullptr};
    wxSpinCtrl* startYear_{nullptr};
    wxChoice*   endMonth_{nullptr};
    wxSpinCtrl* endYear_{nullptr};
    wxCheckBox* endUnknown_{nullptr};
};

}  // namespace tracker::ui
