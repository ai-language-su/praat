#include "GuiP.h"
#include "GuiTheme.h"
#include "GuiHoverHelp.h"
#include "GuiHoverHelp_geometry.h"
#include "praat_tooltip.h"
#include <vector>
#include <string>

namespace {
struct Region { GuiHoverHelpRect bounds; std::u32string text; };
// Small labels take precedence over a background region of the whole plot.
static int hit (const std::vector<Region> &regions, int x, int y) {
	return GuiHoverHelp_hit (regions, x, y);
}
#if motif
struct State {
	HWND tooltip = nullptr;
	std::vector<Region> regions;
	autostring16 shownText;
	int pending = -1;
	POINT anchor { };
	bool shown = false;
};
constexpr const wchar_t *property = L"PraatDrawnHoverHelp";
static TOOLINFOW tool (HWND window) {
	TOOLINFOW info { };
	info.cbSize = TTTOOLINFOW_V2_SIZE;
	info.hwnd = window; info.uId = 1;
	info.uFlags = TTF_TRACK | TTF_ABSOLUTE;
	return info;
}
static void dismiss (HWND window, State *state) {
	KillTimer (window, reinterpret_cast<UINT_PTR> (state));
	TOOLINFOW info = tool (window);
	SendMessageW (state -> tooltip, TTM_TRACKACTIVATE, false, (LPARAM) & info);
	state -> pending = -1; state -> shown = false;
}
static void show (HWND window, State *state) {
	if (GetAncestor (window, GA_ROOT) != GetForegroundWindow ()) { dismiss (window, state); return; }
	POINT cursor; GetCursorPos (& cursor);
	POINT local = cursor; ScreenToClient (window, & local);
	const int index = hit (state -> regions, local.x, local.y);
	if (index < 0 || index != state -> pending || GetCapture() == window ||
		(GetAsyncKeyState (VK_LBUTTON) & 0x8000) || (GetAsyncKeyState (VK_RBUTTON) & 0x8000)) {
		dismiss (window, state); return;
	}
	auto bold = praat_boldTooltipQuestions (state -> regions [index].text.c_str());
	state -> shownText = Melder_32to16 (bold.c_str());
	TOOLINFOW info = tool (window);
	info.lpszText = (LPWSTR) state -> shownText.get();
	SendMessageW (state -> tooltip, TTM_UPDATETIPTEXTW, 0, (LPARAM) & info);
	MONITORINFO monitor { sizeof monitor };
	GetMonitorInfoW (MonitorFromPoint (cursor, MONITOR_DEFAULTTONEAREST), & monitor);
	SendMessageW (state -> tooltip, TTM_SETMAXTIPWIDTH, 0,
		std::min (660, int (monitor.rcWork.right - monitor.rcWork.left - 24)));
	SendMessageW (state -> tooltip, TTM_TRACKPOSITION, 0, MAKELPARAM (cursor.x + 16, cursor.y + 22));
	SendMessageW (state -> tooltip, TTM_TRACKACTIVATE, true, (LPARAM) & info);
	RECT rect; GetWindowRect (state -> tooltip, & rect);
	const int width = rect.right - rect.left, height = rect.bottom - rect.top;
	const int x = std::max (int (monitor.rcWork.left), std::min (int (cursor.x + 16), int (monitor.rcWork.right - width - 4)));
	const int preferredY = cursor.y + 22 + height <= monitor.rcWork.bottom ? cursor.y + 22 : cursor.y - height - 12;
	const int y = std::max (int (monitor.rcWork.top), std::min (preferredY, int (monitor.rcWork.bottom - height - 4)));
	SendMessageW (state -> tooltip, TTM_TRACKPOSITION, 0, MAKELPARAM (x, y));
	state -> shown = true;
}
static LRESULT CALLBACK subclass (HWND window, UINT message, WPARAM wParam, LPARAM lParam,
	UINT_PTR id, DWORD_PTR data)
{
	auto *state = reinterpret_cast<State *> (data);
	if (message == WM_MOUSEMOVE) {
		TRACKMOUSEEVENT tracking { sizeof tracking, TME_LEAVE, window, 0 }; TrackMouseEvent (& tracking);
		const int x = (short) LOWORD (lParam), y = (short) HIWORD (lParam);
		const int next = (wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)) ? -1 : hit (state -> regions, x, y);
		if (next != state -> pending || (! state -> shown &&
			(std::abs (x - state -> anchor.x) > 5 || std::abs (y - state -> anchor.y) > 5))) {
			dismiss (window, state);
			state -> pending = next; state -> anchor = { x, y };
			if (next >= 0) SetTimer (window, reinterpret_cast<UINT_PTR> (state), 650, nullptr);
		}
	} else if (message == WM_TIMER && wParam == reinterpret_cast<UINT_PTR> (state)) {
		if (state -> shown) {
			POINT cursor; GetCursorPos (& cursor); ScreenToClient (window, & cursor);
			if (GetAncestor (window, GA_ROOT) != GetForegroundWindow () ||
				hit (state -> regions, cursor.x, cursor.y) != state -> pending ||
				(GetAsyncKeyState (VK_LBUTTON) & 0x8000) || (GetAsyncKeyState (VK_RBUTTON) & 0x8000))
				dismiss (window, state);
		} else {
			KillTimer (window, wParam); show (window, state);
			if (state -> shown) SetTimer (window, wParam, 200, nullptr);
		}
		return 0;
	} else if (message == WM_MOUSELEAVE || message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
		message == WM_MBUTTONDOWN || message == WM_MOUSEWHEEL || message == WM_KILLFOCUS || message == WM_SHOWWINDOW) {
		dismiss (window, state);
	} else if (message == WM_NCDESTROY) {
		dismiss (window, state); DestroyWindow (state -> tooltip);
		RemovePropW (window, property); RemoveWindowSubclass (window, subclass, id); delete state;
	}
	return DefSubclassProc (window, message, wParam, lParam);
}
static State * stateFor (GuiControl control, bool create) {
	if (! control || ! control -> d_widget || ! control -> d_widget -> window) return nullptr;
	const HWND window = control -> d_widget -> window;
	auto *state = reinterpret_cast<State *> (GetPropW (window, property));
	if (state || ! create) return state;
	state = new State;
	INITCOMMONCONTROLSEX controls { sizeof controls, ICC_WIN95_CLASSES }; InitCommonControlsEx (& controls);
	state -> tooltip = CreateWindowExW (WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, window, nullptr, theGui.instance, nullptr);
	if (! state -> tooltip) { delete state; return nullptr; }
	TOOLINFOW info = tool (window); info.lpszText = const_cast<LPWSTR> (L"");
	GuiTheme_attach (state -> tooltip);
	SendMessageW (state -> tooltip, TTM_ADDTOOLW, 0, (LPARAM) & info);
	SetWindowFont (state -> tooltip, theWinGuiNormalLabelFont(), false);
	if (! SetPropW (window, property, state) || ! SetWindowSubclass (window, subclass, 42, (DWORD_PTR) state)) {
		RemovePropW (window, property); DestroyWindow (state -> tooltip); delete state; return nullptr;
	}
	return state;
}
#elif gtk
struct State { std::vector<Region> regions; };
static gboolean query (GtkWidget *, gint x, gint y, gboolean keyboard, GtkTooltip *tooltip, gpointer data) {
	auto *state = static_cast<State *> (data);
	int index = keyboard ? -1 : hit (state -> regions, x, y);
	if (index < 0) return false;
	const auto &r = state -> regions [index];
	gtk_tooltip_set_text (tooltip, Melder_peek32to8 (r.text.c_str()));
	GdkRectangle bounds { r.bounds.left, r.bounds.top, r.bounds.right - r.bounds.left, r.bounds.bottom - r.bounds.top };
	gtk_tooltip_set_tip_area (tooltip, & bounds); return true;
}
static State *stateFor (GuiControl control, bool create) {
	if (! control || ! control -> d_widget) return nullptr;
	auto *state = static_cast<State *> (g_object_get_data (G_OBJECT (control -> d_widget), "praat-hover-help"));
	if (state || ! create) return state;
	state = new State;
	g_object_set_data_full (G_OBJECT (control -> d_widget), "praat-hover-help", state, [] (gpointer p) { delete static_cast<State *> (p); });
	g_signal_connect (G_OBJECT (control -> d_widget), "query-tooltip", G_CALLBACK (query), state);
	gtk_widget_set_has_tooltip (GTK_WIDGET (control -> d_widget), true); return state;
}
#endif
static void add (Graphics g, GuiHoverHelpRect bounds, conststring32 help) {
	if (! g || ! g -> screen || ! g -> d_drawingArea || ! help || ! *help) return;
	#if motif || gtk
		if (auto *state = stateFor (g -> d_drawingArea, false)) state -> regions.push_back ({ bounds, help });
	#else
		(void) bounds;
	#endif
}
}
void GuiHoverHelp_begin (Graphics g) {
	if (! g || ! g -> d_drawingArea) return;
	#if motif || gtk
		if (auto *state = stateFor (g -> d_drawingArea, true)) {
			#if motif
				dismiss (g -> d_drawingArea -> d_widget -> window, state);
			#endif
			state -> regions.clear();
		}
	#endif
}
void GuiHoverHelp_rectangle (Graphics g, double x1, double x2, double y1, double y2, conststring32 help) {
	integer left, right, top, bottom;
	Graphics_WCtoDC (g, x1, y1, & left, & top); Graphics_WCtoDC (g, x2, y2, & right, & bottom);
	add (g, { int (std::min (left, right)), int (std::min (top, bottom)),
		int (std::max (left, right)), int (std::max (top, bottom)) }, help);
}
void GuiHoverHelp_label (Graphics g, double x, double y, conststring32 label, conststring32 help) {
	if (! g -> d_drawingArea || ! label || ! *label || ! help || ! *help) return;
	integer xDC, yDC, endX, ignored;
	Graphics_WCtoDC (g, x, y, & xDC, & yDC);
	Graphics_WCtoDC (g, x + Graphics_textWidth (g, label), y, & endX, & ignored);
	const double height = Graphics_inqFontSize (g) * g -> resolution / 72.0 * 1.25;
	const auto horizontal = g -> horizontalTextAlignment == int (Graphics_RIGHT) ? GuiHoverHelpAlignment::END :
		g -> horizontalTextAlignment == int (Graphics_CENTRE) ? GuiHoverHelpAlignment::CENTRE : GuiHoverHelpAlignment::START;
	const auto vertical = g -> verticalTextAlignment == Graphics_TOP ? GuiHoverHelpAlignment::START :
		g -> verticalTextAlignment == Graphics_HALF ? GuiHoverHelpAlignment::CENTRE : GuiHoverHelpAlignment::END;
	const double baselineCorrection = g -> verticalTextAlignment == Graphics_BASELINE ? height * 0.2 : 0.0;
	add (g, GuiHoverHelp_labelBounds (xDC, yDC + baselineCorrection, std::abs (endX - xDC), height, horizontal, vertical), help);
}
void GuiControl_setHoverHelp (GuiControl control, conststring32 help) {
	#if motif
		if (auto *state = stateFor (control, true)) {
			RECT r; GetClientRect (control -> d_widget -> window, & r);
			state -> regions = { { { r.left, r.top, r.right, r.bottom }, help } };
		}
	#elif gtk
		gtk_widget_set_tooltip_text (GTK_WIDGET (control -> d_widget), Melder_peek32to8 (help));
	#elif cocoa
		[(NSView *) control -> d_widget setToolTip: (NSString *) Melder_peek32toCfstring (help)];
	#endif
}
