#include "BatchAnalysis.h"
#include "praatP.h"
#include <algorithm>

namespace {
namespace fs = std::filesystem;
using namespace BatchAnalysis;

constexpr integer maxDisplayRows = 1000, previewPairs = 3;
enum class Page { files, preview, results };

struct Report {
	fs::path folder;
	Preset preset;
	std::vector<Pair> pairs;
	Result result;
	bool exists = false;
};

Thing_define (BatchWindow, Thing) {
	GuiWindow window;
	GuiLabel presetName, folderName, summary;
	GuiText tier, label, ceiling, floor, top, detail;
	GuiCheckButton duration, formants, pitch;
	GuiList rows;
	GuiRadioButton filesTab, previewTab, resultsTab;
	GuiButton exportButton;
	fs::path folder;
	std::vector<Pair> pairs;
	Report preview, results;
	Page page = Page::files;
	bool busy = false, updating = false, checkedFiles = false;
};
Thing_implement (BatchWindow, Thing, 0);
BatchWindow window;

std::u32string textOf (const fs::path &path) {
	return Melder_peek8to32_u (path.generic_u8string().c_str());
}
std::u32string widgetText (GuiText text) {
	const auto value = GuiText_getString (text);
	return value ? value.get() : U"";
}
std::u32string rowText (std::u32string value) {
	for (auto &c : value)
		if (c == U'\n' || c == U'\r' || c == U'\t') c = U' ';
	return value;
}
void labelValue (GuiLabel label, conststring32 prefix, const std::u32string &value) {
	GuiLabel_setText (label, Melder_cat (prefix, value.c_str()));
}
void reportError (const std::exception &error) {
	Melder_appendError (Melder_peek8to32_u (error.what()));
	Melder_flushError (U"Batch analysis could not complete this action.");
}
template<typename Action>
void perform (BatchWindow me, Action action) {
	if (my busy) return;
	my busy = true;
	try {
		action ();
	} catch (MelderError) {
		Melder_flushError ();
	} catch (const std::exception &error) {
		reportError (error);
	}
	my busy = false;
}

Preset readControls (BatchWindow me) {
	Preset preset;
	preset.tier = widgetText (my tier);
	preset.label = widgetText (my label);
	preset.duration = GuiCheckButton_getValue (my duration);
	preset.formants = GuiCheckButton_getValue (my formants);
	preset.pitch = GuiCheckButton_getValue (my pitch);
	preset.formantCeiling = parseNumber (widgetText (my ceiling).c_str(), U"Formant ceiling");
	preset.pitchFloor = parseNumber (widgetText (my floor).c_str(), U"Pitch floor");
	preset.pitchTop = parseNumber (widgetText (my top).c_str(), U"Pitch top");
	validate (preset);
	return preset;
}
void showControls (BatchWindow me, const Preset &preset, const std::u32string &name) {
	my updating = true;
	GuiText_setString (my tier, preset.tier.c_str(), false);
	GuiText_setString (my label, preset.label.c_str(), false);
	GuiCheckButton_setValue (my duration, preset.duration);
	GuiCheckButton_setValue (my formants, preset.formants);
	GuiCheckButton_setValue (my pitch, preset.pitch);
	GuiText_setString (my ceiling, Melder_double (preset.formantCeiling), false);
	GuiText_setString (my floor, Melder_double (preset.pitchFloor), false);
	GuiText_setString (my top, Melder_double (preset.pitchTop), false);
	my updating = false;
	labelValue (my presetName, U"", name);
}

std::u32string pairName (const Pair &pair) {
	return textOf (pair.audio.empty() ? pair.grid : pair.audio);
}
std::u32string measurementText (const Row &row, const Preset &preset) {
	auto value = [] (double hz) -> std::u32string { return isdefined (hz) ? Melder_fixed (hz, 1) : U"—"; };
	std::u32string metrics;
	if (preset.duration) metrics += Melder_cat (U"  |  ", Melder_fixed (row.end - row.start, 3), U" s");
	if (preset.formants) metrics += U"  |  F1 " + value (row.f1) + U"  F2 " + value (row.f2);
	if (preset.pitch) metrics += U"  |  F0 " + value (row.f0);
	return metrics;
}
const Report &displayedReport (BatchWindow me) { return my page == Page::preview ? my preview : my results; }
void showDetail (BatchWindow me) {
	const auto selected = GuiList_getSelectedPositions (my rows);
	if (! selected.size) {
		GuiText_setString (my detail, my page == Page::files ?
			U"Select up to three file pairs to preview. With no selection, Preview uses the first three ready pairs." :
			U"Select a row for its recording, interval and measurement details.", false);
		return;
	}
	const auto index = selected [1] - 1;
	if (my page == Page::files && index < (integer) my pairs.size()) {
		const auto &pair = my pairs [index];
		GuiText_setString (my detail, Melder_cat (U"Recording: ", textOf (pair.audio).c_str(),
			U"\nTextGrid: ", textOf (pair.grid).c_str(), U"\n",
			pair.problem.empty() ? U"Ready. Matching intervals: " : pair.problem.c_str(),
			pair.problem.empty() ? Melder_integer (pair.matchingIntervals) : U""), false);
	} else if (my page != Page::files) {
		const auto &report = displayedReport (me);
		if (index < (integer) report.result.rows.size() && index < maxDisplayRows) {
			const auto &row = report.result.rows [index];
			const auto &pair = report.pairs [row.pair];
			const auto metrics = measurementText (row, report.preset);
			GuiText_setString (my detail, Melder_cat (pairName (pair).c_str(), U"\n", report.preset.tier.c_str(), U" · interval ",
				row.interval, U" · ", row.label.c_str(), U"\n",
				Melder_double (row.start), U" – ", Melder_double (row.end), U" s", metrics.c_str(),
				row.problem.empty() ? U"" : U"\n", row.problem.c_str()), false);
		} else {
			const integer failure = index - std::min (maxDisplayRows, (integer) report.result.rows.size());
			if (failure >= 0 && failure < (integer) report.result.failures.size()) {
				const auto &[pair, issue] = report.result.failures [failure];
				GuiText_setString (my detail, Melder_cat (pairName (report.pairs [pair]).c_str(), U"\n", issue.c_str()), false);
			}
		}
	}
}
void selectionChanged (BatchWindow me, GuiList_SelectionChangedEvent) { if (! my updating) showDetail (me); }

void render (BatchWindow me, Page page, bool keepSelection = false) {
	auto selected = keepSelection ? GuiList_getSelectedPositions (my rows) : autoINTVEC();
	my updating = true;
	my page = page;
	GuiRadioButton_set (page == Page::files ? my filesTab : page == Page::preview ? my previewTab : my resultsTab);
	GuiList_deleteAllItems (my rows);
	if (page == Page::files) {
		integer ready = 0;
		for (const auto &pair : my pairs) {
			ready += pair.problem.empty();
			const auto name = pairName (pair);
			GuiList_insertItem (my rows, Melder_cat (name.c_str(), U"  |  ",
				pair.problem.empty() ? U"Ready" : rowText (pair.problem).c_str()), 0);
		}
		GuiLabel_setText (my summary,
			my folder.empty() ? U"Choose the folder that contains the recordings and TextGrids." :
			! my checkedFiles ? U"Check files to update the list for the current settings." :
			my pairs.empty() ? U"No supported recordings or TextGrids in this folder." :
			Melder_cat (ready, U" ready  ·  ", integer (my pairs.size()) - ready, U" need attention"));
	} else {
		const auto &report = displayedReport (me);
		const auto &result = report.result;
		integer fieldsWithIssues = 0, preflightSkipped = 0;
		for (const auto &pair : report.pairs) preflightSkipped += ! pair.problem.empty();
		for (const auto &row : result.rows) {
			fieldsWithIssues += ! row.problem.empty();
			if (GuiList_getNumberOfItems (my rows) >= maxDisplayRows) continue;
			const auto metrics = measurementText (row, report.preset);
			const auto name = textOf (report.pairs [row.pair].audio.filename());
			GuiList_insertItem (my rows, Melder_cat (name.c_str(), U"  #", row.interval, U"  ",
				rowText (row.label).c_str(), metrics.c_str()), 0);
		}
		for (const auto &[index, message] : result.failures)
			GuiList_insertItem (my rows, Melder_cat (U"Failed  |  ", pairName (report.pairs [index]).c_str(),
				U"  |  ", rowText (message).c_str()), 0);
		GuiLabel_setText (my summary, report.exists ? Melder_cat (
			result.cancelled ? U"Stopped  ·  " : U"", integer (result.rows.size()), U" rows  ·  ",
			fieldsWithIssues, U" with issues  ·  ", preflightSkipped + (integer) result.failures.size(), U" skipped or failed",
			result.rows.size() > maxDisplayRows ? U"  ·  first 1000 shown" : U"") :
			page == Page::preview ? U"Preview measures up to three file pairs." : U"Run the analysis to produce results.");
	}
	for (integer i = 1; i <= selected.size; ++ i)
		if (selected [i] <= GuiList_getNumberOfItems (my rows)) GuiList_selectItem (my rows, selected [i]);
	GuiThing_setSensitive (my exportButton, my results.exists);
	my updating = false;
	showDetail (me);
}
void tabChanged (BatchWindow me, GuiRadioButtonEvent event) {
	if (my updating || my busy) return;
	render (me, event -> toggle == my filesTab ? Page::files : event -> toggle == my previewTab ? Page::preview : Page::results);
}
void settingsChanged (BatchWindow me) {
	if (my updating || my busy) return;
	my checkedFiles = false;
	if (my page == Page::files && ! my folder.empty())
		GuiLabel_setText (my summary, U"Check files to update the list for the current settings.");
	GuiLabel_setText (my presetName, U"Preset settings modified");
}
void textChanged (BatchWindow me, GuiTextEvent) { settingsChanged (me); }
void checkChanged (BatchWindow me, GuiCheckButtonEvent) { settingsChanged (me); }
void checkFiles (BatchWindow me, GuiButtonEvent) {
	perform (me, [&] {
		my pairs = scan (my folder, readControls (me));
		my checkedFiles = true;
		render (me, Page::files);
	});
}
void chooseFolder (BatchWindow me, GuiButtonEvent) {
	perform (me, [&] {
		const auto path = GuiFileSelect_getFolderName (my window, U"Choose recordings and TextGrids folder");
		if (! path) return;
		my folder = fs::u8path (Melder_peek32to8 (path.get()));
		labelValue (my folderName, U"", textOf (my folder.filename().empty() ? my folder.root_path() : my folder.filename()));
		my pairs.clear();
		my checkedFiles = false;
		render (me, Page::files);
		my pairs = scan (my folder, readControls (me));
		my checkedFiles = true;
		render (me, Page::files);
	});
}
void choosePreset (BatchWindow me, GuiButtonEvent) {
	perform (me, [&] {
		const auto chosen = GuiFileSelect_getInfileNames (my window, U"Open batch preset", false);
		if (! chosen -> size) return;
		const auto path = fs::u8path (Melder_peek32to8 (chosen -> at [1] -> string.get()));
		showControls (me, loadPreset (path), textOf (path.stem()));
		my pairs.clear();
		my checkedFiles = false;
		render (me, Page::files);
		if (! my folder.empty()) {
			my pairs = scan (my folder, readControls (me));
			my checkedFiles = true;
			render (me, Page::files);
		}
	});
}
void savePresetAs (BatchWindow me, GuiButtonEvent) {
	perform (me, [&] {
		const auto preset = readControls (me);
		const auto chosen = GuiFileSelect_getOutfileName (my window, U"Save batch preset", U"Vowel midpoints.PraatBatchPreset");
		if (! chosen) return;
		auto path = fs::u8path (Melder_peek32to8 (chosen.get()));
		const bool exact = Melder_endsWith_caseAware (chosen.get(), U".PraatBatchPreset");
		if (! exact) path += ".PraatBatchPreset";
		savePreset (preset, path, exact);
		labelValue (my presetName, U"", textOf (path.stem()));
	});
}
void start (BatchWindow me, bool preview) {
	perform (me, [&] {
		const auto preset = readControls (me);
		std::vector<std::pair<fs::path, fs::path>> selectedFiles;
		if (preview && my page == Page::files) {
			const auto selection = GuiList_getSelectedPositions (my rows);
			for (integer i = 1; i <= selection.size; ++ i) {
				const auto &pair = my pairs [selection [i] - 1];
				selectedFiles.emplace_back (pair.audio, pair.grid);
			}
		}
		my pairs = scan (my folder, preset);
		my checkedFiles = true;
		render (me, Page::files);
		std::vector<integer> indices;
		for (integer i = 0; i < (integer) my pairs.size(); ++ i) {
			const auto &pair = my pairs [i];
			if (! pair.problem.empty()) continue;
			if (selectedFiles.empty() || std::find (selectedFiles.begin(), selectedFiles.end(),
					std::make_pair (pair.audio, pair.grid)) != selectedFiles.end()) indices.push_back (i);
			if (preview && (integer) indices.size() == previewPairs) break;
		}
		Melder_require (! indices.empty(), U"No ready file pairs match the current settings and selection. Check the Files list.");
		Report report { my folder, preset, my pairs, run (my folder, preset, my pairs, indices), true };
		(preview ? my preview : my results) = std::move (report);
		render (me, preview ? Page::preview : Page::results);
	});
}
void previewButton (BatchWindow me, GuiButtonEvent) { start (me, true); }
void runButton (BatchWindow me, GuiButtonEvent) { start (me, false); }
void exportButton (BatchWindow me, GuiButtonEvent) {
	perform (me, [&] {
		if (! my results.exists) return;
		const auto path = GuiFileSelect_getFolderName (my window, U"Choose where to put the exported run folder");
		if (! path) return;
		const auto &report = my results;
		const auto saved = exportRun (fs::u8path (Melder_peek32to8 (path.get())), report.folder, report.preset, report.pairs, report.result);
		Melder_information (U"Batch analysis exported to:\n", textOf (saved).c_str());
	});
}
void closeWindow (BatchWindow me) { if (! my busy) GuiThing_hide (my window); }
void closeButton (BatchWindow me, GuiButtonEvent) { closeWindow (me); }

GuiText field (BatchWindow me, conststring32 label, int y) {
	GuiLabel_createShown (my window, 16, 275, y, y + 19, label, 0);
	auto control = GuiText_createShown (my window, 16, 275, y + 21, y + 48, 0);
	GuiText_setChangedCallback (control, textChanged, me);
	return control;
}
} // namespace

void BatchAnalysisWindow_show () {
	if (Melder_batch) return;
	if (! window) {
		auto me = Thing_new (BatchWindow);
		my window = GuiWindow_create (100, 80, 890, 615, 750, 615, U"Batch Analysis", closeWindow, me.get(), 0);
		GuiLabel_createShown (my window, 16, 275, 12, 35, U"Preset", GuiLabel_BOLD);
		my presetName = GuiLabel_createShown (my window, 16, 275, 38, 59, U"Vowel midpoints", 0);
		GuiButton_createShown (my window, 16, 138, 63, 92, U"Load preset…", choosePreset, me.get(), 0);
		GuiButton_createShown (my window, 145, 275, 63, 92, U"Save preset as…", savePresetAs, me.get(), 0);
		GuiLabel_createShown (my window, 16, 275, 107, 128, U"Input folder", GuiLabel_BOLD);
		my folderName = GuiLabel_createShown (my window, 16, 275, 132, 152, U"No folder selected", 0);
		GuiButton_createShown (my window, 16, 138, 156, 185, U"Choose folder…", chooseFolder, me.get(), 0);
		GuiButton_createShown (my window, 145, 275, 156, 185, U"Check files", checkFiles, me.get(), 0);
		my tier = field (me.get(), U"Interval tier name", 200);
		my label = field (me.get(), U"Exact label (optional)", 255);
		GuiLabel_createShown (my window, 16, 275, 312, 333, U"Measurements", GuiLabel_BOLD);
		my duration = GuiCheckButton_createShown (my window, 16, 275, 337, 359, U"Duration", checkChanged, me.get(), 0);
		my formants = GuiCheckButton_createShown (my window, 16, 275, 361, 383, U"Midpoint F1 and F2 (Burg)", checkChanged, me.get(), 0);
		my ceiling = field (me.get(), U"Formant ceiling (Hz)", 390);
		my pitch = GuiCheckButton_createShown (my window, 16, 275, 447, 469, U"Median voiced F0 (Hz)", checkChanged, me.get(), 0);
		GuiLabel_createShown (my window, 16, 138, 475, 495, U"Pitch floor (Hz)", 0);
		my floor = GuiText_createShown (my window, 16, 138, 498, 524, 0);
		GuiText_setChangedCallback (my floor, textChanged, me.get());
		GuiLabel_createShown (my window, 145, 275, 475, 495, U"Pitch top (Hz)", 0);
		my top = GuiText_createShown (my window, 145, 275, 498, 524, 0);
		GuiText_setChangedCallback (my top, textChanged, me.get());
		GuiRadioGroup_begin ();
		my filesTab = GuiRadioButton_createShown (my window, 300, 380, 14, 42, U"Files", tabChanged, me.get(), GuiRadioButton_SET);
		my previewTab = GuiRadioButton_createShown (my window, 390, 485, 14, 42, U"Preview", tabChanged, me.get(), 0);
		my resultsTab = GuiRadioButton_createShown (my window, 495, 590, 14, 42, U"Results", tabChanged, me.get(), 0);
		GuiRadioGroup_end ();
		my summary = GuiLabel_createShown (my window, 300, -16, 53, 76, U"", 0);
		my rows = GuiList_createShown (my window, 300, -16, 82, -155, true, U"Files and measurements");
		GuiList_setSelectionChangedCallback (my rows, selectionChanged, me.get());
		my detail = GuiText_createShown (my window, 300, -16, -145, -68, GuiText_SCROLLED | GuiText_ANYWRAP | GuiText_NONEDITABLE);
		GuiButton_createShown (my window, 300, 423, -49, -17, U"Preview files", previewButton, me.get(), 0);
		GuiButton_createShown (my window, 432, 545, -49, -17, U"Run analysis", runButton, me.get(), 0);
		my exportButton = GuiButton_createShown (my window, 554, 685, -49, -17, U"Export run…", exportButton, me.get(), 0);
		GuiButton_createShown (my window, -83, -16, -49, -17, U"Close", closeButton, me.get(), GuiButton_CANCEL);
		showControls (me.get(), Preset {}, U"Vowel midpoints");
		render (me.get(), Page::files);
		window = me.releaseToAmbiguousOwner ();
	}
	GuiThing_show (window -> window);
}
