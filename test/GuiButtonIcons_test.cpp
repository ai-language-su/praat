// Standalone symbol matching and GDI rendering checks; no application windows.
#include "../sys/GuiButtonIcons.h"
#include <cassert>
#include <iostream>
#include <vector>

int main () {
	using I = GuiButtonIcon;
	assert (GuiButtonIcons_forTitle (L"View && Edit") == I::edit);
	assert (GuiButtonIcons_forTitle (L"\u207aSound help") == I::help);
	assert (GuiButtonIcons_forTitle (L"Analyse spectrum  >") == I::spectrum);
	assert (GuiButtonIcons_forTitle (L"\u202f\u202f   Analyse spectrum  >") == I::spectrum);
	assert (GuiButtonIcons_forTitle (L"\u00a0 Query >") == I::query);
	assert (GuiButtonIcons_forTitle (L"Analyse periodicity -") == I::periodicity);
	assert (GuiButtonIcons_forTitle (L"To Intensity\u2026") == I::intensity);
	assert (GuiButtonIcons_forTitle (L"Filter (pass Hann band)...") == I::filter);
	assert (GuiButtonIcons_forTitle (L"View & Edit || Edit") == I::edit);
	assert (GuiButtonIcons_forTitle (L"Copy...") == I::copy);
	assert (GuiButtonIcons_forTitle (L"Remove") == I::remove);
	assert (GuiButtonIcons_forTitle (L"Play") != GuiButtonIcons_forTitle (L"Pause"));
	assert (GuiButtonIcons_forTitle (L"Pause") != GuiButtonIcons_forTitle (L"Stop"));
	// Word boundaries prevent accidental partial-word matches; unknowns have no invented symbol.
	for (const auto *title : {L"Playbacker", L"Together", L"Filterbanker", L"Getaway", L"My research command", L"音", L"", L"---"})
		assert (GuiButtonIcons_forTitle (title) == I::none);
	// Every visible Sound action and fixed Objects button has a reviewed symbol.
	for (const auto *title : {L"Sound help", L"View & Edit", L"Play", L"Draw >", L"Query >", L"Modify >", L"Annotate >",
		L"Analyse periodicity >", L"Analyse spectrum >", L"To Intensity...", L"Manipulate >", L"Convert >", L"Filter >", L"Combine >",
		L"Rename...", L"Copy...", L"Inspect", L"Info", L"Remove", L"all", L"in", L"out", L"sel", L"bak"})
		assert (GuiButtonIcons_forTitle (title) != I::none);
#if defined (_WIN32)
	HDC dc = CreateCompatibleDC (nullptr);
	BITMAPINFO info { }; info.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	info.bmiHeader.biWidth = 40; info.bmiHeader.biHeight = -40;
	info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
	void *bits; HBITMAP bitmap = CreateDIBSection (dc, & info, DIB_RGB_COLORS, & bits, nullptr, 0);
	auto previous = SelectObject (dc, bitmap);
	const COLORREF background = RGB(24,28,34), ink = RGB(232,236,242);
	const DWORD resources = GetGuiResources (GetCurrentProcess(), GR_GDIOBJECTS);
	for (int side : {12,16,24}) for (int i = 1; i < int (I::count); ++ i) {
		RECT canvas {0,0,40,40}; SetDCBrushColor (dc, background); FillRect (dc, & canvas, (HBRUSH) GetStockObject (DC_BRUSH));
		RECT bounds {8,8,8+side,8+side};
		GuiButtonIcons_draw (dc, static_cast<I> (i), bounds, ink);
		int pixels = 0;
		for (int y = 0; y < 40; ++ y) for (int x = 0; x < 40; ++ x) {
			const COLORREF pixel = GetPixel (dc, x, y);
			if (x < 8 || x >= 8+side || y < 8 || y >= 8+side) assert (pixel == background);
			if (pixel == ink) ++ pixels;
		}
		assert (pixels > 5);   // every defined symbol renders and respects its clipping rectangle
	}
	assert (GetGuiResources (GetCurrentProcess(), GR_GDIOBJECTS) == resources);
	SelectObject (dc, previous); DeleteObject (bitmap); DeleteDC (dc);
#endif
	std::cout << "Button symbol matching, Sound coverage, clipping, scaling and GDI lifetime checks passed\n";
}
