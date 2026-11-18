// Windows integration test on test-owned editors only; no user-window automation.
// Include the backend to inspect its private region ownership and timer state.
#include "../sys/GuiHoverHelp.cpp"
#include "../sys/praat.h"
#include "../foned/SoundEditor.h"
#include "../foned/SpectrumEditor.h"
#include "../fon/Sound_and_Spectrum.h"
#include <cassert>
#include <iostream>

extern void praat_uvafon_init ();
static void paint (FunctionEditor editor) {
	FunctionEditor_redraw (editor);
	UpdateWindow (editor -> drawingArea -> d_widget -> window);
	MSG message;
	while (PeekMessageW (& message, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage (& message); DispatchMessageW (& message);
	}
}
static int requireText (State *state, const char32_t *part) {
	for (size_t i = 0; i < state -> regions.size(); ++ i)
		if (state -> regions [i].text.find (part) != std::u32string::npos) return int (i);
	std::cerr << "Missing drawn readout help: " << Melder_peek32to8 (part) << "\n";
	for (const auto &r : state -> regions) std::cerr << Melder_peek32to8 (r.text.c_str()) << "\n---\n";
	std::abort ();
}
static void checkNativeTool (FunctionEditor editor, const char32_t *part) {
	const HWND drawing = editor -> drawingArea -> d_widget -> window;
	auto *state = stateFor (editor -> drawingArea, false);
	assert (state && IsWindow (state -> tooltip));
	const int index = requireText (state, part);
	const auto rect = state -> regions [index].bounds;
	const int x = (rect.left + rect.right) / 2, y = (rect.top + rect.bottom) / 2;
	SendMessageW (drawing, WM_MOUSEMOVE, 0, MAKELPARAM (x, y));
	assert (state -> pending >= 0);
	assert (state -> regions [state -> pending].text.find (part) != std::u32string::npos);
	SendMessageW (drawing, WM_MOUSELEAVE, 0, 0);
	assert (state -> pending == -1);
	// Verify that the native tooltip tool accepts owned UTF-16 text and can show.
	state -> shownText = Melder_32to16 (state -> regions [index].text.c_str());
	TOOLINFOW info = tool (drawing); info.lpszText = (LPWSTR) state -> shownText.get();
	SendMessageW (state -> tooltip, TTM_SETMAXTIPWIDTH, 0, 600);
	SendMessageW (state -> tooltip, TTM_UPDATETIPTEXTW, 0, (LPARAM) & info);
	SendMessageW (state -> tooltip, TTM_TRACKPOSITION, 0, MAKELPARAM (100, 100));
	SendMessageW (state -> tooltip, TTM_TRACKACTIVATE, true, (LPARAM) & info);
	assert (IsWindowVisible (state -> tooltip));
	dismiss (drawing, state);
}
int main (int argc, char **argv) {
	praat_init (U"PraatHoverHelpTest", U"1", 1, 2026, 11, 15, U"test", U"localhost", argc, argv);
	praat_uvafon_init ();
	autoSound sound = Sound_createSimple (1, 1.0, 16000.0);
	for (integer i = 1; i <= sound -> nx; ++ i)
		sound -> z [1] [i] = 0.1 * sin (2.0 * NUMpi * 180.0 * (i - 0.5) * sound -> dx);
	autoSoundEditor soundEditor = SoundEditor_create (U"Hover help Sound test", sound.get());
	soundEditor -> startSelection = soundEditor -> endSelection = 0.301;
	soundEditor -> soundAnalysisArea() -> setInstancePref_intensity_show (true);
	soundEditor -> soundAnalysisArea() -> setInstancePref_pitch_show (true);
	soundEditor -> soundAnalysisArea() -> setInstancePref_spectrogram_show (true);
	paint (soundEditor.get());
	auto *soundState = stateFor (soundEditor -> drawingArea, false);
	assert (soundState);
	for (const auto *part : { U"Waveform amplitude scale", U"pitch display", U"green intensity", U"spectrogram/formant panel",
		U"Time cursor", U"Visible time span", U"Total duration", U"Zero waveform amplitude" }) requireText (soundState, part);
	checkNativeTool (soundEditor.get(), U"pitch display");
	checkNativeTool (soundEditor.get(), U"green intensity");
	const size_t regionCount = soundState -> regions.size();
	for (int i = 0; i < 4; ++ i) paint (soundEditor.get());
	assert (soundState -> regions.size() == regionCount); // no stale-region accumulation
	const HWND soundDrawing = soundEditor -> drawingArea -> d_widget -> window;
	const HWND soundTooltip = soundState -> tooltip;
	soundEditor.reset();
	assert (! IsWindow (soundDrawing) && ! IsWindow (soundTooltip));
	autoSpectrum spectrum = Sound_to_Spectrum (sound.get(), true);
	autoSpectrumEditor spectrumEditor = SpectrumEditor_create (U"Hover help Spectrum test", spectrum.get());
	spectrumEditor -> startSelection = spectrumEditor -> endSelection = 300.0;
	spectrumEditor -> spectrumArea() -> cursorHeight = 0.5 * (spectrumEditor -> spectrumArea() -> minimum() + spectrumEditor -> spectrumArea() -> maximum());
	paint (spectrumEditor.get());
	auto *spectrumState = stateFor (spectrumEditor -> drawingArea, false);
	assert (spectrumState);
	for (const auto *part : { U"Selected frequency", U"Spectrum cursor", U"power-density scale", U"Lowest frequency currently",
		U"Highest frequency currently", U"Visible frequency span", U"Total bandwidth", U"Width in Hz of this frequency segment" }) requireText (spectrumState, part);
	checkNativeTool (spectrumEditor.get(), U"power-density scale");
	const auto oldCursor = spectrumState -> regions [requireText (spectrumState, U"Selected frequency")].bounds;
	spectrumEditor -> startWindow = 100.0; spectrumEditor -> endWindow = 1000.0;
	paint (spectrumEditor.get());
	const auto zoomedCursor = spectrumState -> regions [requireText (spectrumState, U"Selected frequency")].bounds;
	assert (std::abs (zoomedCursor.left - oldCursor.left) > 10);
	checkNativeTool (spectrumEditor.get(), U"power-density scale");
	const int width = GuiControl_getWidth (spectrumEditor -> drawingArea);
	const int height = GuiControl_getHeight (spectrumEditor -> drawingArea);
	GuiControl_setSize (spectrumEditor -> drawingArea, width + 160, height + 80);
	paint (spectrumEditor.get());
	const auto resizedCursor = spectrumState -> regions [requireText (spectrumState, U"Selected frequency")].bounds;
	assert (std::abs (resizedCursor.left - zoomedCursor.left) > 10);
	checkNativeTool (spectrumEditor.get(), U"power-density scale");
	const HWND spectrumTooltip = spectrumState -> tooltip;
	spectrumEditor.reset();
	assert (! IsWindow (spectrumTooltip));
	std::cout << "Sound and Spectrum native hover regions, tooltip display, refresh and destruction checks passed\n";
	return 0;
}
