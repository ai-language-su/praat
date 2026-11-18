/* Small original vector symbols for Praat buttons. GPL v3 or later. */
#ifndef _GuiButtonIcons_h_
#define _GuiButtonIcons_h_
#include <string>
#include <string_view>

enum class GuiButtonIcon {
	none, help, edit, play, stop, pause, record, draw, query, settings, annotate,
	periodicity, spectrum, intensity, manipulate, convert, filter, combine,
	copy, inspect, info, remove, search, open, save, create, accept, cancel,
	back, zoomIn, zoomOut, zoomAll, zoomSelection, extract, table, sound,
	count
};

GuiButtonIcon GuiButtonIcons_forTitle (std::wstring_view title);

#if defined (_WIN32)
#include <windows.h>
void GuiButtonIcons_draw (HDC dc, GuiButtonIcon icon, RECT bounds, COLORREF colour);
#endif
#endif
