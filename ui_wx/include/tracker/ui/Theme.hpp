#pragma once

#include "tracker/domain/Enums.hpp"

#include <wx/colour.h>
#include <wx/combo.h>
#include <wx/datetime.h>
#include <wx/gdicmn.h>

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

class wxBitmapButton;
class wxDialog;
class wxWindow;
class wxString;
class wxTextCtrl;

namespace tracker::ui {

struct ThemePalette {
    wxColour windowBg;
    wxColour panelBg;
    wxColour text;
    wxColour inputBg;
    wxColour inputText;
    wxColour buttonBg;
    wxColour buttonText;
};

ThemePalette paletteForTheme(Theme theme);
Theme inferThemeFromWindow(const wxWindow* window);
void applyThemeToWindowTree(wxWindow* root, const ThemePalette& palette, Theme theme);

// Combo + owner-drawn calendar. Native SysMonthCal32 ignores palette text colours
// under Windows visual styles, so Add/Edit Game uses this instead of wxDatePickerCtrl.
class ThemedDatePickerCtrl : public wxComboCtrl {
public:
    ThemedDatePickerCtrl(wxWindow* parent, wxWindowID id = wxID_ANY);
    void SetDate(const wxDateTime& date);
    [[nodiscard]] wxDateTime GetDate() const;
    void ApplyTheme(const ThemePalette& palette, Theme theme);
};

// Force palette colors onto a text input (incl. MSW dark-mode typed-text fix).
void applyPaletteToTextCtrl(wxTextCtrl* text, const ThemePalette& palette, Theme theme);
// Empty-state cue that is never part of GetValue(). Required for wxTE_RICH2 on
// MSW: SetHint() has no cue-banner there and wx writes the string as real text.
void installTextCtrlPlaceholder(wxTextCtrl* text, const wxString& hint);
void themeModalDialog(wxDialog* dlg, Theme theme);
int showThemedMessageDialog(wxWindow* parent, const wxString& message, const wxString& caption, long style);
int showThemedConfirmDialog(wxWindow* parent, const wxString& message, const wxString& caption);

// Hand cursor while the pointer is over a row that does something on click.
void installActionableRowCursor(
    wxWindow* window,
    std::function<bool(const wxPoint& clientPos)> isActionable);

// Show/hide the shared or per-game Edit toolbar button and reflow its sizer
// so Add/Delete close the gap when Edit is hidden for multi-select.
void setToolbarEditVisible(wxBitmapButton* edit, bool visible);

// Confirm copy for Delete: one card by name, or "Delete N selected entries?".
wxString deleteCardsConfirmMessage(std::size_t count, std::string_view singleCardName);

}  // namespace tracker::ui
