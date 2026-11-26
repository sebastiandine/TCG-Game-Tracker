#pragma once

// Shared month/year controls and the older-rotation confirm used by dialogs.

#include "tracker/services/FormatService.hpp"

#include <cstdint>

#include <wx/window.h>

class wxChoice;
class wxSpinCtrl;

namespace tracker::ui {

void currentYearMonth(int& year, int& month);

[[nodiscard]] const wxString& monthLabel(int month);

[[nodiscard]] wxChoice* makeMonthChoice(wxWindow* parent, int month);
[[nodiscard]] wxSpinCtrl* makeYearSpin(wxWindow* parent, int year);

// True when recording may continue. A non-latest rotation asks first.
[[nodiscard]] bool confirmOlderRotation(wxWindow* parent,
                                        FormatService& formats,
                                        std::int64_t formatId,
                                        std::int64_t rotationId,
                                        const wxString& prompt);

}  // namespace tracker::ui
