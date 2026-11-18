// Integration tests use a private --pref-dir and only their own windows/data.
#include "../sys/praatP.h"
#include "../sys/praat_session.h"
#include "../sys/NotebookEditor.h"
#include "../sys/GuiP.h"
#include "../fon/Sound_and_Spectrum.h"
#include "../fon/TextGrid.h"
#include "../fon/LongSound.h"
#include "../foned/TextGridEditor.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#undef assert
#define assert(condition) do { \
	if (! (condition)) { \
		std::cerr << "Failed at line " << __LINE__ << ": " << #condition << std::endl; \
		std::_Exit (1); \
	} \
} while (false)

extern void praat_uvafon_init ();
namespace fs = std::filesystem;
static std::string mode;
static fs::path root;
static int reportedErrors = 0;
static void recordError (conststring32 text) {
	assert (str32str (text, U"Praat could not close safely") || str32str (text, U"Praat started, but could not restore") ||
		str32str (text, U"Periodic autosave failed") || str32str (text, U"Could not restore the selected snapshot") ||
		str32str (text, U"This file cannot replace the missing recording"));
	++ reportedErrors;
}
Thing_define (SessionUnsupported, Daata) {
	bool v_writable () override { return false; }
};
Thing_implement (SessionUnsupported, Daata, 0);
Thing_define (SessionWriteFailure, Daata) {
	void v1_writeBinary (FILE *) override { Melder_throw (U"Simulated disk write failure."); }
};
Thing_implement (SessionWriteFailure, Daata, 0);

static structMelderFile fileFor (const fs::path &path) {
	structMelderFile file { };
	Melder_pathToFile (Melder_peek8to32_u (path.u8string().c_str()), & file);
	return file;
}
static void clear () {
	while (theReferencesToAllOpenScriptEditors.size) {
		ScriptEditor editor = theReferencesToAllOpenScriptEditors.at [1];
		forget (editor);
	}
	while (theReferencesToAllOpenNotebookEditors.size) {
		NotebookEditor editor = theReferencesToAllOpenNotebookEditors.at [1];
		forget (editor);
	}
	while (theCurrentPraatObjects -> n) praat_removeObject (theCurrentPraatObjects -> n);
	theCurrentPraatObjects -> sequentialUniqueIdOfLatestObjectInList = 0;
}
static size_t savedCount () {
	size_t count = 0;
	if (! fs::exists (root / "SessionBackups")) return count;
	for (const auto &entry : fs::directory_iterator (root / "SessionBackups"))
		if (entry.path().extension() == ".Collection" && entry.path().filename() != "session-not-a-backup.Collection") ++ count;
	return count;
}
static std::vector<fs::path> filesWithSuffix (const fs::path &folder, const char *suffix) {
	std::vector<fs::path> paths;
	if (fs::exists (folder))
		for (const auto &entry : fs::directory_iterator (folder))
			if (entry.is_regular_file() && entry.path().extension() == suffix) paths.push_back (entry.path());
	std::sort (paths.begin(), paths.end());
	return paths;
}

static void recoveryStorage () {
	const auto preferences = Melder_preferencesFolder ();
	const auto directory = root / "SessionBackups", shared = directory / "Sounds";
	assert (praat_session_diskLimitMiB () == 2048);
	fs::create_directory (directory);
	// A legacy embedded snapshot should continue to restore independently of Sounds/.
	auto legacy = Collection_create ();
	auto addStrings = [&] (std::initializer_list<conststring32> fields) {
		auto strings = Thing_new (Strings);
		strings -> strings = autoSTRVEC ((integer) fields.size());
		for (const auto value : fields) strings -> strings [++ strings -> numberOfStrings] = Melder_dup (value);
		legacy -> addItem_move (strings.move());
	};
	addStrings ({ U"PraatSession 2", U"1" });
	addStrings ({ U"legacy", U"Sound", U"", U"1", U"1", U"1" });
	auto payloads = Collection_create ();
	payloads -> addItem_move (Sound_createSimple (1, 0.01, 16000));
	legacy -> addItem_move (payloads.move());
	addStrings ({});
	addStrings ({});
	const auto legacyPath = directory / "session-1000000000000000-1-1.Collection";
	auto old = fileFor (legacyPath);
	Data_writeToBinaryFile (legacy.get(), & old);
	assert (praat_session_restoreFromFolder (preferences));
	assert (str32equ (theCurrentPraatObjects -> list [1].object -> name.get(), U"legacy"));
	clear ();
	fs::remove (legacyPath);

	auto initial = Sound_createSimple (1, 0.625, 128000);
	initial -> z [1] [1] = 0.125;
	praat_new (initial.move(), U"original");
	praat_updateSelection ();
	auto sound = static_cast<Sound> (theCurrentPraatObjects -> list [1].object);
	const auto interrupted = directory / "pending-recovery-1234567-123";
	fs::create_directory (interrupted);
	std::ofstream (interrupted / "snapshot.Collection") << "incomplete";
	std::ofstream (interrupted / (std::string (64, 'a') + ".Sound")) << "partial audio";
	praat_session_saveToFolder (preferences);
	assert (! fs::exists (interrupted));
	Thing_setName (sound, U"renamed");
	praat_session_saveToFolder (preferences);
	const auto originalBlobs = filesWithSuffix (shared, ".Sound");
	assert (savedCount () == 2 && originalBlobs.size() == 1 && fs::file_size (originalBlobs [0]) > 600000);
	for (const auto &path : filesWithSuffix (directory, ".Collection")) assert (fs::file_size (path) < 4096);
	sound -> z [1] [1] = 0.25;
	praat_new (Thing_new (SessionWriteFailure), U"cannot_save");
	bool failed = false;
	try { praat_session_saveToFolder (preferences); } catch (MelderError) { failed = true; Melder_clearError (); }
	assert (failed && savedCount () == 2 && filesWithSuffix (shared, ".Sound") == originalBlobs);
	praat_removeObject (2);
	praat_session_saveToFolder (preferences);
	assert (savedCount () == 3 && filesWithSuffix (shared, ".Sound").size() == 2);
	const auto removed = directory / "Removed";
	fs::create_directory (removed);
	const auto latest = filesWithSuffix (directory, ".Collection").back();
	fs::rename (latest, removed / latest.filename());
	clear ();
	assert (praat_session_restoreFromFolder (preferences));
	sound = static_cast<Sound> (theCurrentPraatObjects -> list [1].object);
	assert (sound -> z [1] [1] == 0.125 && str32equ (sound -> name.get(), U"renamed"));
	// A removed snapshot still keeps its unique audio until space is needed.
	praat_session_saveToFolder (preferences);
	assert (filesWithSuffix (shared, ".Sound").size() == 2);
	assert (praat_doMenuCommand (U"Session saving settings...", U"yes yes 5 200 1", nullptr));
	assert (praat_session_diskLimitMiB () == 1 && fs::exists (removed / latest.filename()));
	praat_session_saveToFolder (preferences);
	assert (filesWithSuffix (removed, ".Collection").empty() && savedCount () == 4);
	assert (filesWithSuffix (shared, ".Sound") == originalBlobs);
	sound -> z [1] [1] = 0.5;
	praat_session_saveToFolder (preferences);
	assert (savedCount () == 1 && filesWithSuffix (shared, ".Sound").size() == 1 && ! fs::exists (originalBlobs [0]));
	clear ();
	assert (praat_session_restoreFromFolder (preferences));
	assert (static_cast<Sound> (theCurrentPraatObjects -> list [1].object) -> z [1] [1] == 0.5);
	clear ();
	praat_new (Sound_createSimple (1, 1.25, 128000), U"larger_than_limit");
	praat_updateSelection ();
	praat_session_saveToFolder (preferences);
	assert (savedCount () == 1 && fs::file_size (filesWithSuffix (shared, ".Sound").at (0)) > 1024 * 1024);
	clear ();
	assert (praat_session_restoreFromFolder (preferences));
	assert (str32equ (theCurrentPraatObjects -> list [1].object -> name.get(), U"larger_than_limit"));
	clear ();
	std::cout << "Shared Sound storage, earlier audio versions, failure handling and disk limit passed" << std::endl;
}
static void populate () {
	theCurrentPraatObjects -> sequentialUniqueIdOfLatestObjectInList = 100;
	autoSound sound = Sound_createSimple (2, 0.125, 16000.0);
	for (integer i = 1; i <= sound -> nx; ++ i) {
		sound -> z [1] [i] = 0.1 * sin (i * 0.1);
		sound -> z [2] [i] = -0.25;
	}
	auto wav = fileFor (root / fs::u8path ("source å.wav"));
	Sound_saveAsAudioFile (sound.get(), & wav, Melder_WAV, 16);
	autoSpectrum spectrum = Sound_to_Spectrum (sound.get(), true);
	praat_new (sound.move(), U"声_å");
	praat_new (spectrum.move(), U"spectrum");
	praat_new (TextGrid_create (0.0, 0.125, U"words", U""), U"annotations");
	praat_newWithFile (LongSound_open (& wav), & wav, U"long_audio");
	praat_updateSelection ();
	praat_deselectAll ();
	praat_select (1); praat_select (3);
	auto original = fileFor (root / "original.praat");
	MelderFile_writeText_i (& original, U"# original unchanged\n", kMelder_textOutputEncoding::UTF8);
	autoScriptEditor script = ScriptEditor_createFromText (nullptr, U"# unsaved α draft\nwriteInfoLine: \"do not run\"\n");
	MelderFile_copy (& original, & script -> file);
	script -> dirty = true;
	script.releaseToUser ();
	autoNotebookEditor notebook = NotebookEditor_createFromText (U"\"Draft notebook\"\nSome unsaved text.\n");
	notebook -> dirty = true;
	notebook.releaseToUser ();
}
static void check () {
	assert (theCurrentPraatObjects -> n == 4);
	assert (theCurrentPraatObjects -> list [1].klas == classSound);
	auto sound = static_cast<Sound> (theCurrentPraatObjects -> list [1].object);
	assert (sound -> ny == 2 && sound -> nx == 2000 && sound -> z [2] [10] == -0.25);
	assert (str32equ (sound -> name.get(), U"声_å"));
	assert (theCurrentPraatObjects -> list [2].klas == classSpectrum);
	assert (theCurrentPraatObjects -> list [3].klas == classTextGrid);
	assert (theCurrentPraatObjects -> list [4].klas == classLongSound);
	for (integer i = 1; i <= 4; ++ i) {
		assert (theCurrentPraatObjects -> list [i].id == i + 100);
		assert (theCurrentPraatObjects -> list [i].isSelected == (i == 1 || i == 3));
	}
	assert (theCurrentPraatObjects -> sequentialUniqueIdOfLatestObjectInList >= 104);
	assert (theCurrentPraatObjects -> totalSelection == 2);
	assert (theReferencesToAllOpenScriptEditors.size == 1 && ScriptEditors_dirty ());
	assert (theReferencesToAllOpenNotebookEditors.size == 1 && NotebookEditors_dirty ());
	autostring32 text = GuiText_getString (theReferencesToAllOpenScriptEditors.at [1] -> textWidget);
	assert (str32str (text.get(), U"unsaved α draft"));
	auto original = fileFor (root / "original.praat");
	text = MelderFile_readText (& original);
	assert (str32str (text.get(), U"original unchanged") && ! str32str (text.get(), U"do not run"));
}
static void openAndPositionEditor () {
	praat_show ();
	assert (praat_doAction (U"View & Edit", U"", nullptr));
	auto editor = static_cast<TextGridEditor> (theCurrentPraatObjects -> list [3].editors [0]);
	assert (editor && editor -> classInfo == classTextGridEditor && editor -> soundArea());
	editor -> startWindow = 0.02;
	editor -> endWindow = 0.11;
	editor -> startSelection = 0.04;
	editor -> endSelection = 0.06;
	FunctionEditor_windowMarksChanged (editor, true);
	SetWindowPos (editor -> windowForm -> d_xmShell -> window, nullptr, 120, 100, 620, 450,
		SWP_NOACTIVATE | SWP_NOZORDER);
	GuiText_setSelection (theReferencesToAllOpenScriptEditors.at [1] -> textWidget, 3, 11);
	autoScriptEditor clean = ScriptEditor_createFromText (nullptr, U"# saved workspace script");
	clean -> dirty = false;
	clean.releaseToUser ();
}
static void checkEditorRestoration () {
	auto editor = static_cast<TextGridEditor> (theCurrentPraatObjects -> list [3].editors [0]);
	assert (editor && editor -> classInfo == classTextGridEditor && editor -> soundArea());
	assert (editor -> workspaceSoundId == theCurrentPraatObjects -> list [1].id);
	assert (editor -> startWindow == 0.02 && editor -> endWindow == 0.11);
	assert (editor -> startSelection == 0.04 && editor -> endSelection == 0.06);
	RECT bounds {};
	assert (GetWindowRect (editor -> windowForm -> d_xmShell -> window, & bounds));
	assert (bounds.left == 120 && bounds.top == 100 && bounds.right - bounds.left == 620 && bounds.bottom - bounds.top == 450);
	assert (theReferencesToAllOpenScriptEditors.size == 2);
	integer first, last;
	(void) GuiText_getStringAndSelectionPosition (theReferencesToAllOpenScriptEditors.at [1] -> textWidget, & first, & last);
	assert (first == 3 && last == 11);
	assert (! theReferencesToAllOpenScriptEditors.at [2] -> dirty);
	ScriptEditor clean = theReferencesToAllOpenScriptEditors.at [2];
	forget (clean);
}
struct RecoveryControl {
	const wchar_t *className;
	const wchar_t *title;
	HWND window = nullptr;
};
static BOOL CALLBACK findRecoveryChild (HWND child, LPARAM closure) {
	auto &search = *reinterpret_cast<RecoveryControl *> (closure);
	wchar_t className [32] {}, title [64] {};
	GetClassNameW (child, className, 32);
	GetWindowTextW (child, title, 64);
	if ((search.className && _wcsicmp (className, search.className) != 0) ||
			(search.title && wcscmp (title, search.title) != 0))
		return TRUE;
	search.window = child;
	return FALSE;
}
static HWND findRecoveryControl (const wchar_t *className, const wchar_t *title = nullptr) {
	RecoveryControl dialog { nullptr, L"Recover workspace" };
	EnumThreadWindows (GetCurrentThreadId (), findRecoveryChild, (LPARAM) & dialog);
	assert (dialog.window && IsWindowVisible (dialog.window));
	RecoveryControl search { className, title };
	EnumChildWindows (dialog.window, findRecoveryChild, (LPARAM) & search);
	assert (search.window);
	return search.window;
}
static void chooseRecovery (int row) {
	const auto list = findRecoveryControl (L"ListBox");
	SendMessageW (list, LB_SETSEL, FALSE, -1);
	assert (SendMessageW (list, LB_SETSEL, TRUE, row) != LB_ERR);
	SendMessageW (GetParent (list), WM_COMMAND, MAKEWPARAM (GetDlgCtrlID (list), LBN_SELCHANGE), (LPARAM) list);
	SendMessage (findRecoveryControl (L"Button", L"Restore selected"), BM_CLICK, 0, 0);
}
static void checkRecoveryHistory () {
	const auto folder = root / "SessionBackups";
	const auto removed = folder / "Removed";
	const auto list = findRecoveryControl (L"ListBox");
	const auto restore = findRecoveryControl (L"Button", L"Restore selected");
	const auto count = [&] { return SendMessageW (list, LB_GETCOUNT, 0, 0); };
	auto selectRows = [&] (bool multiple) {
		assert (SendMessageW (list, LB_SETSEL, TRUE, multiple ? -1 : 0) != LB_ERR);
		SendMessageW (GetParent (list), WM_COMMAND, MAKEWPARAM (GetDlgCtrlID (list), LBN_SELCHANGE), (LPARAM) list);
	};
	assert (count () == 5 && IsWindowEnabled (restore));
	wchar_t detail [512] {};
	GetWindowTextW (findRecoveryControl (L"Edit"), detail, 512);
	assert (wcsstr (detail, L"Sound") && wcsstr (detail, L"tone"));
	selectRows (true);
	assert (! IsWindowEnabled (restore));
	SendMessageW (findRecoveryControl (L"Button", L"Remove selected"), BM_CLICK, 0, 0);
	assert (count () == 0 && savedCount () == 0 && std::distance (fs::directory_iterator (removed), fs::directory_iterator ()) == 5);
	SendMessageW (findRecoveryControl (L"Button", L"Start empty"), BM_CLICK, 0, 0);
	assert (praat_doMenuCommand (U"Recover workspace...", U"", nullptr));
	assert (count () == 0);
	SendMessageW (findRecoveryControl (L"Button", L"Recently removed (5)"), BM_CLICK, 0, 0);
	assert (count () == 5 && ! IsWindowEnabled (restore));
	selectRows (true);
	SendMessageW (findRecoveryControl (L"Button", L"Return to recovery list"), BM_CLICK, 0, 0);
	assert (count () == 0 && savedCount () == 5);
	SendMessageW (findRecoveryControl (L"Button", L"Back to recovery"), BM_CLICK, 0, 0);
	assert (count () == 5 && IsWindowEnabled (restore));
	SendMessageW (list, LB_SETSEL, FALSE, -1);
	SendMessageW (list, LB_SETSEL, TRUE, 4);
	SendMessageW (GetParent (list), WM_COMMAND, MAKEWPARAM (GetDlgCtrlID (list), LBN_SELCHANGE), (LPARAM) list);
	GetWindowTextW (findRecoveryControl (L"Edit"), detail, 512);
	assert (wcscmp (detail, L"Empty workspace") == 0);
	SendMessageW (findRecoveryControl (L"Button", L"Cancel"), BM_CLICK, 0, 0);
	std::cout << "Recovery previews, bulk removal and undo passed" << std::endl;
}
static HWND windowWithTitle (const wchar_t *title) {
	RecoveryControl query { nullptr, title };
	EnumThreadWindows (GetCurrentThreadId (), findRecoveryChild, (LPARAM) & query);
	assert (query.window && IsWindowVisible (query.window));
	return query.window;
}
static HWND namedButton (HWND window, const wchar_t *title) {
	RecoveryControl query { L"Button", title };
	EnumChildWindows (window, findRecoveryChild, (LPARAM) & query);
	assert (query.window);
	return query.window;
}
static BOOL CALLBACK findMacroText (HWND child, LPARAM result) {
	wchar_t className [32] {};
	GetClassNameW (child, className, 32);
	if (_wcsicmp (className, L"Edit") == 0)
		reinterpret_cast<HWND *> (result) [(GetWindowLongW (child, GWL_STYLE) & WS_VSCROLL) != 0] = child;
	return TRUE;
}
static void CALLBACK confirmClearMacro (HWND, UINT, UINT_PTR timer, DWORD) {
	RecoveryControl query { L"#32770", L"Clear macro slot" };
	EnumThreadWindows (GetCurrentThreadId (), findRecoveryChild, (LPARAM) & query);
	if (! query.window) return;
	KillTimer (nullptr, timer);
	PostMessageW (query.window, WM_COMMAND, IDOK, 0);
}
static void macroPad () {
	const auto objects = windowWithTitle (L"PraatSessionTest Objects");
	if (mode == "write-macros") {
		for (int i = 1; i <= 9; ++ i)
			namedButton (objects, (std::to_wstring (i) + L" +").c_str());
		SendMessageW (namedButton (objects, L"1 +"), WM_RBUTTONUP, 0, 0);
		const auto edit = windowWithTitle (L"Edit macro slot 1");
		HWND fields [2] {};
		EnumChildWindows (edit, findMacroText, (LPARAM) fields);
		assert (fields [0] && fields [1]);
		SetWindowTextW (fields [0], L"Label");
		SetWindowTextW (fields [1], L"# Selected object only\r\nRename: \"měřené\"\r\n");
		SendMessageW (namedButton (edit, L"Save"), BM_CLICK, 0, 0);
		assert (fs::exists (root / "Macros" / "slot-1.PraatMacro") && ! IsWindowVisible (edit));
		const auto empty = namedButton (objects, L"2 +");
		SendMessageW (empty, BM_CLICK, 0, 0);
		assert (windowWithTitle (L"Edit macro slot 2") == edit);
		SetWindowTextW (fields [0], L"Discard");
		SendMessageW (namedButton (edit, L"Cancel"), BM_CLICK, 0, 0);
		assert (! fs::exists (root / "Macros" / "slot-2.PraatMacro"));
	} else {
		const auto macro = namedButton (objects, L"Label");
		praat_new (Sound_createSimple (1, 0.1, 16000), U"unchanged");
		praat_updateSelection ();
		praat_new (Sound_createSimple (1, 0.1, 16000), U"selected");
		praat_updateSelection ();
		SendMessageW (macro, BM_CLICK, 0, 0);
		assert (theCurrentPraatObjects -> n == 2);
		assert (str32equ (theCurrentPraatObjects -> list [1].object -> name.get(), U"unchanged"));
		assert (str32equ (theCurrentPraatObjects -> list [2].object -> name.get(), U"měřené"));
		SendMessageW (macro, WM_CONTEXTMENU, (WPARAM) macro, -1);
		const auto edit = windowWithTitle (L"Edit macro slot 1");
		const auto timer = SetTimer (nullptr, 0, 60, confirmClearMacro);
		assert (timer);
		SendMessageW (namedButton (edit, L"Clear slot…"), BM_CLICK, 0, 0);
		KillTimer (nullptr, timer);
		namedButton (objects, L"1 +");
		assert (! fs::exists (root / "Macros" / "slot-1.PraatMacro"));
		clear ();
	}
	std::cout << "Macro pad verified in mode " << mode << std::endl;
}
static BOOL CALLBACK findRecordingPicker (HWND window, LPARAM result) {
	wchar_t caption [256] {};
	GetWindowTextW (window, caption, 256);
	if (wcsncmp (caption, L"Locate recording: ", 18) != 0 || ! IsWindowEnabled (window)) return TRUE;
	*reinterpret_cast<HWND *> (result) = window;
	return FALSE;
}
static BOOL CALLBACK findRecordingEdit (HWND child, LPARAM result) {
	wchar_t className [32] {};
	GetClassNameW (child, className, 32);
	if (_wcsicmp (className, L"Edit") != 0 || GetDlgCtrlID (child) != 1148) return TRUE;
	*reinterpret_cast<HWND *> (result) = child;
	return FALSE;
}
static int pickerSubmissions = 0;
static void CALLBACK supplyRecording (HWND, UINT, UINT_PTR timer, DWORD) {
	HWND picker = nullptr;
	EnumThreadWindows (GetCurrentThreadId (), findRecordingPicker, (LPARAM) & picker);
	if (! picker) return;
	if (mode == "cancel-relink") {
		KillTimer (nullptr, timer);
		PostMessage (picker, WM_COMMAND, IDCANCEL, 0);
		return;
	}
	HWND edit = nullptr;
	EnumChildWindows (picker, findRecordingEdit, (LPARAM) & edit);
	if (! edit) return;
	const auto chosen = fs::canonical (root / "relocated.wav");
	SendMessageW (edit, EM_SETSEL, 0, -1);
	SendMessageW (edit, EM_REPLACESEL, TRUE, (LPARAM) chosen.c_str());
	wchar_t selected [MAX_PATH + 1] {};
	GetWindowTextW (edit, selected, MAX_PATH + 1);
	assert (chosen == selected);
	++ pickerSubmissions;
	KillTimer (nullptr, timer);
	PostMessage (GetDlgItem (picker, IDOK), BM_CLICK, 0, 0);
}
static void core () {
	assert (praat_session_isEnabled () && praat_session_autosaveEnabled ());
	assert (praat_session_autosaveMinutes () == 5 && praat_session_backupsToKeep () == 200);
	std::cout << "core: populate" << std::endl;
	populate ();
	openAndPositionEditor ();
	auto expectedSound = Data_copy (theCurrentPraatObjects -> list [1].object);
	auto expectedSpectrum = Data_copy (theCurrentPraatObjects -> list [2].object);
	praat_session_saveToFolder (Melder_preferencesFolder());
	assert (savedCount () == 1);
	checkEditorRestoration ();
	check ();
	clear ();
	assert (praat_session_restoreFromFolder (Melder_preferencesFolder()));
	checkEditorRestoration ();
	check ();
	assert (Data_equal (expectedSound.get(), theCurrentPraatObjects -> list [1].object));
	assert (Data_equal (expectedSpectrum.get(), theCurrentPraatObjects -> list [2].object));
	std::cout << "core: restored data" << std::endl;
	// Unsupported objects and I/O errors leave all work and the last backup untouched.
	praat_new (Thing_new (SessionUnsupported), U"unsupported");
	praat_updateSelection ();
	bool failed = false;
	try { praat_session_saveToFolder (Melder_preferencesFolder()); } catch (MelderError) { failed = true; Melder_clearError (); }
	assert (failed && theCurrentPraatObjects -> n == 5 && savedCount () == 1);
	// The real window-close callback catches the error and keeps the application alive.
	Melder_setErrorProc (recordError);
	Gui_runQuitApplicationCallback ();
	assert (reportedErrors == 1 && theCurrentPraatObjects -> n == 5 && savedCount () == 1);
	std::cout << "core: cancelled unsafe quit" << std::endl;
	praat_removeObject (5);
	praat_new (Thing_new (SessionWriteFailure), U"write_failure");
	praat_updateSelection ();
	failed = false;
	try { praat_session_saveToFolder (Melder_preferencesFolder()); } catch (MelderError) { failed = true; Melder_clearError (); }
	assert (failed && savedCount () == 1 && theCurrentPraatObjects -> n == 5);
	for (const auto &entry : fs::directory_iterator (root / "SessionBackups"))
		assert (entry.path().filename().u8string().rfind ("pending-", 0) != 0);
	praat_removeObject (5);
	fs::path blocked = root / "blocked";
	std::cout << "core: partial write cleanup" << std::endl;
	std::ofstream (blocked) << "not a directory";
	structMelderFolder blockedFolder { }; Melder_pathToFolder (Melder_peek8to32_u (blocked.u8string().c_str()), & blockedFolder);
	failed = false;
	try { praat_session_saveToFolder (& blockedFolder); } catch (MelderError) { failed = true; Melder_clearError (); }
	assert (failed && theCurrentPraatObjects -> n == 4 && savedCount () == 1);
	std::cout << "core: unwritable path" << std::endl;
	// A missing referenced LongSound must not produce a silently incomplete backup/restore.
	const auto source = root / fs::u8path ("source å.wav");
	auto longSound = static_cast<LongSound> (theCurrentPraatObjects -> list [4].object);
	auto sourceFile = fileFor (source);
	auto missingFile = fileFor (root / "missing.wav");
	MelderFile_copy (& missingFile, & longSound -> file);
	failed = false;
	try { praat_session_saveToFolder (Melder_preferencesFolder()); } catch (MelderError) { failed = true; Melder_clearError (); }
	assert (failed && savedCount () == 1);
	MelderFile_copy (& sourceFile, & longSound -> file);
	clear ();
	fs::rename (source, root / "moved.wav");
	failed = false;
	try { praat_session_restoreFromFolder (Melder_preferencesFolder()); } catch (MelderError) { failed = true; Melder_clearError (); }
	assert (failed && theCurrentPraatObjects -> n == 0 && theReferencesToAllOpenScriptEditors.size == 0);
	fs::rename (root / "moved.wav", source);
	std::cout << "core: missing reference" << std::endl;
	// Pending and corrupt files are ignored/fall back without duplicate Objects.
	fs::create_directory (root / "SessionBackups" / "pending-incomplete");
	std::ofstream (root / "SessionBackups" / "session-9999999999999999-1-1.Collection") << "truncated";
	assert (praat_session_restoreFromFolder (Melder_preferencesFolder()));
	checkEditorRestoration ();
	check ();
	std::cout << "core: fallback" << std::endl;
	// An intentional empty session must supersede old data rather than resurrecting it.
	fs::remove (root / "SessionBackups" / "session-9999999999999999-1-1.Collection");
	clear ();
	// Default retention spans 200 distinct full snapshots, including an empty state.
	const auto keep = root / "SessionBackups" / "session-not-a-backup.Collection";
	std::ofstream (keep) << "leave unrelated files alone";
	for (int i = 0; i < 201; ++ i) praat_session_saveToFolder (Melder_preferencesFolder());
	assert (savedCount () == 200 && fs::exists (keep));
	assert (fs::is_directory (root / "SessionBackups" / "pending-incomplete"));
	assert (praat_session_restoreFromFolder (Melder_preferencesFolder()));
	assert (theCurrentPraatObjects -> n == 0 && theReferencesToAllOpenScriptEditors.size == 0);
	// Smaller retention is applied only after the next successful save.
	praat_session_setAutosaveSettings (true, 2, 3);
	assert (savedCount () == 200);
	praat_new (Thing_new (SessionWriteFailure), U"retention_failure");
	failed = false;
	try { praat_session_saveToFolder (Melder_preferencesFolder()); } catch (MelderError) { failed = true; Melder_clearError (); }
	assert (failed && savedCount () == 200);
	praat_removeObject (1);
	praat_session_saveToFolder (Melder_preferencesFolder());
	assert (savedCount () == 3);
	std::cout << "core: retention" << std::endl;
	for (const auto &setting : { std::pair<integer, integer> { 0, 200 }, { 1441, 200 }, { 5, 0 }, { 5, 201 } }) {
		failed = false;
		try { praat_session_setAutosaveSettings (false, setting.first, setting.second); }
		catch (MelderError) { failed = true; Melder_clearError (); }
		assert (failed && praat_session_autosaveEnabled () && praat_session_autosaveMinutes () == 2 && praat_session_backupsToKeep () == 3);
	}
	// The automatic hooks must be inert in batch/no-pref-files and when disabled.
	const size_t before = savedCount ();
	praat_session_setEnabled (false); assert (! praat_session_isActive ()); praat_session_save ();
	praat_session_setEnabled (true); praatP.ignorePreferenceFiles = true; assert (! praat_session_isActive ()); praat_session_save ();
	praatP.ignorePreferenceFiles = false; Melder_batch = true; assert (! praat_session_isActive ()); praat_session_save ();
	assert (savedCount () == before);
	Melder_batch = false;
	assert (praat_doMenuCommand (U"Session saving settings...", U"yes no 7 123", nullptr));
	assert (praat_session_isEnabled () && ! praat_session_autosaveEnabled ());
	assert (praat_session_autosaveMinutes () == 7 && praat_session_backupsToKeep () == 123);
	std::cout << "core: settings command" << std::endl;
	auto prefs = fileFor (root / "test-preferences.txt");
	praat_session_setEnabled (false);
	praat_session_setAutosaveSettings (false, 7, 123);
	Preferences_write_i (& prefs);
	std::cout << "core: preferences written" << std::endl;
	praat_session_setEnabled (true);
	praat_session_setAutosaveSettings (true, 5, 200);
	Preferences_read_i (& prefs);
	std::cout << "core: preferences read" << std::endl;
	assert (! praat_session_isEnabled () && ! praat_session_autosaveEnabled ());
	assert (praat_session_autosaveMinutes () == 7 && praat_session_backupsToKeep () == 123);
	praat_session_setEnabled (true);
	praat_session_setAutosaveSettings (true, 5, 200);
	std::cout << "Session data, drafts, references, failure protection, fallback, empty state, retention and opt-out checks passed" << std::endl;
}
static std::chrono::steady_clock::time_point periodicStarted;
static int retryStage = 0;
static void CALLBACK observeRetry (HWND, UINT, UINT_PTR timer, DWORD) {
	const double elapsed = std::chrono::duration<double> (std::chrono::steady_clock::now() - periodicStarted).count();
	if (retryStage == 0) {
		assert (reportedErrors == 1 && savedCount () == 1);
		if (elapsed < 128.0) return;   // a second failed interval must not repeat the error dialog
		praat_removeObject (5);
		praat_deselectAll (); praat_select (1); praat_select (3); praat_show ();
		retryStage = 1;
		std::cout << "Two failed intervals kept the previous backup and reported one error" << std::endl;
	} else if (retryStage == 1) {
		if (savedCount () == 1) return;
		assert (savedCount () == 2 && reportedErrors == 1 && elapsed >= 180.0 && elapsed < 205.0);
		check ();
		praat_session_setAutosaveSettings (false, 1, 3);
		periodicStarted = std::chrono::steady_clock::now();
		retryStage = 2;
		std::cout << "Automatic retry recovered; periodic saving now disabled for a full interval" << std::endl;
	} else {
		assert (savedCount () == 2);
		if (elapsed < 62.0) return;
		KillTimer (nullptr, timer);
		std::cout << "Disabled periodic saving produced no snapshot; normal Quit still saves" << std::endl;
		Gui_runQuitApplicationCallback ();
		std::abort ();
	}
}
static void CALLBACK nestedCommand (HWND, UINT, UINT_PTR timer, DWORD) {
	KillTimer (nullptr, timer);
	assert (savedCount () == 1 && reportedErrors == 0);
	// Emulate a long command pumping messages across a one-minute deadline.
	// The native timer must only wake the main loop, never capture from here.
	const auto end = periodicStarted + std::chrono::seconds (66);
	while (std::chrono::steady_clock::now() < end) {
		MSG message;
		while (PeekMessage (& message, nullptr, 0, 0, PM_REMOVE)) XtDispatchEvent (& message);
		assert (savedCount () == 1 && reportedErrors == 0);
		Sleep (10);
	}
	std::cout << "Autosave deferred safely across a nested command message loop" << std::endl;
	assert (SetTimer (nullptr, 0, 250, observeRetry));
}
static void CALLBACK afterStartup (HWND, UINT, UINT_PTR timer, DWORD) {
	KillTimer (nullptr, timer);
	if (mode == "periodic-crash") {
		assert (theCurrentPraatObjects -> n == 0 && savedCount () == 0);
		assert (praat_session_autosaveEnabled () && praat_session_autosaveMinutes () == 5 && praat_session_backupsToKeep () == 200);
		populate ();
		periodicStarted = std::chrono::steady_clock::now();
		std::cout << "Waiting for the real default five-minute autosave deadline" << std::endl;
		// No further test timers or input: the application's own timer must wake it.
		return;
	} else if (mode == "periodic-retry") {
		populate ();
		praat_session_save ();
		praat_new (Thing_new (SessionWriteFailure), U"periodic_failure");
		praat_updateSelection ();
		Melder_setErrorProc (recordError);
		praat_session_setAutosaveSettings (true, 1, 3);
		periodicStarted = std::chrono::steady_clock::now();
		assert (SetTimer (nullptr, 0, 55000, nestedCommand));
		return;
	} else if (mode == "recovery-storage") {
		recoveryStorage ();
	} else if (mode == "write-macros" || mode == "run-macros") {
		macroPad ();
	} else if (mode == "seed-recovery-history") {
		praat_session_saveToFolder (Melder_preferencesFolder());
		praat_session_saveToFolder (Melder_preferencesFolder());
		praat_new (Sound_createSimple (1, 0.01, 16000), U"tone");
		praat_updateSelection ();
		for (integer i = 0; i < 3; ++ i) praat_session_saveToFolder (Melder_preferencesFolder());
		clear ();
	} else if (mode == "recovery-history") {
		checkRecoveryHistory ();
	} else if (mode == "seed-exit") {
		assert (theCurrentPraatObjects -> n == 0);
		populate ();
	} else if (mode == "package-large") {
		auto large = fileFor (root / "large.wav"), small = fileFor (root / "small.wav");
		praat_new (LongSound_open (& large), U"large");
		praat_new (LongSound_open (& small), U"small");
		praat_updateSelection ();
		auto package = fileFor (root / "large.zip");
		praat_project_package (& package, false);
	} else if (mode == "package-project") {
		populate ();
		openAndPositionEditor ();
		auto audio = fileFor (root / fs::u8path ("source å.wav"));
		praat_newWithFile (LongSound_open (& audio), & audio, U"same_recording");
		praat_updateSelection ();
		auto package = fileFor (root / fs::u8path ("Portable å.zip"));
		praat_project_package (& package, false);
		assert (! praat_project_save ());
		assert (theCurrentPraatObjects -> n == 5);
		bool refusedOverwrite = false;
		try { praat_project_package (& package, false); }
		catch (MelderError) { refusedOverwrite = true; Melder_clearError (); }
		assert (refusedOverwrite && fs::exists (root / fs::u8path ("Portable å.zip")));
	} else if (mode == "open-package") {
		auto project = fileFor (root / fs::u8path ("Portable å.PraatProject"));
		praat_project_open (& project, false);
		assert (theCurrentPraatObjects -> n == 5);
		assert (theCurrentPraatObjects -> list [3].editors [0]);
		auto first = static_cast<LongSound> (theCurrentPraatObjects -> list [4].object);
		auto second = static_cast<LongSound> (theCurrentPraatObjects -> list [5].object);
		assert (first -> nx == 2000 && first -> ny == 2);
		assert (str32equ (MelderFile_peekPath (& first -> file), MelderFile_peekPath (& second -> file)));
		assert (fs::equivalent (fs::u8path (Melder_peek32to8 (MelderFile_peekPath (& first -> file))),
			root / "Recordings" / "1" / fs::u8path ("source å.wav")));
		assert (theReferencesToAllOpenScriptEditors.size == 2);
		for (auto editor : theReferencesToAllOpenScriptEditors)
			assert (MelderFile_isNull (& editor -> file) && editor -> dirty);
		assert (praat_project_save ());
	} else if (mode == "reject-package-recording" || mode == "cancel-relink" || mode == "accept-relink") {
		praat_new (Sound_createSimple (1, 0.01, 16000), U"current_work");
		praat_updateSelection ();
		auto project = fileFor (root / fs::u8path ("Portable å.PraatProject"));
		if (mode != "reject-package-recording") {
			Melder_setErrorProc (recordError);
			assert (SetTimer (nullptr, 0, 150, supplyRecording));
		}
		bool rejected = false;
		try {
			autoMelderFileSetCurrentFolder currentFolder (& project);
			praat_project_open (& project, mode != "reject-package-recording");
		}
		catch (MelderError) { rejected = true; Melder_clearError (); }
		if (mode == "accept-relink") {
			assert (! rejected && theCurrentPraatObjects -> n == 5);
			assert (! reportedErrors && pickerSubmissions == 1);
			auto recording = static_cast<LongSound> (theCurrentPraatObjects -> list [4].object);
			assert (recording -> nx == 2000 && recording -> ny == 2);
			assert (praat_project_save ());
			praat_project_new ();
			praat_project_open (& project, false);
			assert (theCurrentPraatObjects -> n == 5);
			recording = static_cast<LongSound> (theCurrentPraatObjects -> list [4].object);
			assert (fs::equivalent (fs::u8path (Melder_peek32to8 (MelderFile_peekPath (& recording -> file))),
				root / "relocated.wav"));
		} else {
			assert ((mode == "reject-package-recording" ? rejected : ! rejected) && theCurrentPraatObjects -> n == 1);
			assert (str32equ (theCurrentPraatObjects -> list [1].object -> name.get(), U"current_work"));
		}
	} else if (mode == "named-project") {
		populate ();
		openAndPositionEditor ();
		const auto project = root / fs::u8path ("Annotations å.PraatProject");
		const auto projectName = Melder_8to32_e (project.u8string().c_str());
		assert (praat_doMenuCommand (U"Save project as...", projectName.get(), nullptr));
		assert (fs::exists (project));
		static_cast<Sound> (theCurrentPraatObjects -> list [1].object) -> z [2] [10] = 0.5;
		assert (praat_doMenuCommand (U"New project", U"", nullptr));
		assert (theCurrentPraatObjects -> n == 0 && savedCount () == 1);
		assert (praat_doMenuCommand (U"Open project...", projectName.get(), nullptr));
		checkEditorRestoration ();
		check ();
		static_cast<Sound> (theCurrentPraatObjects -> list [1].object) -> z [2] [10] = 0.75;
		assert (praat_doMenuCommand (U"Save project", U"", nullptr));
		assert (praat_doMenuCommand (U"New project", U"", nullptr));
		assert (praat_doMenuCommand (U"Open project...", projectName.get(), nullptr));
		assert (static_cast<Sound> (theCurrentPraatObjects -> list [1].object) -> z [2] [10] == 0.75);
		const auto damagedPath = root / "damaged.PraatProject";
		std::ofstream (damagedPath) << "not a project";
		const auto damagedName = Melder_8to32_e (damagedPath.u8string().c_str());
		bool rejected = false;
		try { praat_doMenuCommand (U"Open project...", damagedName.get(), nullptr); }
		catch (MelderError) { rejected = true; Melder_clearError (); }
		assert (rejected && theCurrentPraatObjects -> n == 4);
		assert (praat_doMenuCommand (U"Recover workspace...", U"", nullptr));
		wchar_t newestLabel [320] {};
		assert (SendMessageW (findRecoveryControl (L"ListBox"), LB_GETTEXT, 0, (LPARAM) newestLabel) != LB_ERR);
		assert (wcsstr (newestLabel, L"Annotations å"));
		chooseRecovery (1);
		assert (static_cast<Sound> (theCurrentPraatObjects -> list [1].object) -> z [2] [10] == 0.5);
		assert (! praat_project_save ());   // a recovered copy has no active project file
	} else if (mode == "concurrent-exit") {
		praat_new (Sound_createSimple (1, 0.01, 16000), U"test_audio");
		praat_updateSelection ();
	} else if (mode == "repair-exit") {
		assert (theCurrentPraatObjects -> n == 0 && reportedErrors == 0 && savedCount () == 1);
		chooseRecovery (0);
		assert (theCurrentPraatObjects -> n == 0 && reportedErrors == 1 && savedCount () == 1);
		fs::rename (root / "moved.wav", root / fs::u8path ("source å.wav"));
		chooseRecovery (0);
		check ();
		static_cast<Sound> (theCurrentPraatObjects -> list [1].object) -> z [2] [10] = 0.75;
	} else if (mode == "restore-exit" || mode == "crash-restore-exit") {
		assert (theCurrentPraatObjects -> n == 0);
		if (mode == "restore-exit") {
			// The newest snapshot has a different audio sample.
			chooseRecovery (1);
		} else
			chooseRecovery (0);
		check ();
		clear ();
	} else {
		assert ((mode == "empty-exit" || mode == "no-pref-exit") && theCurrentPraatObjects -> n == 0);
		assert (theReferencesToAllOpenScriptEditors.size == 0 && theReferencesToAllOpenNotebookEditors.size == 0);
		if (mode == "empty-exit")
			SendMessage (findRecoveryControl (L"Button", L"Start empty"), BM_CLICK, 0, 0);
		assert (theCurrentPraatObjects -> n == 0);
	}
	std::cout << "Real application startup/quit phase passed: " << mode << std::endl;
	Gui_runQuitApplicationCallback ();   // exercises the actual normal-exit hook, including dirty scripts
	std::abort ();   // successful Quit never returns
}
int main (int argc, char **argv) {
	try {
		mode = std::getenv ("PRAAT_SESSION_TEST_MODE") ? std::getenv ("PRAAT_SESSION_TEST_MODE") : "core";
		praat_init (U"PraatSessionTest", U"1", 1, 2026, 11, 16, U"test", U"localhost", argc, argv);
		praat_uvafon_init ();
		Thing_recognizeClassesByName (classSessionUnsupported, classSessionWriteFailure);
		root = fs::u8path (Melder_peek32to8 (MelderFolder_peekPath (Melder_preferencesFolder())));
		if (mode == "core") {
			core ();
			// Use Praat's supported shutdown path after creating a static settings form;
			// returning from main would run GUI destructors after their registries.
			Gui_runQuitApplicationCallback ();
			std::abort ();
		}
		else {
			if (mode == "repair-exit") Melder_setErrorProc (recordError);
			SetTimer (nullptr, 0, 250, afterStartup); praat_run ();
		}
		return 0;
	} catch (MelderError) {
		std::cerr << Melder_peek32to8 (Melder_getError ()) << std::endl;
		return 1;
	}
}
