// Windows integration test. Run with a private --pref-dir.
#include "BatchAnalysis.h"
#include "CommandSearchWindow.h"
#include "CorpusAnnotations.h"
#include "GuiP.h"
#include "InfoEditor.h"
#include "Ltas.h"
#include "praatP.h"
#include "praat_script.h"
#include "Sound.h"
#include "Spectrum.h"
#include "TextGrid.h"
#include "TextGridEditor.h"
#include <shlobj.h>
#include <array>
#include <fstream>
#include <iostream>

extern void praat_uvafon_init ();
namespace fs = std::filesystem;
using namespace BatchAnalysis;

#define check(condition) do { \
	if (! (condition)) { \
		std::cerr << "Check failed at line " << __LINE__ << ": " << #condition << std::endl; \
		std::_Exit (1); \
	} \
} while (false)

static structMelderFile fileFor (const fs::path &path) {
	structMelderFile file {};
	Melder_pathToFile (Melder_peek8to32_u (path.u8string().c_str()), & file);
	return file;
}

static void fixture (const fs::path &folder, const char32_t *tier) {
	fs::create_directories (folder);
	auto sound = Sound_createSimple (1, 1.0, 16000.0);
	for (integer i = 1; i <= 6400; ++ i) {
		const double cycle = i * NUM2pi * 140.0 / 16000.0;
		for (integer harmonic = 1; harmonic <= 20; ++ harmonic)
			sound -> z [1] [i] += 0.18 * sin (harmonic * cycle) / harmonic;
	}
	auto recording = fileFor (folder / "Take.WAV");
	Sound_saveAsAudioFile (sound.get(), & recording, Melder_WAV, 16);
	auto grid = TextGrid_create (0.0, 1.0, tier, U"");
	TextGrid_insertBoundary (grid.get(), 1, 0.4);
	TextGrid_insertBoundary (grid.get(), 1, 0.5);
	TextGrid_setIntervalText (grid.get(), 1, 1, U"æ,\"é\"");
	TextGrid_setIntervalText (grid.get(), 1, 3, U"silence");
	auto annotations = fileFor (folder / "take.TextGrid");
	Data_writeToTextFile (grid.get(), & annotations);
}

static void testAnalysis (const fs::path &root) {
	const auto input = root / "corpus";
	fixture (input / "a", U"vowels");
	fixture (input / "b", U"vowels");
	fixture (input / "wrong-tier", U"words");
	fixture (input / "corrupt-audio", U"vowels");
	std::ofstream (input / "corrupt-audio" / "Take.WAV", std::ios::trunc) << "bad audio";
	fixture (input / "ambiguous", U"vowels");
	std::ofstream (input / "ambiguous" / "Take.FLAC") << "duplicate basename";
	fs::copy_file (input / "a" / "Take.WAV", input / "missing.WAV");
	fs::copy_file (input / "a" / "take.TextGrid", input / "orphan.TextGrid");
	Preset preset;
	preset.pitch = true;
	const auto pairs = scan (input, preset);
	check (pairs.size() == 7);
	check (pairs [0].matchingIntervals == 2 && pairs [1].matchingIntervals == 0 && pairs [2].matchingIntervals == 2);
	check (pairs [0].audio == fs::path ("a/Take.WAV") && pairs [2].audio == fs::path ("b/Take.WAV"));
	check (pairs [1].problem.find (U"Ambiguous") != std::u32string::npos);
	check (pairs [4].problem == U"Missing TextGrid" && pairs [5].problem == U"Missing recording");
	check (pairs [6].problem.find (U"missing") != std::u32string::npos);
	const auto preview = run (input, preset, pairs, { 0 });
	const auto results = run (input, preset, pairs, { 0, 2, 3 });
	check (preview.rows.size() == 2 && results.rows.size() == 4);
	check (results.attempted == 3 && results.succeeded == 2 && results.failures.size() == 1);
	const auto &vowel = results.rows [0];
	check (vowel.interval == 1 && vowel.start == 0.0 && vowel.end == 0.4 && vowel.label == U"æ,\"é\"");
	check (vowel.f0 > 135 && vowel.f0 < 145 && isdefined (vowel.f1) && isdefined (vowel.f2));
	check (preview.rows [0].f0 == vowel.f0 && preview.rows [0].f1 == vowel.f1 && preview.rows [0].f2 == vowel.f2);
	check (results.rows [1].interval == 3 && ! isdefined (results.rows [1].f0));
	check (results.rows [1].problem.find (U"No voiced pitch frames") != std::u32string::npos);
	const auto output = exportRun (root, input, preset, pairs, results);
	check (fs::exists (output / "measurements.csv") && fs::exists (output / "files.csv") && fs::exists (output / "run-settings.txt"));
	check (presetText (loadPreset (output / "settings.PraatBatchPreset")) == presetText (preset));
	const auto path = root / "Custom.PraatBatchPreset";
	preset.label = U"æ,\"é\"";
	preset.formantCeiling = 5250;
	savePreset (preset, path, false);
	check (presetText (loadPreset (path)) == presetText (preset));
	bool duplicate = false;
	try { savePreset (Preset {}, path, false); }
	catch (MelderError) { Melder_clearError (); duplicate = true; }
	check (duplicate && loadPreset (path).label == preset.label);
	const auto filtered = scan (input, loadPreset (path));
	check (filtered [0].matchingIntervals == 1 && run (input, preset, filtered, { 0 }).rows.size() == 1);
	savePreset (Preset {}, path, true);
	check (loadPreset (path).label.empty());
}

struct WindowQuery { const wchar_t *className, *title; HWND match = nullptr; };
static BOOL CALLBACK find (HWND hwnd, LPARAM data) {
	auto &query = *reinterpret_cast<WindowQuery *> (data);
	wchar_t className [64] {}, title [256] {};
	GetClassNameW (hwnd, className, 64);
	GetWindowTextW (hwnd, title, 256);
	if ((! query.className || wcscmp (query.className, className) == 0) && (! query.title || wcscmp (query.title, title) == 0)) {
		query.match = hwnd;
		return FALSE;
	}
	return TRUE;
}
static HWND topWindow (const wchar_t *title) {
	WindowQuery query { nullptr, title };
	EnumThreadWindows (GetCurrentThreadId (), find, (LPARAM) & query);
	return query.match;
}
static HWND control (HWND window, const wchar_t *className, const wchar_t *title = nullptr) {
	WindowQuery query { className, title };
	EnumChildWindows (window, find, (LPARAM) & query);
	check (query.match);
	return query.match;
}
struct FolderPicker {
	HWND owner;
	fs::path choice;
	bool handled = false;
};
static FolderPicker *folderPicker;
static BOOL CALLBACK handleFolderPicker (HWND hwnd, LPARAM data) {
	auto &picker = *reinterpret_cast<FolderPicker *> (data);
	wchar_t className [32] {};
	GetClassNameW (hwnd, className, 32);
	if (wcscmp (className, L"#32770") != 0 || GetWindow (hwnd, GW_OWNER) != picker.owner) return TRUE;
	auto button = GetDlgItem (hwnd, picker.choice.empty() ? IDCANCEL : IDOK);
	if (! button) return TRUE;
	if (! picker.choice.empty()) SendMessageW (hwnd, BFFM_SETSELECTIONW, TRUE, (LPARAM) picker.choice.c_str());
	if (! IsWindowEnabled (button)) return TRUE;
	picker.handled = true;
	PostMessageW (button, BM_CLICK, 0, 0);
	return FALSE;
}
static void CALLBACK provideFolder (HWND, UINT, UINT_PTR timer, DWORD) {
	EnumThreadWindows (GetCurrentThreadId (), handleFolderPicker, (LPARAM) folderPicker);
	if (folderPicker -> handled) KillTimer (nullptr, timer);
}
static void chooseFolder (HWND owner, const wchar_t *button, fs::path folder = {}) {
	FolderPicker picker { owner, std::move (folder) };
	folderPicker = & picker;
	const auto timer = SetTimer (nullptr, 0, 60, provideFolder);
	check (timer);
	SendMessageW (control (owner, L"Button", button), BM_CLICK, 0, 0);
	KillTimer (nullptr, timer);
	folderPicker = nullptr;
	check (picker.handled);
}
static void testWindow (const fs::path &root) {
	CommandSearchWindow_show (nullptr);
	const HWND search = topWindow (L"Search Praat");
	check (search);
	SetWindowTextW (control (search, L"Edit"), L"Batch Analysis");
	CommandSearchWindow_refresh ();
	const HWND entries = control (search, L"ListBox");
	wchar_t entry [256] {};
	check (SendMessageW (entries, LB_GETTEXT, 0, (LPARAM) entry) != LB_ERR);
	check (wcsstr (entry, L"Batch Analysis...") && wcsstr (entry, L"Objects > Tools"));
	SendMessageW (control (search, L"Button", L"Run command"), BM_CLICK, 0, 0);
	const HWND batch = topWindow (L"Batch Analysis");
	check (batch && IsWindowVisible (batch));
	check (! IsWindowEnabled (control (batch, L"Button", L"Export run…")));
	SendMessageW (control (batch, L"Button", L"Preview"), BM_CLICK, 0, 0);
	control (batch, L"Static", L"Preview measures up to three file pairs.");
	chooseFolder (batch, L"Choose folder…", root / "corpus");
	const auto rows = control (batch, L"ListBox");
	check (SendMessageW (rows, LB_GETCOUNT, 0, 0) == 7);
	chooseFolder (batch, L"Choose folder…");
	check (SendMessageW (rows, LB_GETCOUNT, 0, 0) == 7);
	check (SendMessageW (rows, LB_SETSEL, TRUE, 2) != LB_ERR);
	SendMessageW (control (batch, L"Button", L"Preview files"), BM_CLICK, 0, 0);
	check (SendMessageW (rows, LB_GETCOUNT, 0, 0) == 2);
	SendMessageW (control (batch, L"Button", L"Run analysis"), BM_CLICK, 0, 0);
	check (SendMessageW (rows, LB_GETCOUNT, 0, 0) == 5);
	check (IsWindowEnabled (control (batch, L"Button", L"Export run…")));
	const auto destination = root / "ui-export";
	fs::create_directory (destination);
	chooseFolder (batch, L"Export run…", destination);
	const auto exported = fs::directory_iterator (destination);
	check (exported != fs::directory_iterator() && fs::exists (exported -> path() / "measurements.csv"));
	SendMessageW (control (batch, L"Button", L"Close"), BM_CLICK, 0, 0);
	check (! IsWindowVisible (batch));
}

static void testSpectrumHoverHelp () {
	const auto before = theCurrentPraatObjects -> n;
	praat_new (Spectrum_create (8000.0, 256), U"hover_test");
	praat_updateSelection ();
	integer queries = 0;
	Praat_Command realValue = nullptr, queryMenu = nullptr, unknown = nullptr;
	for (integer i = 1; i <= praat_getNumberOfActions (); ++ i) {
		const auto action = praat_getAction (i);
		if (! action -> title) continue;
		const auto title = action -> title.get();
		if (action -> class1 == classSpectrum && ! action -> class2) {
			if (! action -> callback && Melder_startsWith (title, U"Query")) queryMenu = action;
			if (! action -> callback || ! action -> visible || ! Melder_startsWith (title, U"Get ")) continue;
			++ queries;
			check (praat_actionSummary (action));
			const auto guide = praat_actionGuide (action);
			check (! guide || ! str32str (guide, U"What is a query?"));
			check (static_cast<GuiMenuItem> (action -> button) -> d_tooltipText);
			if (str32equ (title, U"Get real value in bin...")) realValue = action;
		}
		if (action -> class1 == classMatrix && ! action -> class2 && str32equ (title, U"Get number of rows")) unknown = action;
	}
	check (queries == 19 && realValue && queryMenu && unknown);
	const auto tooltip = Melder_peek16to32 (static_cast<GuiMenuItem> (realValue -> button) -> d_tooltipText.get());
	check (str32str (tooltip, U"Fourier coefficient") && str32str (tooltip, U"numbered frequency sample"));
	check (str32str (tooltip, U"cosine component") && str32str (tooltip, U"band energy"));
	check (str32str (praat_actionGuide (queryMenu), U"What is a query?"));
	check (! praat_actionSummary (unknown) && ! praat_actionGuide (unknown));
	praat_removeObject (before + 1);
}

static void click (HWND window, const wchar_t *button) {
	SendMessageW (control (window, L"Button", button), BM_CLICK, 0, 0);
}

static void pressControlA (HWND window, bool shift = false) {
	std::array<BYTE, 256> previous {};
	check (GetKeyboardState (previous.data()));
	auto keys = previous;
	keys [VK_CONTROL] = keys [VK_LCONTROL] = 0x80;
	keys [VK_SHIFT] = keys [VK_LSHIFT] = shift ? 0x80 : 0;
	keys [VK_MENU] = 0;
	check (SetKeyboardState (keys.data()));
	MSG event { window, WM_KEYDOWN, 'A', 1 };
	XtDispatchEvent (& event);
	check (SetKeyboardState (previous.data()));
}

static void testSelectAll () {
	const auto before = theCurrentPraatObjects -> n;
	praat_new (Sound_createSimple (1, 0.5, 16000), U"select_all_test");
	praat_updateSelection ();
	check (praat_doAction (U"View & Edit", U"", nullptr));
	const auto editor = static_cast<FunctionEditor> (theCurrentPraatObjects -> list [before + 1].editors [0]);
	check (editor);
	FunctionEditor_ungroup (editor -> data());
	editor -> startWindow = 0.1;
	editor -> endWindow = 0.3;
	editor -> startSelection = 0.15;
	editor -> endSelection = 0.2;
	FunctionEditor_windowMarksChanged (editor, true);
	const HWND view = editor -> drawingArea -> d_widget -> window;
	pressControlA (view);
	check (editor -> startSelection == editor -> tmin && editor -> endSelection == editor -> tmax);
	check (editor -> startWindow == 0.1 && editor -> endWindow == 0.3);
	editor -> startSelection = 0.2;
	editor -> endSelection = 0.25;
	pressControlA (view, true);
	check (editor -> startWindow == editor -> tmin && editor -> endWindow == editor -> tmax);
	check (editor -> startSelection == 0.2 && editor -> endSelection == 0.25);
	praat_removeObject (before + 1);
	praat_show ();

	Melder_information (U"Keep text shortcuts working");
	const auto info = InfoEditor_getTheReferenceToTheOnlyInstance ();
	const HWND text = info -> textWidget -> d_widget -> window;
	SendMessageW (text, EM_SETSEL, 2, 2);
	pressControlA (text);
	DWORD start = 0, end = 0;
	SendMessageW (text, EM_GETSEL, (WPARAM) & start, (LPARAM) & end);
	check (start == 0 && end == (DWORD) GetWindowTextLengthW (text));
}

static void testLiveBinCount () {
	const auto before = theCurrentPraatObjects -> n;
	praat_new (Spectrum_create (8000.0, 64), U"first");
	praat_new (Spectrum_create (8000.0, 128), U"second");
	praat_new (Ltas_create (25, 100.0), U"average");
	praat_new (Sound_createSimple (1, 0.1, 16000), U"sound");
	praat_updateSelection ();
	const auto select = [&] (integer offset) {
		praat_deselectAll ();
		praat_select (before + offset);
		praat_show ();
	};
	select (1);
	check (praat_doAction (U"Get number of bins", U"", nullptr));
	const auto info = InfoEditor_getTheReferenceToTheOnlyInstance ();
	const HWND window = topWindow (L"Praat Info");
	const HWND follow = control (window, L"Button", L"Follow selected object");
	check (IsWindowVisible (follow) && SendMessageW (follow, BM_GETCHECK, 0, 0) == BST_UNCHECKED);
	const auto contains = [&] (conststring32 text) { return str32str (GuiText_getString (info -> textWidget).get(), text) != nullptr; };
	check (contains (U"64 bins"));
	select (2);
	check (contains (U"64 bins"));
	click (window, L"Follow selected object");
	check (contains (U"128 bins") && contains (U"Spectrum second"));
	select (3);
	check (contains (U"25 bins") && contains (U"Ltas average"));
	select (4);
	check (contains (U"Select one Spectrum or LTAS"));
	select (1);
	praat_select (before + 2);
	praat_show ();
	check (contains (U"Select one Spectrum or LTAS"));
	select (1);
	check (contains (U"64 bins"));
	click (window, L"Follow selected object");
	select (2);
	check (contains (U"64 bins"));
	Melder_information (U"Ordinary Info output");
	check (! IsWindowVisible (follow));
	select (3);
	check (contains (U"Ordinary Info output"));
	praat_executeScriptFromText (U"binCount = Get number of bins\nassert binCount = 25\n");
	check (! IsWindowVisible (follow) && contains (U"Ordinary Info output"));
	check (praat_doAction (U"Get number of bins", U"", nullptr));
	check (IsWindowVisible (follow) && contains (U"25 bins"));
	while (theCurrentPraatObjects -> n > before) praat_removeObject (theCurrentPraatObjects -> n);
	praat_show ();
}

static void testSearchScope () {
	praat_show ();
	CommandSearchWindow_show (nullptr);
	const auto window = topWindow (L"Search Praat");
	const auto query = control (window, L"Edit");
	const auto list = control (window, L"ListBox");
	const auto run = control (window, L"Button", L"Run command");
	const auto find = [&] (const wchar_t *title, const wchar_t *path) {
		const auto count = SendMessageW (list, LB_GETCOUNT, 0, 0);
		for (LRESULT i = 0; i < count; ++ i) {
			const auto length = SendMessageW (list, LB_GETTEXTLEN, i, 0);
			check (length >= 0);
			std::wstring row ((size_t) length + 1, L'\0');
			SendMessageW (list, LB_GETTEXT, i, (LPARAM) row.data());
			if (row.rfind (title, 0) == 0 && row.find (path) != row.npos) return true;
		}
		return false;
	};
	const auto search = [&] (const wchar_t *text) {
		check (SetWindowTextW (query, text));
		CommandSearchWindow_refresh ();
	};
	search (L"Batch Analysis");
	check (find (L"Batch Analysis...", L"Objects > Tools"));
	click (window, L"Objects menus");
	check (! find (L"Batch Analysis...", L"Objects > Tools"));
	search (L"Get real value in bin");
	check (find (L"Get real value in bin...", L"Objects > Spectrum"));
	click (window, L"Available now");
	check (! find (L"Get real value in bin...", L"Objects > Spectrum"));
	const auto before = theCurrentPraatObjects -> n;
	praat_new (Spectrum_create (8000.0, 128), U"scope_test");
	praat_updateSelection ();
	check (find (L"Get real value in bin...", L"Objects > Spectrum"));
	click (window, L"Object commands");
	check (! find (L"Get real value in bin...", L"Objects > Spectrum"));
	click (window, L"Available now");
	praat_removeObject (before + 1);
	praat_new (Sound_createSimple (1, 0.1, 16000), U"scope_test");
	praat_updateSelection ();
	check (praat_doAction (U"View & Edit", U"", nullptr));
	search (L"Zoom in");
	check (find (L"Zoom in", L"SoundEditor"));
	click (window, L"Open editors");
	check (! find (L"Zoom in", L"SoundEditor"));
	search (L"Erase all");
	check (find (L"Erase all", L"Picture > Edit"));
	click (window, L"Picture menus");
	check (SendMessageW (list, LB_GETCOUNT, 0, 0) == 0 && ! IsWindowEnabled (run));
	click (window, L"Objects menus");
	click (window, L"Object commands");
	search (L"Batch Analysis");
	check (find (L"Batch Analysis...", L"Objects > Tools"));
	search (L"Get real value in bin");
	check (find (L"Get real value in bin...", L"Objects > Spectrum"));
	click (window, L"Open editors");
	click (window, L"Picture menus");
	praat_removeObject (before + 1);
	click (window, L"Close");
}

static const fs::path *templatePickerPath;
static const wchar_t *templatePickerTitle;
static BOOL CALLBACK findFileNameField (HWND child, LPARAM data) {
	wchar_t className [32] {};
	GetClassNameW (child, className, 32);
	const auto id = GetDlgCtrlID (child);
	if (_wcsicmp (className, L"Edit") != 0 || (id != 1148 && id != 1001)) return TRUE;
	*reinterpret_cast<HWND *> (data) = child;
	return FALSE;
}
static BOOL CALLBACK handleTemplatePicker (HWND window, LPARAM) {
	wchar_t title [64] {};
	GetWindowTextW (window, title, 64);
	if (wcscmp (title, templatePickerTitle) != 0) return TRUE;
	HWND edit = nullptr;
	EnumChildWindows (window, findFileNameField, (LPARAM) & edit);
	if (! edit) return TRUE;
	SendMessageW (edit, EM_SETSEL, 0, -1);
	SendMessageW (edit, EM_REPLACESEL, TRUE, (LPARAM) templatePickerPath -> c_str());
	PostMessageW (GetDlgItem (window, IDOK), BM_CLICK, 0, 0);
	return FALSE;
}
static void CALLBACK provideTemplatePath (HWND, UINT, UINT_PTR timer, DWORD) {
	if (! EnumThreadWindows (GetCurrentThreadId (), handleTemplatePicker, 0)) KillTimer (nullptr, timer);
}
static void chooseTemplateFile (HWND owner, const wchar_t *button, const wchar_t *dialog, const fs::path &path) {
	templatePickerPath = & path;
	templatePickerTitle = dialog;
	const auto timer = SetTimer (nullptr, 0, 60, provideTemplatePath);
	check (timer);
	click (owner, button);
	KillTimer (nullptr, timer);
}

static void testCorpusAnnotations (const fs::path &root) {
	using namespace CorpusAnnotations;
	const auto layout = parseTemplate (U"interval: Words and phrases\n  =æ\n  =Élan\npoint: Events\n  =breath\n");
	const auto path = root / fs::u8path ("Template π.PraatAnnotationTemplate");
	writeTemplate (path, layout, false);
	check (templateBody (readTemplate (path)) == templateBody (layout));
	bool rejected = false;
	try { parseTemplate (U"interval: Words\npoint: Words\n"); }
	catch (MelderError) { Melder_clearError (); rejected = true; }
	check (rejected);
	auto grid = createGrid (layout, 0.0, 1.0);
	check (grid -> tiers -> size == 2 && grid -> xmin == 0.0 && grid -> xmax == 1.0);
	check (validateGrid (grid.get(), layout).empty());
	TextGrid_insertBoundary (grid.get(), 1, 0.3);
	TextGrid_setIntervalText (grid.get(), 1, 1, U"Élan");
	TextGrid_setIntervalText (grid.get(), 1, 2, U"free label");
	TextGrid_insertPoint (grid.get(), 2, 0.6, U"Élan point");
	check (validateGrid (grid.get(), layout).size() == 2);
	check (fromGrid (grid.get(), true) [0].labels.size() == 2);
	check (fromGrid (grid.get(), false) [1].labels.empty());
	const auto directory = root / "annotation-corpus";
	fs::create_directory (directory);
	auto destination = fileFor (directory / "example.TextGrid");
	Data_writeToTextFile (grid.get(), & destination);
	fs::copy_file (root / "corpus" / "a" / "Take.WAV", directory / "EXAMPLE.wav");
	fs::copy_file (directory / "example.TextGrid", directory / "orphan.TextGrid");
	std::ofstream (directory / "unreadable.TextGrid") << "bad TextGrid";
	const auto all = search (directory, U"élan", U"", false, false);
	check (all.gridsSearched == 2 && all.errors.size() == 1 && all.hits.size() == 4);
	check (all.hits [0].recording == "EXAMPLE.wav" && all.hits [2].recording.empty());
	check (all.hits [1].start == 0.6 && all.hits [1].end == 0.6 && stillMatches (grid.get(), all.hits [1]));
	check (search (directory, U"Élan", U"Words and phrases", true, true).hits.size() == 2);
	check (search (directory, U"élan", U"", true, true).hits.empty());
	TextGrid_setPointText (grid.get(), 2, 1, U"breath");
	check (! stillMatches (grid.get(), all.hits [1]));

	check (praat_doMenuCommand (U"Search TextGrids...", U"", nullptr));
	const auto window = topWindow (L"Search TextGrids");
	check (window);
	chooseFolder (window, L"Choose folder…", directory);
	SetWindowTextW (control (window, L"Edit"), L"Élan");
	click (window, L"Search");
	const auto rows = control (window, L"ListBox");
	check (SendMessageW (rows, LB_GETCOUNT, 0, 0) == 4);
	const auto before = theCurrentPraatObjects -> n;
	click (window, L"Open match");
	check (theCurrentPraatObjects -> n == before + 2);
	const auto editor = static_cast<TextGridEditor> (theCurrentPraatObjects -> list [before + 2].editors [0]);
	check (editor && editor -> workspaceSoundId == theCurrentPraatObjects -> list [before + 1].id);
	check (editor -> textGridArea() -> selectedTier == 1 && editor -> startSelection == 0.0 && editor -> endSelection == 0.3);
	SendMessageW (rows, LB_SETCURSEL, 1, 0);
	click (window, L"Open match");
	check (theCurrentPraatObjects -> n == before + 2 && editor -> textGridArea() -> selectedTier == 2);
	check (editor -> startSelection == 0.6 && editor -> endSelection == 0.6);
	click (window, L"Close");

	praat_deselectAll ();
	praat_select (before + 1);
	check (praat_doMenuCommand (U"Annotation Templates...", U"", nullptr));
	const auto templates = topWindow (L"Annotation Templates");
	check (templates);
	chooseTemplateFile (templates, L"Load template…", L"Load annotation template", path);
	const auto copiedTemplate = root / "Shared.PraatAnnotationTemplate";
	chooseTemplateFile (templates, L"Save template as…", L"Save annotation template", copiedTemplate);
	check (templateBody (readTemplate (copiedTemplate)) == templateBody (layout));
	click (templates, L"Create TextGrid for recording");
	check (theCurrentPraatObjects -> n == before + 3);
	const auto created = static_cast<TextGridEditor> (theCurrentPraatObjects -> list [before + 3].editors [0]);
	check (created && created -> textGrid() -> tiers -> size == 2 && created -> annotationTemplate.size() == 2);
	EditorCommand allowed = nullptr;
	for (integer i = 1; i <= created -> menus.size; ++ i) {
		const auto menu = created -> menus.at [i];
		if (str32equ (menu -> menuTitle.get(), U"Labels"))
			for (integer j = 1; j <= menu -> commands.size; ++ j)
				if (str32equ (menu -> commands.at [j] -> itemTitle.get(), U"Choose allowed label..."))
					allowed = menu -> commands.at [j];
	}
	check (allowed);
	Editor_activateSearchCommand (allowed);
	const auto chooser = topWindow (L"Choose allowed label");
	check (chooser);
	click (chooser, L"OK");
	check (str32equ (created -> textGrid() -> intervalTier_cast (1) -> intervals.at [1] -> text.get(), U"æ"));
	TextGrid_insertPoint (created -> textGrid(), 2, 0.3, U"");
	created -> textGridArea() -> selectedTier = 2;
	created -> startSelection = created -> endSelection = 0.3;
	Editor_activateSearchCommand (allowed);
	click (topWindow (L"Choose allowed label"), L"OK");
	check (str32equ (created -> textGrid() -> textTier_cast (2) -> points.at [1] -> mark.get(), U"breath"));
	click (templates, L"Close");
}

int main (int argc, char **argv) {
	try {
		praat_init (U"PraatBatchTest", U"1", 1, 2026, 11, 18, U"test", U"localhost", argc, argv);
		praat_uvafon_init ();
		const auto root = fs::u8path (Melder_peek32to8 (MelderFolder_peekPath (Melder_preferencesFolder())));
		testAnalysis (root);
		testWindow (root);
		testSpectrumHoverHelp ();
		testSearchScope ();
		testLiveBinCount ();
		testSelectAll ();
		testCorpusAnnotations (root);
		std::cout << "Batch analysis, TextGrid search and annotation template integration passed" << std::endl;
		Gui_runQuitApplicationCallback ();
		std::abort ();
	} catch (MelderError) {
		std::cerr << Melder_peek32to8 (Melder_getError ()) << std::endl;
		return 1;
	}
}
