#include "tracker/ui/RotationFields.hpp"
#include "tracker/ui/Theme.hpp"

#include "tracker/domain/Rotation.hpp"

#include <ctime>

#include <wx/choice.h>
#include <wx/spinctrl.h>

namespace tracker::ui {

void currentYearMonth(int& year, int& month) {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    year = local.tm_year + 1900;
    month = local.tm_mon + 1;
}

const wxString& monthLabel(int month) {
    static const wxString kMonths[] = {
        "January", "February", "March",     "April",   "May",      "June",
        "July",    "August",   "September", "October", "November", "December",
    };
    static const wxString kUnknown = "?";
    if (month < 1 || month > 12) return kUnknown;
    return kMonths[month - 1];
}

wxChoice* makeMonthChoice(wxWindow* parent, int month) {
    auto* choice = new wxChoice(parent, wxID_ANY);
    wxArrayString months;
    for (int i = 1; i <= 12; ++i) months.Add(monthLabel(i));
    choice->Append(months);
    const int index = (month >= 1 && month <= 12) ? month - 1 : 0;
    choice->SetSelection(index);
    return choice;
}

wxSpinCtrl* makeYearSpin(wxWindow* parent, int year) {
    auto* spin = new wxSpinCtrl(parent, wxID_ANY, wxEmptyString,
                                wxDefaultPosition, wxSize(80, -1),
                                wxSP_ARROW_KEYS, 1990, 2100, year);
    return spin;
}

bool confirmOlderRotation(wxWindow* parent, FormatService& formats,
                          std::int64_t formatId, std::int64_t rotationId,
                          const wxString& prompt) {
    if (rotationId == 0) return true;
    auto listed = formats.listRotations(formatId);
    if (!listed) {
        showThemedMessageDialog(parent,
            wxString::FromUTF8(listed.error().c_str()), "Error", wxOK);
        return false;
    }
    const auto latest = latestRotation(listed.value());
    if (!latest || latest->id == rotationId) return true;

    wxString name;
    for (const auto& rotation : listed.value()) {
        if (rotation.id == rotationId) {
            name = wxString::FromUTF8(rotation.name.c_str());
            break;
        }
    }
    const wxString message = wxString::Format(
        "The active rotation \"%s\" is an older rotation. %s",
        name, prompt);
    return showThemedConfirmDialog(parent, message, "Older rotation") == wxID_YES;
}

}  // namespace tracker::ui
