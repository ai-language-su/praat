#include "CorpusAnnotations.h"
#include "praatP.h"
#include "TextGridEditor.h"
#include <algorithm>

namespace {
namespace fs = std::filesystem;
using namespace CorpusAnnotations;

constexpr integer hitsPerPage = 500;

std::u32string showPath (const fs::path &path) {
	return Melder_peek8to32_u (path.generic_u8string().c_str());
}
fs::path chosenPath (conststring32 path) { return fs::u8path (Melder_peek32to8 (path)); }
conststring32 shortText (std::u32string &text) {
	for (auto &c : text) if (c == U'\r' || c == U'\n' || c == U'\t') c = U' ';
	return text.c_str();
}
structMelderFile asFile (const fs::path &path) {
	structMelderFile file {};
	Melder_pathToFile (Melder_peek8to32_u (path.u8string().c_str()), & file);
	return file;
}
template<typename Action>
void handle (Action action) {
	try {
		action ();
	} catch (MelderError) {
		Melder_flushError ();
	} catch (const std::exception &error) {
		Melder_flushError (Melder_peek8to32_u (error.what()));
	}
}
bool soundClass (ClassInfo klas) { return klas == classSound || klas == classLongSound; }
auto &objectAt (integer row) { return theCurrentPraatObjects -> list [row]; }

integer objectForFile (MelderFile file, bool recording) {
	for (integer i = 1; i <= theCurrentPraatObjects -> n; ++ i) {
		auto &entry = objectAt (i);
		if (! (recording ? soundClass (entry.klas) : entry.klas == classTextGrid) || MelderFile_isNull (& entry.file)) continue;
		if (MelderFile_equal (file, & entry.file)) return i;
		#if defined (_WIN32)
			if (Melder_equ_caseInsensitive (MelderFile_peekPath (file), MelderFile_peekPath (& entry.file))) return i;
		#endif
	}
	return 0;
}
TextGrid gridInObjects (const fs::path &path) {
	auto file = asFile (path);
	const auto row = objectForFile (& file, false);
	return row ? static_cast<TextGrid> (objectAt (row).object) : nullptr;
}
TextGridEditor openEditor (integer gridRow, integer soundRow) {
	auto &grid = objectAt (gridRow);
	const auto soundId = soundRow ? objectAt (soundRow).id : 0;
	for (const auto candidate : grid.editors) {
		if (candidate && candidate -> classInfo == classTextGridEditor) {
			const auto editor = static_cast<TextGridEditor> (candidate);
			if (editor -> workspaceSoundId == soundId && bool (editor -> soundArea()) == bool (soundRow)) {
				Editor_raise (editor);
				return editor;
			}
		}
	}
	const auto sound = soundRow ? static_cast<SampledXY> (objectAt (soundRow).object) : nullptr;
	auto editor = TextGridEditor_create (Melder_cat (grid.id, U". ", grid.name.get()),
		static_cast<TextGrid> (grid.object), sound, nullptr, nullptr);
	editor -> workspaceSoundId = soundId;
	if (soundRow && objectAt (soundRow).klas == classLongSound)
		praat_installEditor2 (editor.get(), gridRow, soundRow);
	else
		praat_installEditor (editor.get(), gridRow);
	const auto opened = editor.get();
	editor.releaseToUser ();
	return opened;
}
void jump (const fs::path &folder, const Hit &hit) {
	auto gridFile = asFile (folder / hit.grid);
	auto gridRow = objectForFile (& gridFile, false);
	autoDaata loadedGrid;
	if (! gridRow) {
		loadedGrid = Data_readFromFile (& gridFile);
		Melder_require (loadedGrid && loadedGrid -> classInfo == classTextGrid, U"The annotation file is no longer a TextGrid.");
	}
	const auto grid = static_cast<TextGrid> (gridRow ? objectAt (gridRow).object : loadedGrid.get());
	Melder_require (stillMatches (grid, hit), U"This annotation changed. Search again to update the matches.");
	integer soundRow = 0;
	autoDaata loadedSound;
	structMelderFile soundFile {};
	std::u32string recordingIssue;
	if (! hit.recording.empty()) {
		soundFile = asFile (folder / hit.recording);
		soundRow = objectForFile (& soundFile, true);
		if (! soundRow) {
			try {
				loadedSound = Data_readFromFile (& soundFile);
				Melder_require (loadedSound && soundClass (loadedSound -> classInfo), U"The paired file could not be opened as audio.");
			} catch (MelderError) {
				recordingIssue = Melder_getError ();
				Melder_clearError ();
				loadedSound.reset ();
			}
		}
	}
	const auto before = theCurrentPraatObjects -> n;
	try {
		if (loadedSound) {
			praat_newWithFile (loadedSound.move(), & soundFile, U"");
			soundRow = theCurrentPraatObjects -> n;
		}
		if (loadedGrid) {
			praat_newWithFile (loadedGrid.move(), & gridFile, U"");
			gridRow = theCurrentPraatObjects -> n;
		}
		const auto editor = openEditor (gridRow, soundRow);
		FunctionEditor_ungroup (editor -> data());
		editor -> textGridArea() -> selectedTier = hit.tier;
		editor -> startSelection = hit.start;
		editor -> endSelection = hit.end;
		const double padding = std::max (0.25, (hit.end - hit.start) * 0.5);
		editor -> startWindow = std::max (editor -> tmin, hit.start - padding);
		editor -> endWindow = std::min (editor -> tmax, hit.end + padding);
		editor -> v_windowChanged ();
		FunctionEditor_windowMarksChanged (editor, true);
		if (theCurrentPraatObjects -> n > before) praat_updateSelection ();
	} catch (...) {
		while (theCurrentPraatObjects -> n > before) praat_removeObject (theCurrentPraatObjects -> n);
		throw;
	}
	if (! recordingIssue.empty()) Melder_warning (U"Opened the TextGrid alone.\n", recordingIssue.c_str());
}

Thing_define (CorpusSearchWindow, Thing) {
	GuiWindow window;
	GuiLabel folderLabel, summary;
	GuiText query, tier, detail;
	GuiCheckButton exact, matchCase;
	GuiList list;
	GuiButton open, previous, next, errors;
	fs::path folder;
	SearchResult result;
	integer offset = 0;
	bool searched = false, rendering = false, searching = false;
};
Thing_implement (CorpusSearchWindow, Thing, 0);
CorpusSearchWindow searchWindow;

const Hit *selectedHit (CorpusSearchWindow me) {
	const auto indices = GuiList_getSelectedPositions (my list);
	return indices.size && my offset + indices [1] <= (integer) my result.hits.size() ?
		& my result.hits [my offset + indices [1] - 1] : nullptr;
}
void searchSelection (CorpusSearchWindow me, GuiList_SelectionChangedEvent) {
	if (my rendering) return;
	const auto hit = selectedHit (me);
	GuiThing_setSensitive (my open, hit != nullptr);
	if (! hit) {
		GuiText_setString (my detail, U"", false);
		return;
	}
	GuiText_setString (my detail, Melder_cat (showPath (hit -> grid).c_str(), U"  ·  ", hit -> tierName.c_str(), U"  #", hit -> annotation, U"\n",
		Melder_double (hit -> start), U" – ", Melder_double (hit -> end), U" s  ·  ", hit -> label.c_str(), U"\n",
		hit -> recording.empty() ? hit -> recordingIssue.c_str() : showPath (hit -> recording).c_str()), false);
}
void render (CorpusSearchWindow me) {
	my rendering = true;
	GuiList_deleteAllItems (my list);
	const integer count = (integer) my result.hits.size();
	const integer end = std::min (count, my offset + hitsPerPage);
	for (integer i = my offset; i < end; ++ i) {
		const auto &hit = my result.hits [i];
		auto label = hit.label;
		GuiList_insertItem (my list, Melder_cat (Melder_fixed (hit.start, 3), U" s  |  ", hit.tierName.c_str(), U"  |  ",
			shortText (label), U"  |  ", showPath (hit.grid).c_str()), 0);
	}
	if (end > my offset) GuiList_selectItem (my list, 1);
	GuiLabel_setText (my summary, ! my searched ? U"" : Melder_cat (my result.cancelled ? U"Stopped  ·  " : U"", count, U" matches  ·  ", my result.gridsSearched,
		U" TextGrids searched  ·  ", my result.errors.size(), U" unreadable",
		count > hitsPerPage ? Melder_cat (U"  ·  showing ", my offset + 1, U"–", end) : U""));
	GuiThing_setSensitive (my previous, my offset > 0);
	GuiThing_setSensitive (my next, end < count);
	GuiThing_setSensitive (my errors, ! my result.errors.empty());
	my rendering = false;
	searchSelection (me, nullptr);
}
void chooseCorpus (CorpusSearchWindow me, GuiButtonEvent) {
	if (my searching) return;
	handle ([&] {
		const auto path = GuiFileSelect_getFolderName (my window, U"Choose the TextGrid corpus folder");
		if (! path) return;
		my folder = fs::canonical (chosenPath (path.get()));
		GuiLabel_setText (my folderLabel, showPath (my folder).c_str());
		my result = {};
		my searched = false;
		my offset = 0;
		render (me);
	});
}
void runSearch (CorpusSearchWindow me, GuiButtonEvent) {
	if (my searching) return;
	my searching = true;
	handle ([&] {
		const auto query = GuiText_getString (my query), tier = GuiText_getString (my tier);
		my result = search (my folder, query.get(), tier.get(), GuiCheckButton_getValue (my exact),
			GuiCheckButton_getValue (my matchCase), gridInObjects);
		my searched = true;
		my offset = 0;
		render (me);
	});
	my searching = false;
}
void openHit (CorpusSearchWindow me) {
	if (! my searching) handle ([&] { if (const auto hit = selectedHit (me)) jump (my folder, *hit); });
}
void openHitButton (CorpusSearchWindow me, GuiButtonEvent) { openHit (me); }
void doubleClick (CorpusSearchWindow me, GuiList_DoubleClickEvent) { openHit (me); }
void previousHits (CorpusSearchWindow me, GuiButtonEvent) { my offset -= hitsPerPage; render (me); }
void nextHits (CorpusSearchWindow me, GuiButtonEvent) { my offset += hitsPerPage; render (me); }
void searchErrors (CorpusSearchWindow me, GuiButtonEvent) {
	MelderInfo_open ();
	for (const auto &error : my result.errors) MelderInfo_writeLine (error.c_str());
	MelderInfo_close ();
}
void closeSearch (CorpusSearchWindow me) { if (! my searching) GuiThing_hide (my window); }
void closeSearchButton (CorpusSearchWindow me, GuiButtonEvent) { closeSearch (me); }

Thing_define (AnnotationTemplatesWindow, Thing) {
	GuiWindow window;
	GuiText definition;
	GuiLabel status;
	GuiCheckButton collect;
};
Thing_implement (AnnotationTemplatesWindow, Thing, 0);
AnnotationTemplatesWindow templatesWindow;

Template currentTemplate (AnnotationTemplatesWindow me) {
	const auto text = GuiText_getString (my definition);
	return parseTemplate (text.get());
}
void showTemplate (AnnotationTemplatesWindow me, const Template &layout, conststring32 title) {
	GuiText_setString (my definition, templateBody (layout).c_str(), false);
	GuiLabel_setText (my status, title);
}
integer selectedObject (bool recording, bool required) {
	integer row = 0;
	for (integer i = 1; i <= theCurrentPraatObjects -> n; ++ i) {
		const auto &entry = objectAt (i);
		if (entry.isSelected && (recording ? soundClass (entry.klas) : entry.klas == classTextGrid)) {
			Melder_require (! row, recording ? U"Select one Sound or LongSound in Objects." : U"Select one TextGrid in Objects.");
			row = i;
		}
	}
	Melder_require (! required || row, recording ? U"Select one Sound or LongSound in Objects." : U"Select one TextGrid in Objects.");
	return row;
}
void loadTemplate (AnnotationTemplatesWindow me, GuiButtonEvent) {
	handle ([&] {
		const auto files = GuiFileSelect_getInfileNames (my window, U"Load annotation template", false);
		if (! files -> size) return;
		const auto path = chosenPath (files -> at [1] -> string.get());
		showTemplate (me, readTemplate (path), showPath (path.filename()).c_str());
	});
}
void saveTemplate (AnnotationTemplatesWindow me, GuiButtonEvent) {
	handle ([&] {
		const auto layout = currentTemplate (me);
		const auto name = GuiFileSelect_getOutfileName (my window, U"Save annotation template", U"Corpus.PraatAnnotationTemplate");
		if (! name) return;
		auto path = chosenPath (name.get());
		const bool exact = Melder_endsWith_caseAware (name.get(), U".PraatAnnotationTemplate");
		if (! exact) path += ".PraatAnnotationTemplate";
		writeTemplate (path, layout, exact);
		GuiLabel_setText (my status, showPath (path.filename()).c_str());
	});
}
void importGrid (AnnotationTemplatesWindow me, GuiButtonEvent) {
	handle ([&] {
		const auto grid = static_cast<TextGrid> (objectAt (selectedObject (false, true)).object);
		showTemplate (me, fromGrid (grid, GuiCheckButton_getValue (my collect)), U"From selected TextGrid");
	});
}
void newGrid (AnnotationTemplatesWindow me, GuiButtonEvent) {
	handle ([&] {
		auto layout = currentTemplate (me);
		const auto soundRow = selectedObject (true, true);
		const auto sound = static_cast<SampledXY> (objectAt (soundRow).object);
		const auto before = theCurrentPraatObjects -> n;
		try {
			praat_new (createGrid (layout, sound -> xmin, sound -> xmax), sound -> name.get());
			const auto editor = openEditor (theCurrentPraatObjects -> n, soundRow);
			editor -> annotationTemplate = std::move (layout);
			praat_updateSelection ();
		} catch (...) {
			while (theCurrentPraatObjects -> n > before) praat_removeObject (theCurrentPraatObjects -> n);
			throw;
		}
	});
}
void validateSelection (AnnotationTemplatesWindow me, GuiButtonEvent) {
	handle ([&] {
		const auto grid = static_cast<TextGrid> (objectAt (selectedObject (false, true)).object);
		const auto issues = validateGrid (grid, currentTemplate (me));
		MelderInfo_open ();
		MelderInfo_writeLine (grid -> name.get(), U" — ", issues.size(), U" template issues");
		for (const auto &issue : issues) MelderInfo_writeLine (issue.c_str());
		MelderInfo_close ();
	});
}
void closeTemplates (AnnotationTemplatesWindow me) { GuiThing_hide (my window); }
void closeTemplatesButton (AnnotationTemplatesWindow me, GuiButtonEvent) { closeTemplates (me); }
} // namespace

void CorpusSearchWindow_show () {
	if (Melder_batch) return;
	if (! searchWindow) {
		auto me = Thing_new (CorpusSearchWindow);
		my window = GuiWindow_create (100, 90, 930, 610, 730, 550, U"Search TextGrids", closeSearch, me.get(), 0);
		GuiButton_createShown (my window, 15, 152, 12, 43, U"Choose folder…", chooseCorpus, me.get(), 0);
		my folderLabel = GuiLabel_createShown (my window, 162, -15, 12, 43, U"No corpus folder selected", 0);
		GuiLabel_createShown (my window, 15, 395, 54, 74, U"Label", 0);
		my query = GuiText_createShown (my window, 15, 395, 77, 105, 0);
		GuiLabel_createShown (my window, 408, 654, 54, 74, U"Tier name (optional)", 0);
		my tier = GuiText_createShown (my window, 408, 654, 77, 105, 0);
		my exact = GuiCheckButton_createShown (my window, 15, 150, 112, 140, U"Whole label", nullptr, nullptr, 0);
		my matchCase = GuiCheckButton_createShown (my window, 162, 300, 112, 140, U"Match case", nullptr, nullptr, 0);
		GuiButton_createShown (my window, 548, 654, 111, 142, U"Search", runSearch, me.get(), GuiButton_DEFAULT);
		my summary = GuiLabel_createShown (my window, 15, -15, 151, 176, U"", 0);
		my list = GuiList_createShown (my window, 15, -15, 179, -149, false, U"Time, tier, label, TextGrid");
		GuiList_setSelectionChangedCallback (my list, searchSelection, me.get());
		GuiList_setDoubleClickCallback (my list, doubleClick, me.get());
		my detail = GuiText_createShown (my window, 15, -15, -140, -61, GuiText_SCROLLED | GuiText_ANYWRAP | GuiText_NONEDITABLE);
		my open = GuiButton_createShown (my window, 15, 147, -49, -15, U"Open match", openHitButton, me.get(), 0);
		my previous = GuiButton_createShown (my window, 158, 245, -49, -15, U"Previous", previousHits, me.get(), 0);
		my next = GuiButton_createShown (my window, 255, 325, -49, -15, U"Next", nextHits, me.get(), 0);
		my errors = GuiButton_createShown (my window, 338, 460, -49, -15, U"File errors", searchErrors, me.get(), 0);
		GuiButton_createShown (my window, -89, -15, -49, -15, U"Close", closeSearchButton, me.get(), GuiButton_CANCEL);
		render (me.get());
		searchWindow = me.releaseToAmbiguousOwner ();
	}
	GuiThing_show (searchWindow -> window);
}

void AnnotationTemplatesWindow_show () {
	if (Melder_batch) return;
	if (! templatesWindow) {
		auto me = Thing_new (AnnotationTemplatesWindow);
		my window = GuiWindow_create (120, 100, 790, 565, 760, 465, U"Annotation Templates", closeTemplates, me.get(), 0);
		GuiButton_createShown (my window, 15, 131, 14, 46, U"Load template…", loadTemplate, me.get(), 0);
		GuiButton_createShown (my window, 142, 278, 14, 46, U"Save template as…", saveTemplate, me.get(), 0);
		GuiButton_createShown (my window, 290, 463, 14, 46, U"Use selected TextGrid", importGrid, me.get(), 0);
		my collect = GuiCheckButton_createShown (my window, 476, 710, 14, 46, U"Include its labels as vocabulary", nullptr, nullptr, 0);
		my status = GuiLabel_createShown (my window, 15, -15, 54, 77, U"Untitled template", GuiLabel_BOLD);
		GuiLabel_createShown (my window, 15, -15, 84, 107, U"Use ‘interval:’ or ‘point:’ and a tier name. Add each allowed label on its own ‘=label’ line.", 0);
		my definition = GuiText_createShown (my window, 15, -15, 111, -86, GuiText_SCROLLED | GuiText_ANYWRAP);
		GuiText_setString (my definition, U"interval: words\npoint: events\n  =breath\n  =pause\n", false);
		GuiLabel_createShown (my window, 15, -15, -78, -54, U"Select a recording to create a TextGrid, or select a TextGrid to validate it.", 0);
		GuiButton_createShown (my window, 15, 227, -49, -15, U"Create TextGrid for recording", newGrid, me.get(), 0);
		GuiButton_createShown (my window, 240, 438, -49, -15, U"Validate selected TextGrid", validateSelection, me.get(), 0);
		GuiButton_createShown (my window, -89, -15, -49, -15, U"Close", closeTemplatesButton, me.get(), GuiButton_CANCEL);
		templatesWindow = me.releaseToAmbiguousOwner ();
	}
	GuiThing_show (templatesWindow -> window);
}
