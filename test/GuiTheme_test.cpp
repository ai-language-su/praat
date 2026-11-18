// Private Windows integration test; does not load or save the user's session.
#include "../sys/GuiTheme.cpp"
#include "../sys/praatP.h"
#include "../sys/GraphicsP.h"
#include "../sys/CommandSearchWindow.h"
#include "../foned/SoundEditor.h"
#include "../foned/SpectrumEditor.h"
#include "../fon/Sound_and_Spectrum.h"
#include <cassert>
#include <iostream>
#include <cstdlib>
// This is a GUI-linked test: report assertions to the runner instead of a modal CRT dialog.
#undef assert
#define assert(condition) do { if (!(condition)) { std::cerr << "Check failed at line " << __LINE__ << ": " << #condition << std::endl; std::exit (1); } } while (false)
extern void praat_uvafon_init ();
static int clicked = 0;
static void click (Thing, GuiButtonEvent) { ++ clicked; }
static bool same (MelderColour a, MelderColour b) {
	return a.red == b.red && a.green == b.green && a.blue == b.blue;
}
static COLORREF colourRef (MelderColour c) { return RGB (c.red*255, c.green*255, c.blue*255); }
static void paint (FunctionEditor editor) {
	FunctionEditor_redraw (editor);
	UpdateWindow (editor -> drawingArea -> d_widget -> window);
	MSG message;
	while (PeekMessageW (& message, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage (& message); DispatchMessageW (& message); }
}
static void buttonColour (HWND window, COLORREF expected) {
	HDC dc = CreateCompatibleDC (nullptr);
	BITMAPINFO info { }; info.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	info.bmiHeader.biWidth = 300; info.bmiHeader.biHeight = -80;
	info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
	void *bits;
	HBITMAP bitmap = CreateDIBSection (dc, & info, DIB_RGB_COLORS, & bits, nullptr, 0);
	auto previous = SelectObject (dc, bitmap);
	SendMessageW (window, WM_PRINTCLIENT, (WPARAM) dc, PRF_CLIENT);
	assert (GetPixel (dc, 5, 5) == expected);
	SelectObject (dc, previous); DeleteObject (bitmap); DeleteDC (dc);
}
int main (int argc, char **argv) {
	praat_init (U"PraatThemeTest", U"1", 1, 2026, 11, 17, U"test", U"localhost", argc, argv);
	praat_uvafon_init ();
	std::cout << "Theme/icon application initialized" << std::endl;
	autoSound sound = Sound_createSimple (1, 1, 16000);
	for (integer i = 1; i <= sound -> nx; ++ i) sound -> z [1] [i] = .15 * sin (i * .071);
	autoSound unchanged = Data_copy (sound.get());
	autoSpectrum spectrum = Sound_to_Spectrum (sound.get(), true);
	autoSoundEditor editor = SoundEditor_create (U"Theme test Sound", sound.get());
	autoSpectrumEditor spectrumEditor = SpectrumEditor_create (U"Theme test Spectrum", spectrum.get());
	GuiWindow window = GuiWindow_create (80, 80, 480, 360, 300, 200, U"Theme controls test", nullptr, nullptr, 0);
	auto button = GuiButton_createShown (window, 20, 220, 20, 52, U"Play", click, nullptr, GuiButton_DEFAULT);
	auto text = GuiText_createShown (window, 20, 440, 65, 96, 0);
	GuiText_setString (text, U"Editable text: å 音");
	auto check = GuiCheckButton_createShown (window, 20, 240, 110, 140, U"Checked option", nullptr, nullptr, GuiCheckButton_SET);
	GuiRadioGroup_begin ();
	auto radio = GuiRadioButton_createShown (window, 240, 440, 110, 140, U"Selected option", nullptr, nullptr, GuiRadioButton_SET);
	GuiRadioGroup_end ();
	auto list = GuiList_createShown (window, 20, 440, 220, 310, false, U"Test list");
	GuiList_insertItem (list, U"Sound theme preview", 0);
	assert (buttonIcon (button -> d_widget -> window) == GuiButtonIcon::play);
	GuiButton_setText (button, U"Copy..."); assert (buttonIcon (button -> d_widget -> window) == GuiButtonIcon::copy);
	GuiButton_setText (button, U"Unmapped action"); assert (buttonIcon (button -> d_widget -> window) == GuiButtonIcon::none);
	GuiButton_setText (button, U"Play");
	HDC layoutDc = GetDC (button -> d_widget -> window);
	const auto oldFont = SelectObject (layoutDc, (HFONT) SendMessageW (button -> d_widget -> window, WM_GETFONT, 0, 0));
	RECT labelBounds {4,2,196,30}, iconBounds;
	assert (iconLayout (layoutDc, labelBounds, L"Play", iconBounds));
	assert (iconBounds.right < labelBounds.left);
	labelBounds = {4,2,30,30};
	assert (! iconLayout (layoutDc, labelBounds, L"Play", iconBounds) && labelBounds.left == 4);
	labelBounds = {4,2,196,30};
	assert (! iconLayout (layoutDc, labelBounds, L"A very long label whose text must never be crowded out", iconBounds));
	SelectObject (layoutDc, oldFont); ReleaseDC (button -> d_widget -> window, layoutDc);
	GuiThing_show (window);
	std::cout << "Button matching and label fitting passed" << std::endl;
	for (int value : { 2, 3, 4, 1, 3 }) {
		GuiTheme_set (value);
		paint (editor.get()); paint (spectrumEditor.get());
		assert (GuiTheme_get() == value);
		GuiTheme_setButtonIcons (false); assert (! iconPainting (button -> d_widget -> window));
		GuiTheme_setButtonIcons (true); assert (iconPainting (button -> d_widget -> window));
		wchar_t label [20]; GetWindowTextW (button -> d_widget -> window, label, 20);
		assert (wcscmp (label, L"Play") == 0);   // icons never enter accessible names or command text
		assert (Data_equal (sound.get(), unchanged.get()));
		assert (SendMessageW (check -> d_widget -> window, BM_GETCHECK, 0, 0) == BST_CHECKED);
		assert (GuiRadioButton_getValue (radio));
		Graphics_setColour (editor -> graphics.get(), Melder_BLACK);
		assert (((GraphicsScreen) editor -> graphics.get()) -> d_winForegroundColour == colourRef (GuiTheme_colour (editor -> graphics.get(), Melder_BLACK)));
		Graphics_setColour (theCurrentPraatPicture -> graphics, Melder_BLACK);
		assert (same (GuiTheme_colour (theCurrentPraatPicture -> graphics, Melder_BLACK), Melder_BLACK));
		if (value != 1) {
			buttonColour (button -> d_widget -> window, palettes [value-1].button);
			HDC dc = GetDC (text -> d_widget -> window);
			HBRUSH response = (HBRUSH) SendMessageW (window -> d_widget -> window, WM_CTLCOLOREDIT, (WPARAM) dc, (LPARAM) text -> d_widget -> window);
			LOGBRUSH lb; GetObject (response, sizeof lb, & lb);
			assert (lb.lbColor == palettes [value-1].surface && GetTextColor (dc) == palettes [value-1].text);
			ReleaseDC (text -> d_widget -> window, dc);
			dc = GetDC (list -> d_widget -> window);
			response = (HBRUSH) SendMessageW (window -> d_widget -> window, WM_CTLCOLORLISTBOX, (WPARAM) dc, (LPARAM) list -> d_widget -> window);
			GetObject (response, sizeof lb, & lb);
			assert (lb.lbColor == palettes [value-1].surface && GetTextColor (dc) == palettes [value-1].text);
			ReleaseDC (list -> d_widget -> window, dc);
		}
	}
	// New controls inherit the live theme without toggling it again.
	std::cout << "Live palette and icon switching passed" << std::endl;
	auto lateButton = GuiButton_createShown (window, 240, 440, 170, 202, U"Filter >", click, nullptr, 0);
	assert (buttonIcon (lateButton -> d_widget -> window) == GuiButtonIcon::filter);
	buttonColour (lateButton -> d_widget -> window, palettes [2].button);
	SendMessageW (button -> d_widget -> window, BM_CLICK, 0, 0); assert (clicked == 1);
	SendMessageW (button -> d_widget -> window, WM_KEYDOWN, VK_SPACE, 0);
	SendMessageW (button -> d_widget -> window, WM_KEYUP, VK_SPACE, 0); assert (clicked == 2);
	EnableWindow (button -> d_widget -> window, false); buttonColour (button -> d_widget -> window, palettes [2].button);
	EnableWindow (button -> d_widget -> window, true);
	// The grayscale transfer is monotonic, including both endpoints (spectrogram energy ordering).
	auto previousGrey = GuiTheme_colour (editor -> graphics.get(), MelderColour (0));
	for (int i = 1; i <= 255; ++ i) {
		auto next = GuiTheme_colour (editor -> graphics.get(), MelderColour (i/255.0));
		assert (next.red <= previousGrey.red && next.green <= previousGrey.green && next.blue <= previousGrey.blue);
		previousGrey = next;
	}
	// Recording/export colours are never transformed, even if originating from an editor.
	Graphics_startRecording (editor -> graphics.get());
	assert (! GuiTheme_editor (editor -> graphics.get()));
	assert (same (GuiTheme_colour (editor -> graphics.get(), Melder_WHITE), Melder_WHITE));
	Graphics_stopRecording (editor -> graphics.get()); Graphics_clearRecording (editor -> graphics.get());
	// Simulate the accessibility state internally without changing Windows settings.
	highContrast = true; assert (! GuiTheme_active());
	assert (! iconPainting (button -> d_widget -> window));
	assert (same (GuiTheme_colour (editor -> graphics.get(), Melder_BLACK), Melder_BLACK));
	highContrast = false;
	structMelderFile prefs { }; MelderFolder_getFile (Melder_preferencesFolder(), U"theme-test.txt", & prefs);
	GuiTheme_set (4); GuiTheme_setButtonIcons (false); Preferences_write_i (& prefs);
	GuiTheme_set (1); GuiTheme_setButtonIcons (true); Preferences_read_i (& prefs);
	assert (GuiTheme_get() == 4 && ! GuiTheme_buttonIcons()); GuiTheme_apply ();
	GuiTheme_setButtonIcons (true);
	const DWORD before = GetGuiResources (GetCurrentProcess(), GR_GDIOBJECTS);
	for (int i = 0; i < 40; ++ i) { GuiTheme_set (i % 4 + 1); paint (editor.get()); }
	const DWORD after = GetGuiResources (GetCurrentProcess(), GR_GDIOBJECTS);
	assert (after <= before + 5);
	GuiTheme_set (3);
	praat_sortActions (); praatP.phase = praat_HANDLING_EVENTS;
	praat_new (Data_copy (sound.get()), U"theme_preview"); praat_updateSelection ();
	int iconCoverage [2] { };
	EnumChildWindows (theCurrentPraatApplication -> topShell -> d_widget -> window,
		[] (HWND window, LPARAM data) -> BOOL {
			if (! isClass (window, L"Button") || ! (GetWindowLongPtrW (window, GWL_STYLE) & WS_VISIBLE)) return TRUE;
			const auto type = GetWindowLongPtrW (window, GWL_STYLE) & BS_TYPEMASK;
			if (type != BS_PUSHBUTTON && type != BS_DEFPUSHBUTTON) return TRUE;
			auto counts = (int *) data;
			++ counts [0];
			if (buttonIcon (window) != GuiButtonIcon::none) ++ counts [1];
			else {
				wchar_t title [256]; char utf8 [1024] { };
				GetWindowTextW (window, title, 256);
				WideCharToMultiByte (CP_UTF8, 0, title, -1, utf8, sizeof utf8, nullptr, nullptr);
				std::cerr << "Unmapped live button: " << utf8 << std::endl;
			}
			return TRUE;
		}, (LPARAM) iconCoverage);
	std::cout << "Live Sound Objects buttons with symbols: " << iconCoverage [1] << "/" << iconCoverage [0] << std::endl;
	assert (iconCoverage [0] >= 19 && iconCoverage [0] == iconCoverage [1]);
	std::cout << "Theme switching, button icons, label fitting, native controls, canvas colours, data/export isolation, grayscale, preferences, accessibility fallback and GDI lifetime checks passed" << std::endl;
	if (std::getenv ("PRAAT_THEME_PREVIEW")) {
		praat_run ();
	}
	GuiObject_destroy (window -> d_widget);
	return 0;
}
