/* Appearance themes for the customized Windows build. GPL v3 or later. */
#include "GuiP.h"
#include "GuiTheme.h"
#include "GuiButtonIcons.h"
#include "Preferences.h"
#include <algorithm>
#include <vector>

namespace {
integer theme = 1;   // Classic, Light, Dark, Warm
bool showButtonIcons = true;
#if defined (_WIN32)
struct Palette { COLORREF panel, surface, text, muted, button, hot, border, accent; };
const Palette palettes [] = {
	{ RGB(224,224,224), RGB(255,255,255), RGB(0,0,0), RGB(109,109,109), RGB(240,240,240), RGB(230,230,230), RGB(128,128,128), RGB(0,120,215) },
	{ RGB(239,242,246), RGB(255,255,255), RGB(27,35,46), RGB(90,100,112), RGB(249,250,252), RGB(221,234,249), RGB(154,165,180), RGB(35,102,180) },
	{ RGB(34,38,45), RGB(24,28,34), RGB(232,236,242), RGB(165,175,190), RGB(49,56,67), RGB(64,80,103), RGB(112,127,147), RGB(110,180,255) },
	{ RGB(237,229,211), RGB(252,247,234), RGB(53,44,32), RGB(105,90,68), RGB(246,239,222), RGB(231,214,179), RGB(158,140,111), RGB(132,83,24) }
};
bool highContrast = false;
const Palette &palette () { return palettes [GuiTheme_active() ? theme - 1 : 0]; }
HBRUSH brushes [4] [3] { };   // fixed lifetime: no brush is deleted while Windows can still use it
HBRUSH brush (int role) {
	const int index = GuiTheme_active() ? int (theme - 1) : 0;
	if (! brushes [index] [role]) {
		const auto &p = palettes [index];
		brushes [index] [role] = CreateSolidBrush (role == 0 ? p.panel : role == 1 ? p.surface : p.button);
	}
	return brushes [index] [role];
}
bool isClass (HWND window, const wchar_t *name) {
	wchar_t actual [128] { };
	if (! GetClassNameW (window, actual, 128)) return false;
	return _wcsicmp (actual, name) == 0;
}
void titleBar (HWND window) {
	if (GetWindowLongPtrW (window, GWL_STYLE) & WS_CHILD) return;
	using SetAttribute = HRESULT (WINAPI *)(HWND, DWORD, LPCVOID, DWORD);
	static HMODULE dwm = LoadLibraryW (L"dwmapi.dll");
	static auto setAttribute = dwm ? reinterpret_cast<SetAttribute> (GetProcAddress (dwm, "DwmSetWindowAttribute")) : nullptr;
	if (setAttribute) {
		BOOL dark = GuiTheme_active() && theme == 3;
		setAttribute (window, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, & dark, sizeof dark);
	}
}
void fill (HDC dc, RECT rect, COLORREF colour) {
	SetDCBrushColor (dc, colour); FillRect (dc, & rect, (HBRUSH) GetStockObject (DC_BRUSH));
}
void outline (HDC dc, RECT rect, COLORREF colour) {
	SetDCBrushColor (dc, colour); FrameRect (dc, & rect, (HBRUSH) GetStockObject (DC_BRUSH));
}
GuiButtonIcon buttonIcon (HWND window) {
	return static_cast<GuiButtonIcon> (reinterpret_cast<UINT_PTR> (GetPropW (window, L"PraatButtonIcon")));
}
void updateButtonIcon (HWND window) {
	if (! isClass (window, L"Button")) return;
	const auto style = GetWindowLongPtrW (window, GWL_STYLE);
	const int type = style & BS_TYPEMASK;
	GuiButtonIcon icon = GuiButtonIcon::none;
	if ((type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON) && !(style & (BS_MULTILINE | BS_ICON | BS_BITMAP))) {
		std::vector<wchar_t> label (GetWindowTextLengthW (window) + 1);
		GetWindowTextW (window, label.data(), int (label.size()));
		icon = GuiButtonIcons_forTitle (label.data());
	}
	if (icon == GuiButtonIcon::none) RemovePropW (window, L"PraatButtonIcon");
	else SetPropW (window, L"PraatButtonIcon", reinterpret_cast<HANDLE> (static_cast<UINT_PTR> (icon)));
}
bool iconPainting (HWND window) {
	return showButtonIcons && ! highContrast && buttonIcon (window) != GuiButtonIcon::none;
}
// Reserve a left-hand icon slot only when the entire original label still fits.
bool iconLayout (HDC dc, RECT &text, const wchar_t *label, RECT &icon) {
	TEXTMETRICW metrics; GetTextMetricsW (dc, & metrics);
	const int side = std::min ({24, int (metrics.tmHeight), int (text.bottom - text.top)});
	const int gap = std::max (3, side / 4);
	RECT measured { };
	DrawTextW (dc, label, -1, & measured, DT_SINGLELINE | DT_CALCRECT);
	if (side < 10 || measured.right + side + gap > text.right - text.left) return false;
	icon = { text.left, text.top + (text.bottom - text.top - side) / 2, text.left + side, 0 };
	icon.bottom = icon.top + side;
	text.left += side + gap;
	return true;
}
void paintButton (HWND window, HDC dc) {
	const int saved = SaveDC (dc);
	RECT rect; GetClientRect (window, & rect);
	const auto &p = palette();
	const auto style = GetWindowLongPtrW (window, GWL_STYLE);
	const int type = style & BS_TYPEMASK;
	const bool check = type == BS_CHECKBOX || type == BS_AUTOCHECKBOX || type == BS_3STATE || type == BS_AUTO3STATE;
	const bool radio = type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON;
	const bool enabled = IsWindowEnabled (window), focused = GetFocus() == window;
	const bool pushed = SendMessageW (window, BM_GETSTATE, 0, 0) & BST_PUSHED;
	const bool hot = GetPropW (window, L"PraatThemeHot");
	const bool customTheme = GuiTheme_active();
	const COLORREF ink = customTheme ? (enabled ? p.text : p.muted) : GetSysColor (enabled ? COLOR_BTNTEXT : COLOR_GRAYTEXT);
	fill (dc, rect, customTheme ? (check || radio ? p.panel : (pushed || hot) && enabled ? p.hot : p.button) : GetSysColor (COLOR_BTNFACE));
	RECT text = rect;
	if (check || radio) {
		const int side = std::min (17, int (rect.bottom - rect.top - 4));
		RECT box { 2, (rect.bottom - side) / 2, 2 + side, (rect.bottom + side) / 2 };
		if (radio) {
			SelectObject (dc, GetStockObject (DC_PEN)); SetDCPenColor (dc, enabled ? p.border : p.muted);
			SelectObject (dc, GetStockObject (DC_BRUSH)); SetDCBrushColor (dc, p.surface);
			Ellipse (dc, box.left, box.top, box.right, box.bottom);
		} else {
			fill (dc, box, p.surface); outline (dc, box, enabled ? p.border : p.muted);
		}
		if (SendMessageW (window, BM_GETCHECK, 0, 0) != BST_UNCHECKED) {
			InflateRect (& box, -4, -4);
			if (radio) {
				SetDCBrushColor (dc, enabled ? p.accent : p.muted); SetDCPenColor (dc, enabled ? p.accent : p.muted);
				Ellipse (dc, box.left, box.top, box.right, box.bottom);
			} else fill (dc, box, enabled ? p.accent : p.muted);
		}
		text.left = side + 8;
	} else {
		if (customTheme) outline (dc, rect, focused || type == BS_DEFPUSHBUTTON ? p.accent : p.border);
		else {
			RECT frame = rect;
			if (type == BS_DEFPUSHBUTTON) { outline (dc, frame, GetSysColor (COLOR_WINDOWFRAME)); InflateRect (& frame, -1, -1); }
			DrawFrameControl (dc, & frame, DFC_BUTTON, DFCS_BUTTONPUSH | (pushed ? DFCS_PUSHED : 0) | (enabled ? 0 : DFCS_INACTIVE));
		}
		InflateRect (& text, -4, -2);
		if (pushed) OffsetRect (& text, 1, 1);
	}
	std::vector<wchar_t> label (GetWindowTextLengthW (window) + 1);
	GetWindowTextW (window, label.data(), int (label.size()));
	SelectObject (dc, (HFONT) SendMessageW (window, WM_GETFONT, 0, 0));
	SetBkMode (dc, TRANSPARENT); SetTextColor (dc, ink);
	RECT focus = text;
	if (iconPainting (window)) {
		RECT icon;
		if (iconLayout (dc, text, label.data(), icon))
			GuiButtonIcons_draw (dc, buttonIcon (window), icon, ink);
	}
	UINT flags = (style & BS_MULTILINE ? DT_WORDBREAK : DT_SINGLELINE | DT_VCENTER) | (check || radio ? DT_LEFT : DT_CENTER);
	if (SendMessageW (window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEACCEL) flags |= DT_HIDEPREFIX;
	if (style & BS_MULTILINE) {
		RECT measured = text;
		DrawTextW (dc, label.data(), -1, & measured, flags | DT_CALCRECT);
		text.top += std::max (0L, (text.bottom - text.top - (measured.bottom - measured.top)) / 2);
	}
	DrawTextW (dc, label.data(), -1, & text, flags);
	if (focused && !(SendMessageW (window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS)) {
		InflateRect (& focus, -2, -2); DrawFocusRect (dc, & focus);
	}
	RestoreDC (dc, saved);
}
constexpr UINT_PTR subclassId = 0x5054484d;
LRESULT CALLBACK subclass (HWND window, UINT message, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
	if (message == WM_NCDESTROY) {
		RemovePropW (window, L"PraatThemeHot"); RemovePropW (window, L"PraatThemeEditor"); RemovePropW (window, L"PraatThemeContainer");
		RemovePropW (window, L"PraatThemeAttached");
		RemovePropW (window, L"PraatThemeChanged");
		RemovePropW (window, L"PraatButtonIcon");
		RemoveWindowSubclass (window, subclass, subclassId);
	}
	if ((message == WM_SETTINGCHANGE || message == WM_SYSCOLORCHANGE) &&
		!(GetWindowLongPtrW (window, GWL_STYLE) & WS_CHILD))
		GuiTheme_apply ();   // refresh descendants and tooltip colours on accessibility changes too
	if (GuiTheme_active()) {
		const auto &p = palette();
		if (message >= WM_CTLCOLORMSGBOX && message <= WM_CTLCOLORSTATIC) {
			HDC dc = (HDC) wp;
			const HWND child = (HWND) lp;
			const bool input = message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX || isClass (child, L"Edit");
			SetTextColor (dc, IsWindowEnabled (child) ? p.text : p.muted);
			SetBkColor (dc, input ? p.surface : p.panel);
			return (LRESULT) brush (input ? 1 : 0);
		}
	}
	if (isClass (window, L"Button") && (GuiTheme_active() || iconPainting (window))) {
			if (message == WM_MOUSEMOVE) {
				if (! GetPropW (window, L"PraatThemeHot")) { SetPropW (window, L"PraatThemeHot", (HANDLE) 1); InvalidateRect (window, nullptr, FALSE); }
				TRACKMOUSEEVENT tracking { sizeof tracking, TME_LEAVE, window, 0 }; TrackMouseEvent (& tracking);
			} else if (message == WM_MOUSELEAVE) { RemovePropW (window, L"PraatThemeHot"); InvalidateRect (window, nullptr, FALSE); }
			if (message == WM_PAINT) {
				PAINTSTRUCT ps; HDC dc = BeginPaint (window, & ps); paintButton (window, dc); EndPaint (window, & ps); return 0;
			}
			if (message == WM_PRINTCLIENT) { paintButton (window, (HDC) wp); return 0; }
			if (message == WM_ERASEBKGND) return 1;
	}
	if (GuiTheme_active()) {
		// Containers have a Praat widget, while all drawing areas paint their own canvas.
		if (message == WM_ERASEBKGND) {
			if (GetPropW (window, L"PraatThemeContainer")) {
				RECT rect; GetClientRect (window, & rect); FillRect ((HDC) wp, & rect, brush (0)); return 1;
			}
		}
	}
	const LRESULT result = DefSubclassProc (window, message, wp, lp);
	if (message == WM_SETTEXT) updateButtonIcon (window);
	if ((GuiTheme_active() || showButtonIcons) && isClass (window, L"Button") && (message == BM_SETCHECK || message == BM_SETSTATE ||
		message == WM_ENABLE || message == WM_SETFOCUS || message == WM_KILLFOCUS || message == WM_SETTEXT || message == WM_UPDATEUISTATE))
		InvalidateRect (window, nullptr, FALSE);
	return result;
}
BOOL CALLBACK refreshChild (HWND window, LPARAM) {
	if (GetPropW (window, L"PraatThemeEditor"))
		SetPropW (window, L"PraatThemeChanged", (HANDLE) 1);
	GuiTheme_attach (window); InvalidateRect (window, nullptr, TRUE); return TRUE;
}
BOOL CALLBACK refreshTop (HWND window, LPARAM) {
	DWORD process; GetWindowThreadProcessId (window, & process);
	if (process != GetCurrentProcessId()) return TRUE;
	// Only our registered windows, never system file choosers or message boxes.
	if (! GetPropW (window, L"PraatThemeAttached")) return TRUE;
	GuiTheme_attach (window); EnumChildWindows (window, refreshChild, 0);
	RedrawWindow (window, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	return TRUE;
}
#endif
}
void GuiTheme_prefs () {
	Preferences_addInteger (U"PraatShell.appearanceTheme", & theme, 1);
	Preferences_addBool (U"PraatShell.showButtonIcons", & showButtonIcons, true);
}
bool GuiTheme_buttonIcons () { return showButtonIcons; }
void GuiTheme_setButtonIcons (bool show) { showButtonIcons = show; GuiTheme_apply (); }
integer GuiTheme_get () { return theme >= 1 && theme <= 4 ? theme : 1; }
bool GuiTheme_active () {
#if defined (_WIN32)
	return theme >= 2 && theme <= 4 && ! highContrast;
#else
	return false;
#endif
}
void GuiTheme_set (integer value) {
	Melder_require (value >= 1 && value <= 4, U"Unknown appearance theme.");
	theme = value; GuiTheme_apply ();
}
void GuiTheme_apply () {
#if defined (_WIN32)
	HIGHCONTRASTW contrast { sizeof contrast };
	SystemParametersInfoW (SPI_GETHIGHCONTRAST, sizeof contrast, & contrast, 0);
	highContrast = contrast.dwFlags & HCF_HIGHCONTRASTON;
	EnumWindows (refreshTop, 0);
#endif
}
#if defined (_WIN32)
void GuiTheme_attach (HWND window) {
	if (! window) return;
	if (SetWindowSubclass (window, subclass, subclassId, 0))
		SetPropW (window, L"PraatThemeAttached", (HANDLE) 1);
	updateButtonIcon (window);
	titleBar (window);
	if (isClass (window, TOOLTIPS_CLASSW)) {
		using SetTheme = HRESULT (WINAPI *)(HWND, LPCWSTR, LPCWSTR);
		static HMODULE module = LoadLibraryW (L"uxtheme.dll");
		static auto setTheme = module ? reinterpret_cast<SetTheme> (GetProcAddress (module, "SetWindowTheme")) : nullptr;
		if (setTheme) setTheme (window, GuiTheme_active() ? L"" : nullptr, GuiTheme_active() ? L"" : nullptr);
		SendMessageW (window, TTM_SETTIPBKCOLOR, GuiTheme_active() ? palette().surface : GetSysColor (COLOR_INFOBK), 0);
		SendMessageW (window, TTM_SETTIPTEXTCOLOR, GuiTheme_active() ? palette().text : GetSysColor (COLOR_INFOTEXT), 0);
	}
}
HBRUSH GuiTheme_backgroundBrush () { return brush (0); }
bool GuiTheme_drawCanvasButton (Graphics graphics, HDC dc, RECT rect) {
	if (! GuiTheme_editor (graphics)) return false;
	const int saved = SaveDC (dc);
	fill (dc, rect, palette().button); outline (dc, rect, palette().border);
	RestoreDC (dc, saved); return true;
}
#endif
void GuiTheme_enableEditor (Graphics graphics) {
#if defined (_WIN32)
	if (graphics && graphics -> d_drawingArea)
		SetPropW (graphics -> d_drawingArea -> d_widget -> window, L"PraatThemeEditor", (HANDLE) 1);
#endif
}
bool GuiTheme_editor (Graphics graphics) {
#if defined (_WIN32)
	return GuiTheme_active() && graphics && graphics -> screen && ! graphics -> recording && ! graphics -> printer &&
		graphics -> d_drawingArea && GetPropW (graphics -> d_drawingArea -> d_widget -> window, L"PraatThemeEditor");
#else
	return false;
#endif
}
MelderColour GuiTheme_colour (Graphics graphics, MelderColour colour) {
#if defined (_WIN32)
	if (! GuiTheme_editor (graphics)) return colour;
	const auto convert = [] (COLORREF c) { return MelderColour (GetRValue(c)/255.0, GetGValue(c)/255.0, GetBValue(c)/255.0); };
	const auto &p = palette();
	if (colour.red == .95 && colour.green == .95 && colour.blue == .9) return convert (p.panel);
	if (colour.red == 1 && colour.green == 1 && colour.blue == 1) return convert (p.surface);
	if (colour.red == 0 && colour.green == 0 && colour.blue == 0) return convert (p.text);
	if (colour.red == colour.green && colour.red == colour.blue) {
		const auto foreground = convert (p.text), background = convert (p.surface);
		const double v = colour.red;
		return MelderColour (foreground.red + v * (background.red - foreground.red),
			foreground.green + v * (background.green - foreground.green),
			foreground.blue + v * (background.blue - foreground.blue));
	}
	if (theme == 3) {
		if (colour.red > .9 && colour.green > .6 && colour.blue > .6)
			return MelderColour (.38, .22, .28);   // pink selection/overview surfaces, with light text
		// Keep semantic hues (pitch blue, intensity green, cursor red) while lifting dark ink.
		if (colour.blue > .8 && colour.red < .1 && colour.green < .1) return MelderColour (.50, .71, 1.0);
		const double lift = .50;
		return MelderColour (lift + (1-lift)*colour.red, lift + (1-lift)*colour.green, lift + (1-lift)*colour.blue);
	}
#endif
	return colour;
}
