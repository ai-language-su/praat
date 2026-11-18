/* InfoEditor.cpp
 *
 * Copyright (C) 2004-2011,2012,2013,2015,2017 Paul Boersma
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

#include "InfoEditor.h"
#include "praatP.h"
#include "Ltas.h"
#include <utility>

Thing_implement (InfoEditor, TextEditor, 0);

static InfoEditor theReferenceToTheOnlyInfoEditor;
static bool nextOutputIsBinCount;

void InfoEditor_refreshBinCount () {
	const auto editor = theReferenceToTheOnlyInfoEditor;
	if (! editor || ! editor -> showingBinCount || ! GuiCheckButton_getValue (editor -> followSelection) ||
			Melder_backgrounding || theCurrentPraatObjects != & theForegroundPraatObjects) return;
	if (theCurrentPraatObjects -> totalSelection == 1) {
		for (integer i = 1; i <= theCurrentPraatObjects -> n; ++ i) {
			const auto &entry = theCurrentPraatObjects -> list [i];
			if (! entry.isSelected) continue;
			if (entry.klas == classSpectrum || entry.klas == classLtas) {
				const auto count = static_cast<Sampled> (entry.object) -> nx;
				GuiText_setString (editor -> textWidget,
					Melder_cat (entry.id, U". ", entry.name.get(), U"\n\n", count, count == 1 ? U" bin\n" : U" bins\n"), false);
				return;
			}
			break;
		}
	}
	GuiText_setString (editor -> textWidget, U"Select one Spectrum or LTAS object to see its bin count.", false);
}

static void followChanged (InfoEditor, GuiCheckButtonEvent) { InfoEditor_refreshBinCount (); }

void structInfoEditor :: v_createChildren () {
	InfoEditor_Parent :: v_createChildren ();
	our followSelection = GuiCheckButton_create (our windowForm, 8, -8, -30, -4, U"Follow selected object", followChanged, this, 0);
}

void InfoEditor_reportBinCount (integer count) {
	nextOutputIsBinCount = true;
	try {
		Melder_information (count, U" bins");
	} catch (...) {
		nextOutputIsBinCount = false;
		throw;
	}
	nextOutputIsBinCount = false;
}

void structInfoEditor :: v9_destroy () noexcept {
	theReferenceToTheOnlyInfoEditor = nullptr;   // undangle
	InfoEditor_Parent :: v9_destroy ();
}

void structInfoEditor :: v_clear () {
	Melder_clearInfo ();
}

InfoEditor InfoEditor_getTheReferenceToTheOnlyInstance () {
	if (! theReferenceToTheOnlyInfoEditor) {
		autoInfoEditor editor = Thing_new (InfoEditor);
		TextEditor_init (editor.get(), U"");
		Thing_setName (editor.get(), U"Praat Info");
		theReferenceToTheOnlyInfoEditor = editor.get();
		editor.releaseToUser();
	}
	return theReferenceToTheOnlyInfoEditor;
}

static void gui_information (conststring32 message) {
	const bool binCount = std::exchange (nextOutputIsBinCount, false);
	InfoEditor editor = InfoEditor_getTheReferenceToTheOnlyInstance ();
	#if cocoa
		/*
			REPAIRED MEMORY LEAK
				2019-10-31
			As we may be called in long sequences outside of the event loop,
			we may need to clean up. Without the autorelease pool,
			the retainCount of the cocoaTextView will quickly rise,
			as shown by running the following script:
				for i to 1e6
					writeInfoLine: i
				endfor
			and waiting for Praat to crash after taking 3 GB of memory (macOS 10.15).
			Also, if you stop before crashing (e.g. have the loop run to only 1e5),
			then much of the memory will not be released at all.
		*/
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		//Melder_casual (U"cocoaTextView retain count before: ", [editor -> textWidget -> d_cocoaTextView  retainCount]);
	#endif
	editor -> showingBinCount = binCount;
	if (binCount) {
		GuiThing_show (editor -> followSelection);
	} else {
		GuiCheckButton_setValue (editor -> followSelection, false);
		GuiThing_hide (editor -> followSelection);
	}
	if (binCount && GuiCheckButton_getValue (editor -> followSelection))
		InfoEditor_refreshBinCount ();
	else
		GuiText_setString (editor -> textWidget, message, false);
	GuiThing_show (editor -> windowForm);
	/*
		Try to make sure that the invalidated text widget and the elements of the fronted window are
		redrawn before the next event.

		The following Praat script can test this:

		writeInfoLine: "hoi"
		for i to 100
			appendInfoLine: i
		endfor

		The Info window should scroll continuously while the lines are added,
		not just show the end result.
	*/
	#if cocoa
		/*
			TODO: This code is very similar to that of GuiShell_drain (editor -> windowForm, false, false);
			perhaps it can be merged at some point.
		*/
		[editor -> windowForm -> d_cocoaShell   displayIfNeeded];
		/*
			Just calling `displayIfNeeded` doesn't suffice for redrawing.
			We have to poke the run loop a little, which can be achieved
			by calling `nextEventMatchingMask` with `dequeue: YES`,
			a combination that potentially progresses the run loop by one iteration.
			This effect, even if not realized, forces pending redraws to be executed to the screen.

			Let's use the opportunity to throw away any mouse clicks and key strokes.
		*/
		NSEvent *nsEvent;
		while ((nsEvent = [NSApp
			nextEventMatchingMask: NSEventMaskAny
			untilDate: [NSDate distantPast]
			inMode: NSDefaultRunLoopMode
			dequeue: YES]) != nullptr)
		{
			NSUInteger nsEventType = [nsEvent type];
			if (nsEventType == NSEventTypeKeyDown)
				NSBeep();
			/*
				Ignore all other events.
			*/
		}
		[pool release];
	#endif
}

void InfoEditor_injectInformationProc () {
	Melder_setInformationProc (gui_information);
}

/* End of file InfoEditor.cpp */
