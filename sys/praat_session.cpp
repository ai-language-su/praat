/* Session backups for this custom Praat build. GPL v3 or later. */
#include "praatP.h"
#include "praat_session.h"
#include "Strings_.h"
#include "ScriptEditor.h"
#include "NotebookEditor.h"
#include "GuiP.h"
#include "../fon/LongSound.h"
#include "../foned/SoundEditor.h"
#include "../foned/TextGridEditor.h"
#include "../foned/SpectrumEditor.h"
#include "../foned/PitchEditor.h"
#include "../foned/ManipulationEditor.h"
#include "../external/zlib/zlib.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <random>
#include <set>
#include <vector>

namespace {
namespace fs = std::filesystem;
bool saveOnExit = true;
bool autosaveEnabled = true;
integer autosaveMinutes = 5;
integer backupsToKeep = 200;
constexpr integer maximumBackups = 200;
bool autosaveStarted = false;
bool autosaveErrorReported = false;
std::chrono::steady_clock::time_point nextAutosave;
#if motif
UINT_PTR autosaveTimer = 0;
#endif
bool restoreFailed = false;
autostring32 restoreWarning;
fs::path currentProject;
constexpr integer objectFields = 6;   // name, class, source path, id, selected, payload index
constexpr integer legacyDraftFields = 3;
constexpr integer draftFields = 10;   // kind, path, text, dirty, selection (2), window bounds (4)
constexpr integer viewFields = 13;    // kind, object rows (3), zoom and selection (4), tier, window bounds (4)
constexpr integer recordingFields = 3;   // channels, samples, and sampling period

void normalizeSettings () {
	if (autosaveMinutes < 1 || autosaveMinutes > 1440) autosaveMinutes = 5;
	if (backupsToKeep < 1 || backupsToKeep > maximumBackups) backupsToKeep = maximumBackups;
}
void resetAutosaveTimer () {
	normalizeSettings ();
	nextAutosave = std::chrono::steady_clock::now() + std::chrono::minutes (autosaveMinutes);
	#if motif
		if (autosaveTimer) KillTimer (nullptr, autosaveTimer);
		autosaveTimer = 0;
		if (autosaveStarted && autosaveEnabled && praat_session_isActive ()) {
			// No callback: a timer message only wakes the OUTER event loop. Progress,
			// file choosers, playback and other nested loops cannot trigger a capture.
			// Use a short wake interval so native timer rounding cannot miss the
			// steady-clock deadline and postpone a save by a whole backup interval.
			autosaveTimer = SetTimer (nullptr, 0, 1000, nullptr);
			Melder_require (autosaveTimer, U"Could not start the session autosave timer.");
		}
	#endif
}

fs::path pathFor (conststring32 path) {
	return fs::u8path (Melder_peek32to8 (path));
}
structMelderFile fileFor (const fs::path &path) {
	structMelderFile file { };
	Melder_pathToFile (Melder_peek8to32_u (path.u8string().c_str()), & file);
	return file;
}
autoStrings strings (integer size) {
	autoStrings result = Thing_new (Strings);
	result -> strings = autoSTRVEC (size);
	result -> numberOfStrings = size;
	return result;
}
void put (Strings me, integer index, conststring32 text) {
	my strings [index] = Melder_dup (text ? text : U"");
}
void appendWindow (Strings fields, integer &index, Editor editor) {
	#if motif
		RECT bounds {};
		const HWND window = editor -> windowForm -> d_xmShell -> window;
		WINDOWPLACEMENT placement {};
		placement.length = sizeof (placement);
		if (IsIconic (window) && GetWindowPlacement (window, & placement))
			bounds = placement.rcNormalPosition;
		else
			GetWindowRect (window, & bounds);
		for (long value : { bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top })
			put (fields, ++ index, Melder_integer (value));
	#else
		for (int i = 0; i < 4; ++ i) put (fields, ++ index, U"0");
	#endif
}
double viewNumber (Strings fields, integer index) {
	const double value = Melder_atof (fields -> strings [index].get());
	Melder_require (std::isfinite (value), U"Invalid workspace view position.");
	return value;
}
void restoreWindow (Strings fields, integer index, Editor editor) {
	#if motif
		const auto coordinate = [&] (integer offset) {
			return (int) std::clamp (viewNumber (fields, index + offset), -32768.0, 32767.0);
		};
		int x = coordinate (0), y = coordinate (1), width = coordinate (2), height = coordinate (3);
		if (width <= 0 || height <= 0) return;
		const RECT desired { x, y, x + width, y + height };
		MONITORINFO monitor {};
		monitor.cbSize = sizeof (monitor);
		if (GetMonitorInfo (MonitorFromRect (& desired, MONITOR_DEFAULTTONEAREST), & monitor)) {
			const RECT &work = monitor.rcWork;
			width = std::min (width, (int) (work.right - work.left));
			height = std::min (height, (int) (work.bottom - work.top));
			x = std::clamp (x, (int) work.left, (int) work.right - width);
			y = std::clamp (y, (int) work.top, (int) work.bottom - height);
		}
		SetWindowPos (editor -> windowForm -> d_xmShell -> window, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
	#else
		(void) fields; (void) index; (void) editor;
	#endif
}
integer number (conststring32 text, integer maximum) {
	Melder_require (text && *text, U"Missing session number.");
	integer value = 0;
	for (const char32 *p = text; *p; ++ p) {
		Melder_require (*p >= U'0' && *p <= U'9' && value <= (maximum - (*p - U'0')) / 10,
			U"Invalid session number.");
		value = value * 10 + (*p - U'0');
	}
	Melder_require (value <= maximum, U"Session number out of range.");
	return value;
}
bool isSnapshotName (const std::string &name) {
	const std::string suffix = ".Collection";
	if (name.rfind ("session-", 0) != 0 || name.size() <= 8 + suffix.size() ||
		name.compare (name.size() - suffix.size(), suffix.size(), suffix) != 0)
		return false;
	int separators = 0;
	bool digits = false;
	for (size_t i = 8; i < name.size() - suffix.size(); ++ i) {
		if (name [i] >= '0' && name [i] <= '9') digits = true;
		else if (name [i] == '-' && digits && separators < 2) { ++ separators; digits = false; }
		else return false;
	}
	return separators == 2 && digits;
}
std::vector<fs::path> snapshots (const fs::path &directory) {
	std::vector<fs::path> result;
	if (! fs::exists (directory)) return result;
	for (const auto &entry : fs::directory_iterator (directory)) {
		const auto name = entry.path().filename().u8string();
		if (entry.is_regular_file() && isSnapshotName (name))
			result.push_back (entry.path());
	}
	std::sort (result.begin(), result.end(), [] (const auto &a, const auto &b) {
		return a.filename() > b.filename();
	});
	return result;
}
bool hasWorkspace () {
	return theCurrentPraatObjects -> n || theReferencesToAllOpenScriptEditors.size ||
		theReferencesToAllOpenNotebookEditors.size || ! currentProject.empty();
}
void setProject (fs::path path) {
	currentProject = std::move (path);
	autostring32 name = currentProject.empty() ? autostring32() : Melder_8to32_e (currentProject.stem().u8string().c_str());
	GuiShell_setTitle (theCurrentPraatApplication -> topShell,
		name ? Melder_cat (Melder_upperCaseAppName(), U" Objects — ", name.get()) :
			Melder_cat (Melder_upperCaseAppName(), U" Objects"));
}
void ensureWorkspaceGui () {
	Melder_require (! Melder_batch && ! Melder_backgrounding &&
		theCurrentPraatObjects == & theForegroundPraatObjects && theCurrentPraatApplication -> topShell,
		U"This command is available from the Praat Objects window.");
}
fs::path backupDirectory (MelderFolder folder) {
	Melder_require (! MelderFolder_isNull (folder), U"No session backup folder is available.");
	return pathFor (MelderFolder_peekPath (folder)) / "SessionBackups";
}
bool hasExtension (const fs::path &path, const char *expected) {
	auto extension = path.extension().u8string();
	for (char &letter : extension)
		if (letter >= 'A' && letter <= 'Z')
			letter += 'a' - 'A';
	return extension == expected;
}
fs::path createStagingFolder (const fs::path &directory) {
	const auto stamp = std::chrono::duration_cast<std::chrono::microseconds> (
		std::chrono::system_clock::now().time_since_epoch()).count();
	for (int attempt = 0; attempt < 100; ++ attempt) {
		const auto path = directory / ("pending-" + std::to_string (stamp) + "-" + std::to_string (std::random_device{}()));
		if (fs::create_directory (path)) return path;
	}
	Melder_throw (U"Cannot allocate a temporary workspace file.");
}
void cleanStagingFile (const fs::path &pending) {
	if (pending.empty()) return;
	std::error_code ignored;
	fs::remove (pending, ignored);
	fs::remove (pending.parent_path(), ignored);
}
void appendDraft (Strings drafts, integer &index, TextEditor editor, conststring32 kind) {
	put (drafts, ++ index, kind);
	put (drafts, ++ index, MelderFile_peekPath (& editor -> file));
	integer first, last;
	autostring32 text = GuiText_getStringAndSelectionPosition (editor -> textWidget, & first, & last);
	put (drafts, ++ index, text.get());
	put (drafts, ++ index, editor -> dirty ? U"1" : U"0");
	put (drafts, ++ index, Melder_integer (first));
	put (drafts, ++ index, Melder_integer (last));
	appendWindow (drafts, index, editor);
}
integer rowOf (Daata object) {
	if (object)
		for (integer i = 1; i <= theCurrentPraatObjects -> n; ++ i)
			if (theCurrentPraatObjects -> list [i].object == object) return i;
	return 0;
}
struct SavedView {
	FunctionEditor editor;
	integer primary, sound = 0, spelling = 0, tier = 0;
};
std::vector<SavedView> openViews () {
	std::vector<SavedView> views;
	std::set<Editor> seen;
	for (integer i = 1; i <= theCurrentPraatObjects -> n; ++ i) {
		for (Editor editor : theCurrentPraatObjects -> list [i].editors) {
			if (! editor || ! seen.insert (editor).second) continue;
			if (editor -> classInfo != classSoundEditor && editor -> classInfo != classTextGridEditor &&
					editor -> classInfo != classSpectrumEditor && editor -> classInfo != classPitchEditor &&
					editor -> classInfo != classManipulationEditor) continue;
			auto function = static_cast<FunctionEditor> (editor);
			SavedView view { function, rowOf (function -> data()) };
			if (! view.primary) continue;
			if (editor -> classInfo == classTextGridEditor) {
				auto grid = static_cast<TextGridEditor> (editor);
				view.sound = grid -> soundArea() ? rowOf (grid -> soundArea() -> soundOrLongSound()) : 0;
				if (! view.sound && grid -> workspaceSoundId)
					for (integer j = 1; j <= theCurrentPraatObjects -> n; ++ j)
						if (theCurrentPraatObjects -> list [j].id == grid -> workspaceSoundId) {
							view.sound = j;
							break;
						}
				view.spelling = rowOf (grid -> textGridArea() -> spellingChecker);
				view.tier = grid -> textGridArea() -> selectedTier;
			}
			views.push_back (view);
		}
	}
	return views;
}
autoCollection capture (const fs::path &projectDirectory = {}) {
	autoCollection result = Collection_create ();
	autoStrings header = strings (2);
	put (header.get(), 1, projectDirectory.empty() ? U"PraatSession 2" : U"PraatSession 3");
	put (header.get(), 2, Melder_integer (theCurrentPraatObjects -> sequentialUniqueIdOfLatestObjectInList));
	autoStrings metadata = strings (theCurrentPraatObjects -> n * objectFields);
	autoStrings recordings;
	if (! projectDirectory.empty())
		recordings = strings (theCurrentPraatObjects -> n * recordingFields);
	autoCollection payloads = Collection_create ();
	integer field = 0;
	for (integer i = 1; i <= theCurrentPraatObjects -> n; ++ i) {
		auto &entry = theCurrentPraatObjects -> list [i];
		const bool writable = Data_canWriteBinary (entry.object);
		const bool longSound = entry.klas == classLongSound;
		Melder_require (writable || longSound,
			U"Cannot back up ", entry.name.get(), U". Save or remove this object, "
			U"or turn off saving in Praat > Settings > Session saving settings.");
		autostring32 source;
		if (longSound) {
			const auto recording = static_cast<LongSound> (entry.object);
			const auto original = MelderFile_peekPath (& recording -> file);
			Melder_require (MelderFile_readable (& recording -> file), U"The source file for ", entry.name.get(), U" is unavailable.");
			source = Melder_dup (original);
			if (recordings) {
				const auto relative = pathFor (original).lexically_relative (projectDirectory);
				if (! relative.empty() && *relative.begin() != "..")
					source = Melder_8to32_e (relative.generic_u8string().c_str());
				const integer index = (i - 1) * recordingFields;
				put (recordings.get(), index + 1, Melder_integer (recording -> ny));
				put (recordings.get(), index + 2, Melder_integer (recording -> nx));
				put (recordings.get(), index + 3, Melder_double (recording -> dx));
			}
		}
		if (writable) payloads -> addItem_ref (entry.object);   // no second copy of a large Sound in memory
		put (metadata.get(), ++ field, entry.object -> name.get());
		put (metadata.get(), ++ field, entry.klas -> className);
		put (metadata.get(), ++ field, source.get());
		put (metadata.get(), ++ field, Melder_integer (entry.id));
		put (metadata.get(), ++ field, entry.isSelected ? U"1" : U"0");
		put (metadata.get(), ++ field, Melder_integer (writable ? payloads -> size : 0));
	}
	const integer draftCount = theReferencesToAllOpenScriptEditors.size + theReferencesToAllOpenNotebookEditors.size;
	autoStrings drafts = strings (draftCount * draftFields);
	field = 0;
	for (auto editor : theReferencesToAllOpenScriptEditors)
		appendDraft (drafts.get(), field, editor, U"script");
	for (auto editor : theReferencesToAllOpenNotebookEditors)
		appendDraft (drafts.get(), field, editor, U"notebook");
	const auto views = openViews ();
	autoStrings viewRecords = strings (integer (views.size()) * viewFields);
	field = 0;
	for (const auto &view : views) {
		put (viewRecords.get(), ++ field, Thing_className (view.editor));
		for (integer row : { view.primary, view.sound, view.spelling })
			put (viewRecords.get(), ++ field, Melder_integer (row));
		for (double value : { view.editor -> startWindow, view.editor -> endWindow,
				view.editor -> startSelection, view.editor -> endSelection })
			put (viewRecords.get(), ++ field, Melder_double (value));
		put (viewRecords.get(), ++ field, Melder_integer (view.tier));
		appendWindow (viewRecords.get(), field, view.editor);
	}
	result -> addItem_move (header.move());
	result -> addItem_move (metadata.move());
	result -> addItem_move (payloads.move());
	result -> addItem_move (drafts.move());
	result -> addItem_move (viewRecords.move());
	if (recordings) result -> addItem_move (recordings.move());
	return result;
}

fs::path resolveRecordingPath (conststring32 stored, const fs::path &projectDirectory) {
	Melder_require (stored && stored [0], U"A recording reference is empty.");
	const auto path = pathFor (stored);
	if (path.is_absolute()) return path;
	Melder_require (! projectDirectory.empty() && ! path.has_root_path(), U"Invalid relative recording path.");
	for (const auto &part : path)
		Melder_require (part != "..", U"A relative recording path leaves the project folder.");
	return projectDirectory / path;
}
struct RecordingShape {
	integer channels = 0, samples = 0;
	double period = 0.0;
	bool matches (LongSound recording) const {
		return ! channels || (channels == recording -> ny && samples == recording -> nx &&
			std::abs (period - recording -> dx) <= period * 1e-12);
	}
};
autoLongSound openRecording (const fs::path &expected, RecordingShape shape, bool askToRelink,
		std::map<fs::path, fs::path> &replacements)
{
	const auto previous = replacements.find (expected);
	auto path = previous == replacements.end() ? expected : previous -> second;
	bool picked = previous != replacements.end();
	for (;;) {
		try {
			auto file = fileFor (path);
			auto recording = LongSound_open (& file);
			Melder_require (shape.matches (recording.get()),
				U"The recording has a different sample count, channel count, or sample rate.");
			if (path != expected) replacements [expected] = path;
			return recording;
		} catch (MelderError) {
			if (! askToRelink) throw;
			if (picked)
				Melder_flushError (U"This file cannot replace the missing recording.");
			else
				Melder_clearError ();
		}
		const auto title = Melder_dup (Melder_cat (U"Locate recording: ", Melder_peek8to32_u (expected.filename().u8string().c_str())));
		const auto choices = GuiFileSelect_getInfileNames (theCurrentPraatApplication -> topShell, title.get(), false);
		if (! choices -> size) return {};
		path = pathFor (choices -> at [1] -> string.get());
		picked = true;
	}
}

bool checkpointBeforeSwitch ();

// Resolve dependencies before saving or replacing anything in the live workspace.
bool restore (autoCollection snapshot, bool replace = false, const fs::path &projectDirectory = {}, bool askToRelink = false) {
	const bool legacy = snapshot -> size == 4;
	const bool hasRecordingMetadata = snapshot -> size == 6;
	Melder_require ((legacy || snapshot -> size == 5 || hasRecordingMetadata) && snapshot -> at [1] -> classInfo == classStrings &&
		snapshot -> at [2] -> classInfo == classStrings && snapshot -> at [3] -> classInfo == classCollection &&
		snapshot -> at [4] -> classInfo == classStrings && (legacy || snapshot -> at [5] -> classInfo == classStrings) &&
		(! hasRecordingMetadata || snapshot -> at [6] -> classInfo == classStrings),
		U"Invalid session structure.");
	auto header = static_cast<Strings> (snapshot -> at [1]);
	auto meta = static_cast<Strings> (snapshot -> at [2]);
	auto data = static_cast<Collection> (snapshot -> at [3]);
	auto drafts = static_cast<Strings> (snapshot -> at [4]);
	auto views = legacy ? nullptr : static_cast<Strings> (snapshot -> at [5]);
	auto recordings = hasRecordingMetadata ? static_cast<Strings> (snapshot -> at [6]) : nullptr;
	const integer stride = legacy ? legacyDraftFields : draftFields;
	Melder_require (header -> numberOfStrings == 2 &&
		str32equ (header -> strings [1].get(), legacy ? U"PraatSession 1" : hasRecordingMetadata ? U"PraatSession 3" : U"PraatSession 2") &&
		meta -> numberOfStrings % objectFields == 0 && drafts -> numberOfStrings % stride == 0 &&
		(! views || views -> numberOfStrings % viewFields == 0),
		U"Invalid session metadata.");
	const integer count = meta -> numberOfStrings / objectFields;
	Melder_require (! recordings || recordings -> numberOfStrings == count * recordingFields, U"Invalid recording metadata.");
	Melder_require (count <= praat_MAXNUM_OBJECTS - theCurrentPraatObjects -> n, U"Too many session objects.");
	const integer latestId = number (header -> strings [2].get(), INTPTR_MAX - praat_MAXNUM_OBJECTS);
	autoCollection staged = Collection_create ();
	std::set<integer> ids;
	std::map<fs::path, fs::path> replacements;
	integer payload = 0;
	for (integer i = 0; i < count; ++ i) {
		const integer f = i * objectFields;
		const integer id = number (meta -> strings [f + 4].get(), latestId);
		Melder_require (id > 0 && ids.insert (id).second, U"Invalid or duplicate session object ID.");
		number (meta -> strings [f + 5].get(), 1);
		const integer index = number (meta -> strings [f + 6].get(), data -> size);
		autoDaata object;
		if (index) {
			Melder_require (index == ++ payload, U"Invalid session payload order.");
			Melder_require (Thing_isa (data -> at [index], classDaata), U"Invalid session data.");
			object.adoptFromAmbiguousOwner (static_cast<Daata> (data -> at [index]));
			data -> at [index] = nullptr;
		} else {
			Melder_require (str32equ (meta -> strings [f + 2].get(), U"LongSound"), U"Unknown session reference.");
			const auto expected = resolveRecordingPath (meta -> strings [f + 3].get(), projectDirectory);
			RecordingShape shape;
			if (recordings) {
				const integer dimensions = i * recordingFields;
				shape.channels = number (recordings -> strings [dimensions + 1].get(), INTPTR_MAX);
				shape.samples = number (recordings -> strings [dimensions + 2].get(), INTPTR_MAX);
				shape.period = viewNumber (recordings, dimensions + 3);
				Melder_require (shape.channels > 0 && shape.samples > 0 && shape.period > 0.0, U"Invalid recording dimensions.");
			}
			auto recording = openRecording (expected, shape, askToRelink, replacements);
			if (! recording) return false;
			object = recording.move();
		}
		Melder_require (str32equ (Thing_className (object.get()), meta -> strings [f + 2].get()), U"Wrong session object class.");
		Thing_setName (object.get(), meta -> strings [f + 1].get());
		staged -> addItem_move (object.move());
	}
	Melder_require (payload == data -> size, U"Unexpected session payloads.");
	for (integer i = 1; i <= drafts -> numberOfStrings; i += stride)
		Melder_require (str32equ (drafts -> strings [i].get(), U"script") || str32equ (drafts -> strings [i].get(), U"notebook"),
			U"Unknown session draft kind.");
	if (! projectDirectory.empty() && ! checkpointBeforeSwitch ()) return false;
	const integer previousCount = theCurrentPraatObjects -> n;
	const integer previousId = theCurrentPraatObjects -> sequentialUniqueIdOfLatestObjectInList;
	std::vector<TextEditor> previousTextEditors;
	if (replace) {
		for (auto editor : theReferencesToAllOpenScriptEditors) previousTextEditors.push_back (editor);
		for (auto editor : theReferencesToAllOpenNotebookEditors) previousTextEditors.push_back (editor);
	}
	std::vector<autoTextEditor> editors;
	std::vector<std::pair<FunctionEditor, integer>> restoredViews;
	try {
		for (integer i = 0; i < count; ++ i) {
			autostring32 originalName = Melder_dup (meta -> strings [i * objectFields + 1].get());
			praat_new (staged -> subtractItem_move (1).static_cast_move<structDaata>());
			auto &entry = theCurrentPraatObjects -> list [theCurrentPraatObjects -> n];
			Thing_setName (entry.object, originalName.get());
			// Keep IDs stable in a fresh session; avoid collisions when restored alongside start-up objects.
			if (previousCount == 0) entry.id = number (meta -> strings [i * objectFields + 4].get(), latestId);
			entry.name = Melder_dup (Melder_cat (entry.klas -> className, U" ", entry.object -> name.get()));
			praat_list_renameAndSelect (theCurrentPraatObjects -> n, Melder_cat (entry.id, U". ", entry.name.get()));
		}
		for (integer i = 1; i <= drafts -> numberOfStrings; i += stride) {
			autoTextEditor editor;
			if (str32equ (drafts -> strings [i].get(), U"script"))
				editor = ScriptEditor_createFromText (nullptr, drafts -> strings [i + 2].get());
			else
				editor = NotebookEditor_createFromText (drafts -> strings [i + 2].get());
			if (drafts -> strings [i + 1] [0]) Melder_pathToFile (drafts -> strings [i + 1].get(), & editor -> file);
			editor -> dirty = legacy || number (drafts -> strings [i + 3].get(), 1) != 0;
			Thing_setName (editor.get(), nullptr);
			if (! legacy) {
				const integer length = Melder_length (drafts -> strings [i + 2].get());
				const integer first = number (drafts -> strings [i + 4].get(), length);
				const integer last = number (drafts -> strings [i + 5].get(), length);
				Melder_require (first <= last, U"Invalid script selection.");
				GuiText_setSelection (editor -> textWidget, first, last);
				restoreWindow (drafts, i + 6, editor.get());
			}
			editors.push_back (editor.move());
		}
		for (integer i = 1; views && i <= views -> numberOfStrings; i += viewFields) {
			conststring32 kind = views -> strings [i].get();
			const integer row = number (views -> strings [i + 1].get(), count);
			const integer soundRow = number (views -> strings [i + 2].get(), count);
			const integer spellingRow = number (views -> strings [i + 3].get(), count);
			Melder_require (row, U"Missing workspace editor object.");
			auto &entry = theCurrentPraatObjects -> list [previousCount + row];
			Daata dataObject = entry.object;
			conststring32 title = Melder_cat (entry.id, U". ", entry.name.get());
			autoFunctionEditor editor;
			const bool soundObject = entry.klas == classSound || entry.klas == classLongSound;
			if (str32equ (kind, U"SoundEditor") && soundObject)
				editor = SoundEditor_create (title, static_cast<SampledXY> (dataObject));
			else if (str32equ (kind, U"TextGridEditor") && entry.klas == classTextGrid) {
				Daata sound = soundRow ? theCurrentPraatObjects -> list [previousCount + soundRow].object : nullptr;
				Daata spelling = spellingRow ? theCurrentPraatObjects -> list [previousCount + spellingRow].object : nullptr;
				Melder_require (! sound || sound -> classInfo == classSound || sound -> classInfo == classLongSound,
					U"Invalid TextGrid audio reference.");
				Melder_require (! spelling || spelling -> classInfo == classSpellingChecker, U"Invalid spelling checker reference.");
				auto grid = TextGridEditor_create (title, static_cast<TextGrid> (dataObject), static_cast<SampledXY> (sound),
					static_cast<SpellingChecker> (spelling), nullptr);
				if (soundRow) grid -> workspaceSoundId = theCurrentPraatObjects -> list [previousCount + soundRow].id;
				const integer tier = number (views -> strings [i + 8].get(), grid -> textGrid() -> tiers -> size);
				grid -> textGridArea() -> selectedTier = std::max (1_integer, tier);
				editor = grid.move();
			} else if (str32equ (kind, U"SpectrumEditor") && entry.klas == classSpectrum)
				editor = SpectrumEditor_create (title, static_cast<Spectrum> (dataObject));
			else if (str32equ (kind, U"PitchEditor") && entry.klas == classPitch)
				editor = PitchEditor_create (title, static_cast<Pitch> (dataObject));
			else if (str32equ (kind, U"ManipulationEditor") && entry.klas == classManipulation)
				editor = ManipulationEditor_create (title, static_cast<Manipulation> (dataObject));
			else
				Melder_throw (U"Invalid workspace editor type for ", entry.name.get(), U".");
			const auto restoredEditor = editor.get();
			restoredViews.emplace_back (restoredEditor, i);
			if (str32equ (kind, U"TextGridEditor") && spellingRow)
				praat_installEditor2 (restoredEditor, previousCount + row, previousCount + spellingRow);
			else
				praat_installEditor (restoredEditor, previousCount + row);
			editor.releaseToUser();
		}
		// New editors can join Praat's time group during creation. Apply the
		// saved ranges after every editor has been opened.
		for (const auto &[editor, i] : restoredViews) {
			FunctionEditor_ungroup (editor -> data());
			const auto withinDomain = [&] (integer offset) {
				return std::clamp (viewNumber (views, i + offset), editor -> tmin, editor -> tmax);
			};
			const double first = withinDomain (4), last = withinDomain (5);
			if (first < last) {
				editor -> startWindow = first;
				editor -> endWindow = last;
			}
			editor -> startSelection = withinDomain (6);
			editor -> endSelection = std::max (editor -> startSelection, withinDomain (7));
			editor -> v_windowChanged ();
			FunctionEditor_windowMarksChanged (editor, true);
			restoreWindow (views, i + 9, editor);
		}
	} catch (...) {
		while (theCurrentPraatObjects -> n > previousCount) praat_removeObject (theCurrentPraatObjects -> n);
		theCurrentPraatObjects -> sequentialUniqueIdOfLatestObjectInList = previousId;
		throw;
	}
	for (auto &editor : editors) editor.releaseToUser();   // opening never runs a script or notebook
	theCurrentPraatObjects -> sequentialUniqueIdOfLatestObjectInList = std::max (
		latestId, theCurrentPraatObjects -> sequentialUniqueIdOfLatestObjectInList);
	praat_updateSelection ();
	praat_deselectAll ();
	for (integer i = 0; i < count; ++ i)
		if (str32equ (meta -> strings [i * objectFields + 5].get(), U"1")) praat_select (previousCount + i + 1);
	if (replace) {
		for (TextEditor editor : previousTextEditors) forget (editor);
		for (integer i = previousCount; i >= 1; -- i) praat_removeObject (i);
	}
	praat_show ();
	return true;
}
autoCollection readWorkspace (const fs::path &path) {
	auto file = fileFor (path);
	autoDaata data = Data_readFromFile (& file);
	Melder_require (data && data -> classInfo == classCollection, U"This file is not a Praat workspace.");
	return data.static_cast_move<structCollection>();
}
bool checkpointBeforeSwitch () {
	if (! hasWorkspace ()) return true;
	if (praat_session_isActive ()) {
		praat_session_saveToFolder (Melder_preferencesFolder());
		return true;
	} else {
		// With recovery disabled, require an explicit choice before replacing work.
		#if motif
		return MessageBoxW (theCurrentPraatApplication -> topShell -> d_xmShell -> window,
			L"Close the current workspace without saving it?", L"Switch workspace",
			MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) == IDOK;
		#else
		GuiDialog dialog = GuiDialog_create (theCurrentPraatApplication -> topShell, 150, 100, 475, 115,
			U"Switch workspace", nullptr, nullptr, GuiDialog_Modality::BLOCKING);
		GuiLabel_createShown (dialog, 15, -15, 15, 40, U"Close the current workspace without saving it?", 0);
		GuiButton_createShown (dialog, 125, 260, 65, 95, U"Cancel", nullptr, nullptr, GuiButton_DEFAULT);
		GuiButton_createShown (dialog, 275, 455, 65, 95, U"Close workspace", nullptr, nullptr, 0);
		GuiThing_show (dialog);
		const integer clicked = GuiDialog_run (dialog);
		GuiThing_hide (dialog);
		GuiObject_destroy (dialog -> d_widget);
		return clicked == 2;
		#endif
	}
}
fs::path snapshotLabelPath (fs::path snapshot) {
	return snapshot.replace_extension (".txt");
}
autostring32 snapshotLabel (const fs::path &path, const fs::path &directory) {
	auto labelFile = fileFor (snapshotLabelPath (directory / path.filename()));
	autostring32 name;
	try {
		if (MelderFile_readable (& labelFile)) name = MelderFile_readText (& labelFile);
	} catch (MelderError) {
		Melder_clearError ();
	}
	const auto stem = path.stem().u8string();
	long long microseconds = 0;
	const auto parse = std::from_chars (stem.data() + 8, stem.data() + stem.find ('-', 8), microseconds);
	const auto time = (std::time_t) (microseconds / 1000000);
	char date [40] {};
	const auto *local = parse.ec == std::errc() ? std::localtime (& time) : nullptr;
	if (local) {
		const auto length = std::strftime (date, sizeof (date), "%Y-%m-%d  %H:%M:%S", local);
		if (length)
			std::snprintf (date + length, sizeof (date) - length, ".%03d", (int) (microseconds / 1000 % 1000));
	}
	return Melder_dup (Melder_cat (date [0] ? Melder_peek8to32_u (date) : U"Unknown date",
		U"    |    ", name && name [0] ? name.get() : U"Untitled workspace"));
}

// Read the two leading Strings records. Audio and draft text can be large and
// are never loaded just to populate the recovery preview.
std::u32string snapshotContents (const fs::path &path) {
	auto file = fileFor (path);
	autofile in = Melder_fopen (& file, "rb");
	char magic [12] {};
	Melder_require (fread (magic, 1, 12, in) == 12 && std::memcmp (magic, "ooBinaryFile", 12) == 0 &&
		strequ (bingets8 (in).get(), "Collection"), U"Preview unavailable for this file format.");
	const auto sections = bingeti32 (in);
	Melder_require (sections == 4 || sections == 5, U"Preview unavailable for this workspace format.");
	auto sectionSize = [&] (const char *type) {
		Melder_require (strequ (bingets8 (in).get(), type), U"Invalid recovery snapshot metadata.");
		(void) bingetw16 (in);   // name of the collection item
		return bingeti32 (in);
	};
	Melder_require (sectionSize ("Strings") == 2, U"Invalid recovery snapshot header.");
	const auto version = bingetw16 (in);
	Melder_require (str32equ (version.get(), U"PraatSession 1") || str32equ (version.get(), U"PraatSession 2"),
		U"Preview unavailable for this workspace version.");
	(void) bingetw16 (in);   // last Objects-list ID
	const auto fields = sectionSize ("Strings");
	Melder_require (fields >= 0 && fields % objectFields == 0 && fields <= praat_MAXNUM_OBJECTS * objectFields,
		U"Invalid recovery snapshot object count.");
	const integer count = fields / objectFields;
	if (! count && sectionSize ("Collection") == 0) {
		const auto drafts = sectionSize ("Strings");
		const integer stride = sections == 4 ? legacyDraftFields : draftFields;
		Melder_require (drafts >= 0 && drafts % stride == 0, U"Invalid recovery draft count.");
		return drafts ? Melder_cat (drafts / stride, U" script or notebook drafts · no objects") : U"Empty workspace";
	}
	std::u32string detail = Melder_cat (count, count == 1 ? U" object" : U" objects");
	const integer previewCount = std::min (count, 5_integer);
	for (integer i = 0; i < previewCount; ++ i) {
		const auto name = bingetw16 (in), type = bingetw16 (in);
		detail += Melder_cat (i == 0 ? U"  ·  " : U", ", type.get(), name && *name.get() ? U" “" : U"",
			name.get(), name && *name.get() ? U"”" : U"");
		for (integer field = 2; field < objectFields; ++ field) (void) bingetw16 (in);
	}
	if (count > previewCount) detail += Melder_cat (U"  ·  ", count - previewCount, U" more");
	return detail;
}
void publishWorkspaceFile (const fs::path &pending, const fs::path &destination, bool replaceExisting) {
	#if motif
		const DWORD flags = MOVEFILE_WRITE_THROUGH | (replaceExisting ? MOVEFILE_REPLACE_EXISTING : 0);
		Melder_require (MoveFileExW (pending.c_str(), destination.c_str(), flags),
			U"Cannot publish ", Melder_peek8to32_u (destination.filename().u8string().c_str()),
			U" (Windows error ", (integer) GetLastError (), U").");
	#else
		Melder_require (replaceExisting || ! fs::exists (destination), U"A file with that name already exists.");
		fs::rename (pending, destination);
	#endif
}
void writeProject (const fs::path &destination, bool replaceExisting) {
	fs::path pending;
	try {
		Melder_require (replaceExisting || ! fs::exists (destination), U"A project with that name already exists.");
		auto snapshot = capture (destination.parent_path());
		pending = createStagingFolder (destination.parent_path()) / "project";
		auto file = fileFor (pending);
		Data_writeToBinaryFile (snapshot.get(), & file);
		publishWorkspaceFile (pending, destination, replaceExisting);
		cleanStagingFile (pending);
	} catch (const std::exception &error) {
		cleanStagingFile (pending);
		Melder_throw (U"Could not save project: ", Melder_peek8to32_u (error.what()));
	} catch (MelderError) {
		cleanStagingFile (pending);
		throw;
	}
}

struct ArchiveFile {
	fs::path source;
	std::string name;
	uint64_t size = 0, offset = 0;
	uint32_t crc = 0;
};
void zipValue (std::ostream &out, uint64_t value, unsigned bytes) {
	char buffer [8];
	for (unsigned i = 0; i < bytes; ++ i)
		buffer [i] = char (value >> (i * 8));
	out.write (buffer, bytes);
}
// Stored ZIP entries avoid re-encoding the recordings. ZIP64 supports LongSounds
// beyond 4 GiB. See PKWARE APPNOTE sections 4.3 and 4.5.3.
void writeProjectZip (const fs::path &destination, std::vector<ArchiveFile> &files) {
	constexpr uint64_t max32 = UINT32_MAX;
	constexpr uint64_t max16 = UINT16_MAX;
	constexpr unsigned utf8Names = 1U << 11;
	constexpr unsigned dosDateTime = 0x00210000;   // 1980-01-01, midnight
	uint64_t total = 0, copied = 0;
	for (auto &entry : files) {
		entry.size = fs::file_size (entry.source);
		total += entry.size;
		Melder_require (! entry.name.empty() && entry.name.size() <= max16, U"Archive entry name is too long.");
	}
	autoMelderProgress progress (U"Packaging project...");
	std::ofstream out (destination, std::ios::binary);
	out.exceptions (std::ios::badbit | std::ios::failbit);
	auto put = [&] (uint64_t value, unsigned bytes) { zipValue (out, value, bytes); };
	std::vector<char> buffer (256 * 1024);
	for (auto &entry : files) {
		entry.offset = (uint64_t) out.tellp();
		const bool large = entry.size >= max32;
		put (0x04034b50, 4);   // local header
		put (large ? 45 : 20, 2); put (utf8Names, 2); put (0, 2); put (dosDateTime, 4);
		put (0, 4);   // CRC, filled after streaming the bytes
		put (large ? max32 : entry.size, 4); put (large ? max32 : entry.size, 4);
		put (entry.name.size(), 2); put (large ? 20 : 0, 2);
		out.write (entry.name.data(), entry.name.size());
		if (large) {
			put (1, 2); put (16, 2);
			put (entry.size, 8); put (entry.size, 8);
		}
		std::ifstream input (entry.source, std::ios::binary);
		Melder_require (input.is_open(), U"Cannot read a file included in the archive.");
		uint64_t remaining = entry.size;
		uLong checksum = crc32 (0, Z_NULL, 0);
		while (remaining) {
			const auto chunk = (std::streamsize) std::min<uint64_t> (remaining, buffer.size());
			input.read (buffer.data(), chunk);
			Melder_require (input.gcount() == chunk, U"A source file changed or could not be read during packaging.");
			out.write (buffer.data(), chunk);
			checksum = crc32 (checksum, reinterpret_cast<const Bytef *> (buffer.data()), (uInt) chunk);
			remaining -= chunk;
			copied += chunk;
			Melder_progress (0.99 * double (copied) / double (total),
				U"Packaging ", Melder_peek8to32_u (entry.name.c_str()));
		}
		Melder_require (input.peek() == std::char_traits<char>::eof() && ! input.bad(),
			U"A source file changed while packaging it.");
		entry.crc = (uint32_t) checksum;
		const auto next = out.tellp();
		out.seekp ((std::streamoff) (entry.offset + 14));
		put (entry.crc, 4);
		out.seekp (next);
	}
	const uint64_t directoryStart = (uint64_t) out.tellp();
	for (const auto &entry : files) {
		const bool large = entry.size >= max32, distant = entry.offset >= max32;
		const unsigned extra = (large ? 16 : 0) + (distant ? 8 : 0);
		put (0x02014b50, 4);   // central directory header
		put (45, 2); put (extra ? 45 : 20, 2); put (utf8Names, 2);
		put (0, 2); put (dosDateTime, 4); put (entry.crc, 4);
		put (large ? max32 : entry.size, 4); put (large ? max32 : entry.size, 4);
		put (entry.name.size(), 2); put (extra ? extra + 4 : 0, 2);
		put (0, 2); put (0, 2); put (0, 2); put (0, 4);
		put (distant ? max32 : entry.offset, 4);
		out.write (entry.name.data(), entry.name.size());
		if (extra) {
			put (1, 2); put (extra, 2);
			if (large) { put (entry.size, 8); put (entry.size, 8); }
			if (distant) put (entry.offset, 8);
		}
	}
	const uint64_t directoryEnd = (uint64_t) out.tellp();
	const uint64_t directorySize = directoryEnd - directoryStart;
	const uint64_t count = files.size();
	if (directoryStart >= max32 || directorySize >= max32 || count >= max16) {
		put (0x06064b50, 4); put (44, 8);   // ZIP64 end of central directory
		put (45, 2); put (45, 2); put (0, 4); put (0, 4);
		put (count, 8); put (count, 8); put (directorySize, 8); put (directoryStart, 8);
		put (0x07064b50, 4); put (0, 4); put (directoryEnd, 8); put (1, 4);   // ZIP64 locator
	}
	put (0x06054b50, 4);   // end of central directory
	put (0, 2); put (0, 2); put (std::min (count, max16), 2); put (std::min (count, max16), 2);
	put (std::min (directorySize, max32), 4); put (std::min (directoryStart, max32), 4); put (0, 2);
	out.close();
}

void packageProject (const fs::path &destination, bool replaceExisting) {
	fs::path staging;
	auto clean = [&] {
		if (staging.empty()) return;
		std::error_code ignored;
		fs::remove (staging / "project", ignored);
		fs::remove (staging / "archive", ignored);
		fs::remove (staging, ignored);
	};
	try {
		Melder_require (replaceExisting || ! fs::exists (destination), U"A ZIP with that name already exists.");
		staging = createStagingFolder (destination.parent_path());
		auto snapshot = capture (staging);
		auto metadata = static_cast<Strings> (snapshot -> at [2]);
		std::vector<ArchiveFile> files;
		files.push_back ({ staging / "project", destination.stem().u8string() + ".PraatProject" });
		std::map<fs::path, size_t> included;
		for (integer i = 1; i <= theCurrentPraatObjects -> n; ++ i) {
			const auto &entry = theCurrentPraatObjects -> list [i];
			if (entry.klas != classLongSound) continue;
			const auto recording = static_cast<LongSound> (entry.object);
			const auto source = fs::canonical (pathFor (MelderFile_peekPath (& recording -> file)));
			const auto [item, added] = included.emplace (source, files.size());
			if (added) {
				const auto name = "Recordings/" + std::to_string (files.size()) + "/" + source.filename().u8string();
				files.push_back ({ source, name });
			}
			put (metadata, (i - 1) * objectFields + 3, Melder_peek8to32_u (files [item -> second].name.c_str()));
		}
		auto drafts = static_cast<Strings> (snapshot -> at [4]);
		for (integer i = 1; i <= drafts -> numberOfStrings; i += draftFields) {
			// Embed editor text as a draft, independent of the sender's original path.
			put (drafts, i + 1, U"");
			put (drafts, i + 3, U"1");
		}
		auto project = fileFor (staging / "project");
		Data_writeToBinaryFile (snapshot.get(), & project);
		writeProjectZip (staging / "archive", files);
		publishWorkspaceFile (staging / "archive", destination, replaceExisting);
		clean ();
	} catch (const std::exception &error) {
		clean ();
		Melder_throw (U"Could not package the project: ", Melder_peek8to32_u (error.what()));
	} catch (MelderError) {
		clean ();
		throw;
	}
}
Thing_define (WorkspaceRecovery, Thing) {
	GuiWindow window;
	GuiList list;
	GuiButton cancel, restoreButton, moveButton, toggleButton;
	GuiLabel instruction, status;
	GuiText details;
	fs::path directory;
	std::vector<fs::path> files;
	bool showingRemoved = false, updating = false;
};
Thing_implement (WorkspaceRecovery, Thing, 0);
WorkspaceRecovery recovery;

void closeRecovery (WorkspaceRecovery me) { GuiThing_hide (my window); }
void cancelRecovery (WorkspaceRecovery me, GuiButtonEvent) { closeRecovery (me); }
void recoverySelection (WorkspaceRecovery me, GuiList_SelectionChangedEvent) {
	if (my updating) return;
	const auto selection = GuiList_getSelectedPositions (my list);
	GuiThing_setSensitive (my restoreButton, ! my showingRemoved && selection.size == 1);
	GuiThing_setSensitive (my moveButton, selection.size > 0);
	std::u32string text;
	if (selection.size == 1 && selection [1] <= (integer) my files.size()) {
		try {
			text = snapshotContents (my files [selection [1] - 1]);
		} catch (MelderError) {
			Melder_clearError ();
			text = U"The contents of this snapshot could not be previewed.";
		}
	} else if (selection.size > 1) {
		text = Melder_cat (selection.size, U" snapshots selected");
	}
	GuiText_setString (my details, text.c_str(), false);
}
void renderRecovery (WorkspaceRecovery me, integer row = 1) {
	const auto removed = my directory / "Removed";
	const auto removedFiles = snapshots (removed);
	auto files = my showingRemoved ? removedFiles : snapshots (my directory);
	my updating = true;
	my files = std::move (files);
	GuiList_deleteAllItems (my list);
	for (const auto &path : my files) {
		const auto label = snapshotLabel (path, my directory);
		GuiList_insertItem (my list, label.get(), 0);
	}
	if (! my files.empty()) GuiList_selectItem (my list, std::min (row, (integer) my files.size()));
	GuiLabel_setText (my instruction, my showingRemoved ? U"Recently removed snapshots" :
		U"Choose a saved session to reopen.");
	GuiLabel_setText (my status, my files.empty() ? (my showingRemoved ? U"No removed snapshots." : U"No recovery snapshots.") : U"");
	GuiButton_setText (my moveButton, my showingRemoved ? U"Return to recovery list" : U"Remove selected");
	GuiButton_setText (my toggleButton, my showingRemoved ? U"Back to recovery" : Melder_cat (U"Recently removed (", removedFiles.size(), U")"));
	my updating = false;
	recoverySelection (me, nullptr);
}
void toggleRemoved (WorkspaceRecovery me, GuiButtonEvent) {
	my showingRemoved = ! my showingRemoved;
	try {
		renderRecovery (me);
	} catch (const std::exception &error) {
		my showingRemoved = ! my showingRemoved;
		Melder_flushError (U"Could not read recovery history.\n", Melder_peek8to32_u (error.what()));
	}
}
void moveSnapshots (WorkspaceRecovery me, GuiButtonEvent) {
	const auto selected = GuiList_getSelectedPositions (my list);
	if (! selected.size) return;
	const auto target = my showingRemoved ? my directory : my directory / "Removed";
	integer moved = 0;
	std::u32string error;
	try {
		fs::create_directories (target);
		for (integer i = 1; i <= selected.size; ++ i) {
			const auto &path = my files [selected [i] - 1];
			// Labels stay with the backup directory so each move only publishes one file.
			publishWorkspaceFile (path, target / path.filename(), false);
			++ moved;
		}
	} catch (const std::exception &exception) {
		error = Melder_peek8to32_u (exception.what());
	} catch (MelderError) {
		error = Melder_getError ();
		Melder_clearError ();
	}
	try {
		renderRecovery (me, selected [1]);
		if (moved) GuiLabel_setText (my status, Melder_cat (moved, moved == 1 ? U" snapshot " : U" snapshots ",
			my showingRemoved ? U"returned to the recovery list." : U"moved to Recently removed."));
	} catch (const std::exception &exception) {
		if (error.empty()) error = Melder_peek8to32_u (exception.what());
	}
	if (! error.empty()) Melder_flushError (U"Could not finish moving the selected snapshots.\n", error.c_str());
}
void restoreSelected (WorkspaceRecovery me) {
	try {
		const auto selected = GuiList_getSelectedPositions (my list);
		if (my showingRemoved || selected.size != 1) return;
		auto snapshot = readWorkspace (my files [selected [1] - 1]);
		if (! checkpointBeforeSwitch ()) return;
		restore (snapshot.move(), true);
		restoreFailed = false;
		setProject ({});
		closeRecovery (me);
	} catch (MelderError) {
		Melder_flushError (U"Could not restore the selected snapshot. Choose another snapshot or close this window.");
	}
}
void recoverFromButton (WorkspaceRecovery me, GuiButtonEvent) { restoreSelected (me); }
void recoverFromList (WorkspaceRecovery me, GuiList_DoubleClickEvent) { restoreSelected (me); }
} // namespace

void praat_session_prefs () {
	Preferences_addBool (U"PraatShell.saveSessionOnExit", & saveOnExit, true);
	Preferences_addBool (U"PraatShell.periodicAutosave", & autosaveEnabled, true);
	Preferences_addInteger (U"PraatShell.autosaveMinutes", & autosaveMinutes, 5);
	Preferences_addInteger (U"PraatShell.sessionBackupsToKeep", & backupsToKeep, 200);
}
bool praat_session_isEnabled () { return saveOnExit; }
void praat_session_setEnabled (bool enabled) {
	saveOnExit = enabled;
	resetAutosaveTimer ();
}
bool praat_session_isActive () {
	return saveOnExit && ! Melder_batch && ! praatP.ignorePreferenceFiles &&
		theCurrentPraatObjects == & theForegroundPraatObjects;
}
bool praat_session_autosaveEnabled () { return autosaveEnabled; }
integer praat_session_autosaveMinutes () { normalizeSettings (); return autosaveMinutes; }
integer praat_session_backupsToKeep () { normalizeSettings (); return backupsToKeep; }
void praat_session_setAutosaveSettings (bool enabled, integer minutes, integer backups) {
	Melder_require (minutes >= 1 && minutes <= 1440, U"Autosave interval must be between 1 and 1440 minutes.");
	Melder_require (backups >= 1 && backups <= maximumBackups, U"Number of backups must be between 1 and 200.");
	autosaveEnabled = enabled;
	autosaveMinutes = minutes;
	backupsToKeep = backups;
	autosaveErrorReported = false;
	resetAutosaveTimer ();
}
void praat_session_startAutosave () {
	#if motif
		autosaveStarted = true;
		resetAutosaveTimer ();
	#endif
}
void praat_session_stopAutosave () {
	autosaveStarted = false;
	#if motif
		if (autosaveTimer) KillTimer (nullptr, autosaveTimer);
		autosaveTimer = 0;
	#endif
}
void praat_session_pollAutosave () {
	if (! autosaveStarted || ! autosaveEnabled || ! praat_session_isActive () ||
		restoreFailed || Melder_backgrounding || praatP.phase != praat_HANDLING_EVENTS ||
		std::chrono::steady_clock::now() < nextAutosave)
		return;
	try {
		praat_session_save ();
		autosaveErrorReported = false;
	} catch (MelderError) {
		// Notify once, then retry on the next interval without repeated modal errors.
		const bool alreadyReported = autosaveErrorReported;
		autosaveErrorReported = true;
		if (alreadyReported) Melder_clearError ();
		else Melder_flushError (U"Periodic autosave failed. Earlier backups have been kept. "
			U"Praat will retry at the next autosave interval.");
	}
	// Start the next interval after I/O (including slow saves), avoiding catch-up bursts.
	try {
		resetAutosaveTimer ();
	} catch (MelderError) {
		Melder_flushError (U"The next periodic autosave could not be scheduled. "
			U"Normal-exit saving remains enabled. Try applying Session saving settings again.");
	}
}

void praat_session_saveToFolder (MelderFolder folder) {
	fs::path pending;
	try {
		Melder_require (! MelderFolder_isNull (folder), U"No session backup folder is available.");
		autoCollection snapshot = capture ();
		const fs::path directory = backupDirectory (folder);
		fs::create_directories (directory);
		const auto staging = createStagingFolder (directory);
		pending = staging / "snapshot.Collection";
		auto file = fileFor (pending);
		Data_writeToBinaryFile (snapshot.get(), & file);   // closes and checks I/O before publication
		const auto completedStamp = std::chrono::duration_cast<std::chrono::microseconds> (
			std::chrono::system_clock::now().time_since_epoch()).count();
		const fs::path completed = directory / ("session-" + std::to_string (completedStamp) + "-" + staging.filename().u8string().substr (8) + ".Collection");
		fs::rename (pending, completed);   // previous completed snapshots are never overwritten
		std::error_code ignored;
		fs::remove (staging, ignored);
		pending.clear();
		if (! currentProject.empty()) {
			try {
				auto label = fileFor (snapshotLabelPath (completed));
				MelderFile_writeText_i (& label, Melder_peek8to32_u (currentProject.stem().u8string().c_str()),
					kMelder_textOutputEncoding::UTF8);
			} catch (MelderError) {
				Melder_clearError ();   // the completed snapshot is usable without its display label
			}
		}
		const auto saved = snapshots (directory);
		// Prune only after publication; periodic and exit backups share one history.
		for (size_t i = (size_t) praat_session_backupsToKeep (); i < saved.size(); ++ i)
			if (fs::remove (saved [i], ignored)) fs::remove (snapshotLabelPath (saved [i]), ignored);
	} catch (const std::exception &error) {
		cleanStagingFile (pending);
		Melder_throw (U"Session backup failed: ", Melder_peek8to32_u (error.what()), U". Praat has stayed open.");
	} catch (MelderError) {
		cleanStagingFile (pending);
		Melder_throw (U"Session backup failed. Praat has stayed open; your work is still in memory.");
	}
}

bool praat_session_restoreFromFolder (MelderFolder folder) {
	restoreWarning.reset();
	try {
		const fs::path directory = pathFor (MelderFolder_peekPath (folder)) / "SessionBackups";
		autostring32 failures;
		for (const auto &path : snapshots (directory)) {
			try {
				restore (readWorkspace (path));
				if (failures) restoreWarning = Melder_dup (failures.get());
				return true;
			} catch (MelderError) {
				if (! failures) failures = Melder_dup (Melder_getError());
				Melder_clearError ();
			}
		}
		if (failures) Melder_throw (U"The saved session could not be restored. The backup files have been kept.\n", failures.get());
		return false;
	} catch (const std::exception &error) {
		Melder_throw (U"Cannot read the session backup folder: ", Melder_peek8to32_u (error.what()));
	}
}
void praat_session_save () {
	if (praat_session_isActive ()) {
		Melder_require (! restoreFailed,
			U"The previous session could not be restored. Its backups have been kept. "
			U"To close without saving a new session, turn off Praat > Settings > Session saving settings.");
		if (hasWorkspace ()) praat_session_saveToFolder (Melder_preferencesFolder());
	}
}

bool praat_project_save () {
	ensureWorkspaceGui ();
	if (currentProject.empty()) return false;
	writeProject (currentProject, true);
	return true;
}

void praat_project_saveAs (MelderFile file, bool overwriteConfirmed) {
	ensureWorkspaceGui ();
	auto path = pathFor (MelderFile_peekPath (file));
	const bool namedProject = hasExtension (path, ".praatproject");
	if (! namedProject) path += ".PraatProject";
	writeProject (path, namedProject && overwriteConfirmed);
	setProject (std::move (path));
}

void praat_project_package (MelderFile file, bool overwriteConfirmed) {
	ensureWorkspaceGui ();
	Melder_require (hasWorkspace (), U"Open a project or add objects before packaging a workspace.");
	auto path = pathFor (MelderFile_peekPath (file));
	const bool namedZip = hasExtension (path, ".zip");
	if (! namedZip) path += ".zip";
	packageProject (path, namedZip && overwriteConfirmed);
}

void praat_project_open (MelderFile file, bool askToRelink) {
	ensureWorkspaceGui ();
	auto path = pathFor (MelderFile_peekPath (file));
	Melder_require (hasExtension (path, ".praatproject"),
		U"Choose a .PraatProject file. If it is in a ZIP, extract the ZIP first.");
	auto snapshot = readWorkspace (path);
	if (restore (snapshot.move(), true, path.parent_path(), askToRelink))
		setProject (std::move (path));
}

void praat_project_new () {
	ensureWorkspaceGui ();
	if (! checkpointBeforeSwitch ()) return;
	while (theReferencesToAllOpenScriptEditors.size) {
		ScriptEditor editor = theReferencesToAllOpenScriptEditors.at [1];
		forget (editor);
	}
	while (theReferencesToAllOpenNotebookEditors.size) {
		NotebookEditor editor = theReferencesToAllOpenNotebookEditors.at [1];
		forget (editor);
	}
	while (theCurrentPraatObjects -> n) praat_removeObject (theCurrentPraatObjects -> n);
	praat_show ();
	setProject ({});
}

void praat_session_choose (bool atStartup) {
	ensureWorkspaceGui ();
	if (MelderFolder_isNull (Melder_preferencesFolder()) || (atStartup && ! praat_session_isActive ())) return;
	try {
		const auto directory = backupDirectory (Melder_preferencesFolder());
		if (atStartup && snapshots (directory).empty()) return;
		if (! recovery) {
			auto me = Thing_new (WorkspaceRecovery);
			my window = GuiWindow_create (150, 100, 790, 450, 740, 360, U"Recover workspace", closeRecovery, me.get(), 0);
			my instruction = GuiLabel_createShown (my window, 15, -15, 15, 38, U"", 0);
			my list = GuiList_createShown (my window, 15, -15, 45, -165, true, U"Recovery snapshots");
			my details = GuiText_createShown (my window, 15, -15, -154, -86, GuiText_SCROLLED | GuiText_ANYWRAP | GuiText_NONEDITABLE);
			my status = GuiLabel_createShown (my window, 15, -15, -79, -55, U"", 0);
			my toggleButton = GuiButton_createShown (my window, 15, 164, -50, -20, U"Recently removed", toggleRemoved, me.get(), 0);
			my moveButton = GuiButton_createShown (my window, 175, 358, -50, -20, U"Remove selected", moveSnapshots, me.get(), 0);
			my cancel = GuiButton_createShown (my window, -328, -185, -50, -20, U"Close", cancelRecovery, me.get(), GuiButton_CANCEL);
			my restoreButton = GuiButton_createShown (my window, -174, -15, -50, -20, U"Restore selected", recoverFromButton, me.get(), GuiButton_DEFAULT);
			GuiList_setSelectionChangedCallback (my list, recoverySelection, me.get());
			GuiList_setDoubleClickCallback (my list, recoverFromList, me.get());
			recovery = me.releaseToAmbiguousOwner();
		}
		GuiButton_setText (recovery -> cancel, atStartup ? U"Start empty" : U"Cancel");
		recovery -> directory = directory;
		recovery -> showingRemoved = false;
		renderRecovery (recovery);
		GuiThing_show (recovery -> window);
	} catch (const std::exception &error) {
		Melder_throw (U"Cannot read recovery snapshots: ", Melder_peek8to32_u (error.what()));
	}
}
void praat_session_restore () {
	if (praat_session_isActive ()) {
		try {
			praat_session_restoreFromFolder (Melder_preferencesFolder());
			restoreFailed = false;
			if (restoreWarning) Melder_warning (U"Praat restored an older session because a newer backup could not be read. "
				U"All backup files have been kept. Check your Objects and source files.\n", restoreWarning.get());
		} catch (MelderError) {
			restoreFailed = true;
			throw;
		}
	}
}
