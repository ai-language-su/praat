/* TextGridEditor.cpp
 *
 * Copyright (C) 1992-2022 Paul Boersma
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this work. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TextGridEditor.h"
#include "EditorM.h"
#include <algorithm>

Thing_implement (TextGridEditor, FunctionEditor, 0);

#include "Prefs_define.h"
#include "TextGridEditor_prefs.h"
#include "Prefs_install.h"
#include "TextGridEditor_prefs.h"
#include "Prefs_copyToInstance.h"
#include "TextGridEditor_prefs.h"

static void menu_cb_TextGridEditorHelp (TextGridEditor, EDITOR_ARGS) { HELP (U"TextGridEditor") }
static void menu_cb_AboutSpecialSymbols (TextGridEditor, EDITOR_ARGS) { HELP (U"Special symbols") }
static void menu_cb_PhoneticSymbols (TextGridEditor, EDITOR_ARGS) { HELP (U"Phonetic symbols") }
static void menu_cb_AboutTextStyles (TextGridEditor, EDITOR_ARGS) { HELP (U"Text styles") }

void structTextGridEditor :: v_createMenuItems_help (EditorMenu menu) {
	TextGridEditor_Parent :: v_createMenuItems_help (menu);
	EditorMenu_addCommand (menu, U"TextGridEditor help", '?', menu_cb_TextGridEditorHelp);
	EditorMenu_addCommand (menu, U"About special symbols", 0, menu_cb_AboutSpecialSymbols);
	EditorMenu_addCommand (menu, U"Phonetic symbols", 0, menu_cb_PhoneticSymbols);
	EditorMenu_addCommand (menu, U"About text styles", 0, menu_cb_AboutTextStyles);
}

static void menu_cb_LoadAnnotationTemplate (TextGridEditor me, EDITOR_ARGS) {
	const auto files = GuiFileSelect_getInfileNames (my windowForm, U"Load annotation template", false);
	if (! files -> size) return;
	try {
		my annotationTemplate = CorpusAnnotations::readTemplate (
			std::filesystem::u8path (Melder_peek32to8 (files -> at [1] -> string.get())));
	} catch (const std::exception &error) {
		Melder_throw (U"Could not load the annotation template: ", Melder_peek8to32_u (error.what()));
	}
}

static auto vocabularyFor (TextGridEditor me, Function tier) {
	const auto name = tier -> name ? tier -> name.get() : U"";
	return std::find_if (my annotationTemplate.begin(), my annotationTemplate.end(), [&] (const auto &candidate) {
		return candidate.name == name && candidate.point == (tier -> classInfo == classTextTier);
	});
}

static void applyAllowedLabel (UiForm, integer, Stackel, conststring32, Interpreter, conststring32,
	bool, void *closure, Editor)
{
	const auto me = static_cast<TextGridEditor> (closure);
	const auto area = my textGridArea().get();
	const auto tierNumber = area -> selectedTier;
	Melder_require (tierNumber >= 1 && tierNumber <= my textGrid() -> tiers -> size &&
			tierNumber == my templateLabelTier && my startSelection == my templateLabelTime,
		U"The annotation selection changed. Choose an allowed label again.");
	const auto tier = my textGrid() -> tiers -> at [tierNumber];
	const auto label = my templateLabelChoices [my templateLabelIndex].get();
	const auto entry = vocabularyFor (me, tier);
	Melder_require (entry != my annotationTemplate.end() && std::find (entry -> labels.begin(), entry -> labels.end(), label) != entry -> labels.end(),
		U"The tier or template changed. Choose an allowed label again.");
	if (tier -> classInfo == classIntervalTier) {
		const auto interval = IntervalTier_timeToIndex (static_cast<IntervalTier> (tier), my startSelection);
		Melder_require (interval, U"Select an interval first.");
		Editor_save (me, U"Set allowed label");
		TextGrid_setIntervalText (my textGrid(), tierNumber, interval, label);
	} else {
		const auto point = AnyTier_hasPoint (static_cast<TextTier> (tier) -> asAnyTier(), my startSelection);
		Melder_require (point, U"Select an existing point first.");
		Editor_save (me, U"Set allowed label");
		TextGrid_setPointText (my textGrid(), tierNumber, point, label);
	}
	FunctionArea_broadcastDataChanged (area);
	my v_updateText ();
}

static void menu_cb_ChooseAllowedLabel (TextGridEditor me, EDITOR_ARGS) {
	Melder_require (! my annotationTemplate.empty(), U"Load an annotation template from the Labels menu first.");
	const auto tierNumber = my textGridArea() -> selectedTier;
	Melder_require (tierNumber >= 1 && tierNumber <= my textGrid() -> tiers -> size, U"Select an annotation tier first.");
	const auto tier = my textGrid() -> tiers -> at [tierNumber];
	const auto entry = vocabularyFor (me, tier);
	Melder_require (entry != my annotationTemplate.end() && ! entry -> labels.empty(), U"The template has no allowed-label list for this tier.");
	if (entry -> point)
		Melder_require (AnyTier_hasPoint (static_cast<TextTier> (tier) -> asAnyTier(), my startSelection), U"Select an existing point first.");
	cmd -> d_uiform.reset();
	my templateLabelChoices = autoSTRVEC ((integer) entry -> labels.size());
	for (integer i = 1; i <= my templateLabelChoices.size; ++ i)
		my templateLabelChoices [i] = Melder_dup (entry -> labels [i - 1].c_str());
	my templateLabelTier = tierNumber;
	my templateLabelTime = my startSelection;
	cmd -> d_uiform = UiForm_create (my windowForm, me, U"Choose allowed label", applyAllowedLabel, me, cmd -> itemTitle.get(), nullptr);
	UiForm_addList (cmd -> d_uiform.get(), & my templateLabelIndex, nullptr, U"label", U"Label", my templateLabelChoices.get(), 1);
	UiForm_finish (cmd -> d_uiform.get());
	UiForm_do (cmd -> d_uiform.get(), false);
}

void structTextGridEditor :: v_createMenus () {
	TextGridEditor_Parent :: v_createMenus ();
	const auto menu = Editor_addMenu (this, U"Labels", 0);
	EditorMenu_addCommand (menu, U"Load annotation template...", 0, menu_cb_LoadAnnotationTemplate);
	EditorMenu_addCommand (menu, U"Choose allowed label...", 0, menu_cb_ChooseAllowedLabel);
}

autoTextGridEditor TextGridEditor_create (conststring32 title, TextGrid textGrid,
	SampledXY optionalSoundOrLongSound, SpellingChecker spellingChecker, conststring32 callbackSocket)
{
	try {
		autoTextGridEditor me = Thing_new (TextGridEditor);
		my textGridArea() = TextGridArea_create (true, nullptr, me.get());
		if (optionalSoundOrLongSound) {
			Melder_assert (optionalSoundOrLongSound -> ny > 0);
			if (Thing_isa (optionalSoundOrLongSound, classSound))
				my soundArea() = SoundArea_create (false, static_cast <Sound> (optionalSoundOrLongSound), me.get());
			else
				my soundArea() = LongSoundArea_create (false, static_cast <LongSound> (optionalSoundOrLongSound), me.get());
			my soundAnalysisArea() = SoundAnalysisArea_create (false, nullptr, me.get());
			my textGridArea() -> borrowedSoundArea = my soundArea().get();
			my textGridArea() -> borrowedSoundAnalysisArea = my soundAnalysisArea().get();
		}
		my textGridArea() -> spellingChecker = spellingChecker;
		my callbackSocket = Melder_dup (callbackSocket);
		FunctionEditor_init (me.get(), title, textGrid);

		Melder_assert (isdefined (my startSelection));   // precondition of v_updateText()
		my v_updateText ();   // to reflect changed tier selection AND to get first text; BUG: should not be needed
		if (spellingChecker)
			GuiText_setSelection (my textArea, 0, 0);
		if (optionalSoundOrLongSound &&
			optionalSoundOrLongSound -> xmin == 0.0 &&
			my textGrid() -> xmin != 0.0 &&
			my textGrid() -> xmax > optionalSoundOrLongSound -> xmax
		)
			Melder_warning (U"The time domain of the TextGrid (starting at ",
				Melder_fixed (my textGrid() -> xmin, 6), U" seconds) does not overlap with that of the sound "
				U"(which starts at 0 seconds).\nIf you want to repair this, you can select the TextGrid "
				U"and choose “Shift times to...” from the Modify menu "
				U"to shift the starting time of the TextGrid to zero.");
		return me;
	} catch (MelderError) {
		Melder_throw (U"TextGrid window not created.");
	}
}

/* End of file TextGridEditor.cpp */
