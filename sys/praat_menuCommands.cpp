/* praat_menuCommands.cpp
 *
 * Copyright (C) 1992-2018,2020-2025 Paul Boersma
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
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

#include "praatP.h"
#include "praat_script.h"
#include "GuiP.h"
#include "praat_tooltip.h"

static OrderedOf <structPraat_Command> theCommands;
void praat_menuCommands_exit_optimizeByLeaking () { theCommands. _ownItems = false; }

void praat_sortMenuCommands () {
	for (integer i = 1; i <= theCommands.size; i ++) {
		Praat_Command command = theCommands.at [i];
		command -> sortingTail = i;
	}
	std::sort (theCommands.begin(), theCommands.end(),
		[] (Praat_Command me, Praat_Command thee) {
			if (my window) {
				if (! thy window)
					return false;
				int compare = str32cmp (my window.get(), thy window.get());
				if (compare != 0)
					return compare < 0;
			} else if (thy window)
				return true;
			if (my menu) {
				if (! thy menu)
					return false;
				int compare = str32cmp (my menu.get(), thy menu.get());
				if (compare != 0)
					return compare < 0;
			} else if (thy menu)
				return true;
			return my sortingTail < thy sortingTail;
		}
	);
}

static integer lookUpMatchingMenuCommand_0 (conststring32 window, conststring32 menu, conststring32 title) {
	/*
		A menu command is fully specified by its environment (window + menu) and its title.
	*/
	for (integer i = 1; i <= theCommands.size; i ++) {
		Praat_Command command = theCommands.at [i];
		conststring32 tryWindow = command -> window.get();
		conststring32 tryMenu = command -> menu.get();
		conststring32 tryTitle = command -> title.get();
		if (
			(window == tryWindow || (window && tryWindow && str32equ (window, tryWindow))) &&
			(menu == tryMenu || (menu && tryMenu && str32equ (menu, tryMenu))) &&
			(title == tryTitle || (title && tryTitle && str32equ (title, tryTitle)))
		)
			return i;
	}
	return 0;   // not found
}

static void do_menu (Praat_Command me, bool isModified) {
	if (my callback == DO_RunTheScriptFromAnyAddedMenuCommand) {
		UiHistory_write (U"\nrunScript: ");
		try {
			DO_RunTheScriptFromAnyAddedMenuCommand (nullptr, 0, nullptr, my script.get(), nullptr, nullptr, false, nullptr, nullptr);
		} catch (MelderError) {
			Melder_flushError (U"Command \"", my title.get(), U"\" not executed.");
		}
		praat_updateSelection ();
	} else {
		if (my title && ! str32str (my title.get(), U"...")) {
			UiHistory_write (U"\n");
			UiHistory_write (my title.get());
		}
		try {
			my callback (nullptr, 0, nullptr, nullptr, nullptr, my title.get(), isModified, nullptr, nullptr);
		} catch (MelderError) {
			Melder_flushError (U"Command \"", my title.get(), U"\" not executed.");
		}
		praat_updateSelection ();
	}
}

static void gui_button_cb_menu (Praat_Command me, GuiButtonEvent event) {
	const bool isModified = event -> shiftKeyPressed || event -> commandKeyPressed || event -> optionKeyPressed;
	do_menu (me, isModified);
}

static void gui_cb_menu (Praat_Command me, GuiMenuItemEvent event) {
	const bool isModified = event -> shiftKeyPressed || event -> commandKeyPressed || event -> optionKeyPressed;
	do_menu (me, isModified);
}

static GuiMenu windowMenuToWidget (conststring32 window, conststring32 menu) {
	return
		str32equ (window, U"Picture") ? praat_picture_resolveMenu (menu) :
		str32equ (window, U"Objects") ? praat_objects_resolveMenu (menu) : nullptr;
}

static GuiMenuItem praat_addMenuCommand__ (conststring32 window, conststring32 menu, conststring32 title /* cattable */,
	conststring32 after, uint32 flags, UiCallback callback, conststring32 nameOfCallback)
{
	uint32 depth = flags, key = 0;
	bool unhidable = false, hidden = false, noApi = false, forceApi = false;
	int deprecationYear = 0;
	uint32 guiFlags = 0;
	if (flags > 7) {
		depth = (flags & GuiMenu_DEPTH_7) >> 16;
		unhidable = (flags & GuiMenu_UNHIDABLE) != 0;
		hidden = (flags & GuiMenu_HIDDEN) != 0 && ! unhidable;
		key = flags & 0x000000FF;
		noApi = (flags & GuiMenu_NO_API) != 0;
		forceApi = (flags & GuiMenu_FORCE_API) != 0;
		deprecationYear = (flags & GuiMenu_DEPRECATED) == GuiMenu_DEPRECATED ? 2000 + (flags >> 24) : 0;
		guiFlags = key ? flags & (0x000000FF | GuiMenu_SHIFT | GuiMenu_OPTION | GuiMenu_BUTTON_STATE_MASK) : flags & GuiMenu_BUTTON_STATE_MASK;
	}
	if (callback && ! title) {
		Melder_flushError (U"praat_addMenuCommand: command with callback has no title. Window \"", window, U"\", menu \"", menu, U"\".");
		return nullptr;
	}

	/*
		Determine the position of the new command.
	*/
	integer position;
	if (after) {   // search for existing command with same selection
		const integer found = lookUpMatchingMenuCommand_0 (window, menu, after);
		if (found == 0) {
			Melder_flushError (U"praat_addMenuCommand: the command \"", title, U"\" cannot be put after \"", after, U"\",\n"
				U"in the menu \"", menu, U"\" in the window \"", window, U"\"\n"
				U"because the latter command does not exist.");
			return nullptr;
		}
		position = found + 1;   // after 'after'
	} else {
		position = theCommands.size + 1;   // at end
	}

	/*
		Make new command.
	*/
	autoPraat_Command command = Thing_new (Praat_Command);
	command -> window = Melder_dup_f (window);
	command -> menu = Melder_dup_f (menu);
	command -> title = Melder_dup_f (title);
	trace (U"insert new command \"", title, U"\"");
	command -> depth = depth;
	command -> callback = callback;   // null for a separator or cascade button
	command -> nameOfCallback = nameOfCallback;
	command -> executable = !! callback;
	command -> script = autostring32();
	command -> hidden = hidden;
	command -> unhidable = unhidable;
	command -> deprecationYear = deprecationYear;
	command -> noApi = noApi;
	command -> forceApi = forceApi;

	if (! theCurrentPraatApplication -> batch) {
		GuiMenu parentMenu = nullptr;

		/*
			WHERE TO PUT IT?
			Determine parent menu widget.
			This is not going to fail:
			if 'depth' is inappropriate, the alleged subitem will be put in the top menu.
		*/
		if (depth == 0) {
			/*
				Put the new command in the window's top menu.
			*/
			parentMenu = windowMenuToWidget (window, menu);
		} else {
			/*
				Put the new command in a submenu.
				The supermenu to put the new command in is the first menu that we find when going up.
			*/
			for (integer parentPosition = position - 1; parentPosition > 0; parentPosition --) {
				Praat_Command parentCommand = theCommands.at [parentPosition];
				if (parentCommand -> depth == depth - 1 && str32equ (parentCommand -> menu.get(), command -> menu.get())) {
					/*
						We found the supermenu.
					*/
					if (! parentCommand -> callback && parentCommand -> title && parentCommand -> title [0] != U'-') {
						if (! parentCommand -> button)
							Melder_crash (U"No button for ", window, U"/", menu, U"/", title, U".");
						Thing_cast (GuiMenuItem, parentButton_as_GuiMenuItem, parentCommand -> button);
						parentMenu = parentButton_as_GuiMenuItem -> d_menu;
					}
					break;
				}
			}
			if (! parentMenu)
				parentMenu = windowMenuToWidget (window, menu);   // fallback: put the command in the window's top menu
		}
		if (! parentMenu) {
			trace (U"WARNING: no parent menu for ", window, U"/", menu, U"/", title, U".");
			return nullptr;
		}

		/*
			WHAT TO PUT THERE?
		*/
		if (! title || title [0] == U'-') {
			trace (U"insert the command as a separator");
			command -> button = GuiMenu_addSeparator (parentMenu);
			Melder_assert (command -> button);
		} else if (! callback) {
			trace (U"insert the command as a submenu");
			command -> button = GuiMenu_createInMenu (parentMenu, title, 0) -> d_menuItem.get();
			Melder_assert (command -> button);
		} else {
			trace (U"insert the command as a normal menu item");
			command -> button = GuiMenu_addItem (parentMenu, title, guiFlags, gui_cb_menu, command.get());
			Melder_assert (command -> button);
		}
		if (hidden)
			GuiThing_hide (command -> button);
	}
	Thing_cast (GuiMenuItem, button_as_GuiMenuItem, command -> button);
	theCommands. addItemAtPosition_move (command.move(), position);
	return button_as_GuiMenuItem;
}
GuiMenuItem praat_addMenuCommand_ (conststring32 window, conststring32 menu, conststring32 title /* cattable */,
	conststring32 after, uint32 flags, UiCallback callback, conststring32 nameOfCallback)
{
	const char32 *pSeparator = str32str (title, U" || ");
	if (! pSeparator)
		return praat_addMenuCommand__ (window, menu, title, after, flags, callback, nameOfCallback);
	if (flags < 8)
		flags *= GuiMenu_DEPTH_1;   // turn 1..7 into GuiMenu_DEPTH_1..GuiMenu_DEPTH_7, because the flags are ORed below
	integer positionOfSeparator = pSeparator - title;
	static MelderString string;
	MelderString_copy (& string, title);
	char32 *pTitle = & string.string [0];
	GuiMenuItem result = nullptr;
	do {
		pTitle [positionOfSeparator] = U'\0';
		GuiMenuItem menuItem = praat_addMenuCommand__ (window, menu, pTitle, after, flags, callback, nameOfCallback);
		if (menuItem)
			result = menuItem;   // only the first
		pTitle += positionOfSeparator + 4;   // step past " || "
		pSeparator = str32str (pTitle, U" || ");
		if (pSeparator)
			positionOfSeparator = pSeparator - pTitle;
		flags |= GuiMenu_HIDDEN;
	} while (pSeparator);
	(void) praat_addMenuCommand__ (window, menu, pTitle, after, flags | GuiMenu_HIDDEN, callback, nameOfCallback);
	return result;
}

void praat_addMenuCommandScript (conststring32 window, conststring32 menu, conststring32 title,
	conststring32 after, integer depth, conststring32 script)
{
	try {
		Melder_assert (window && menu && title && after && script);
		if (script [0] != U'\0' && title [0] == U'\0')
			Melder_throw (U"Command with script has no title. Window \"", window, U"\", menu \"", menu, U"\".");

		/*
			Determine the position of the new command.
		*/
		integer position;
		if (Melder_length (after)) {   // search for existing command with same selection
			const integer found = lookUpMatchingMenuCommand_0 (window, menu, after);
			if (found == 0) {
				/*Melder_throw (U"The menu command \"", title, U"\" cannot be put after \"", after, U"\",\n"
					U"in the menu \"", menu, "\" in the window \"", window, U"\"\n"
					U"because the latter command does not exist.");*/
				position = theCommands.size + 1;   // default: at end
			} else
				position = found + 1;   // after 'after'
		} else {
			position = theCommands.size + 1;   // at end
		}

		/*
			Make new command.
		*/
		autoPraat_Command command = Thing_new (Praat_Command);
		command -> window = Melder_dup_f (window);
		command -> menu = Melder_dup_f (menu);
		command -> title = ( title [0] != U'\0' ? Melder_dup_f (title) : autostring32() );   // allow old-fashioned untitled separators
		command -> depth = depth;
		command -> callback = ( script [0] != U'\0' ? DO_RunTheScriptFromAnyAddedMenuCommand : nullptr );   // null for a separator or cascade button
		command -> executable = ( script [0] != U'\0' );
		command -> noApi = true;
		if (script [0] == U'\0') {
			command -> script = Melder_dup_f (U"");   // empty string, which will be needed to signal origin
		} else {
			structMelderFile file { };
			Melder_relativePathToFile (script, & file);
			command -> script = Melder_dup_f (MelderFile_peekPath (& file));
		}
		command -> after = ( after [0] != U'\0' ? Melder_dup_f (after) : autostring32() );
		if (praatP.phase >= praat_READING_BUTTONS) {
			static integer uniqueID = 0;
			command -> uniqueID = ++ uniqueID;
		}

		if (! theCurrentPraatApplication -> batch) {
			GuiMenu parentMenu = nullptr;

			/*
				WHERE TO PUT IT?
				Determine parent menu widget.
				This is not going to fail:
				if 'depth' is inappropriate, the alleged subitem will be put in the top menu.
			*/
			if (depth == 0) {
				parentMenu = windowMenuToWidget (window, menu);   // not a subitem: in the top menu
			} else {
				for (integer parentPosition = position - 1; parentPosition > 0; parentPosition --) {
					Praat_Command parentCommand = theCommands.at [parentPosition];
					if (parentCommand -> depth == depth - 1) {
						if (! parentCommand -> callback && parentCommand -> title && parentCommand -> title [0] != U'-') {
							if (! parentCommand -> button)
								Melder_crash (U"No button for ", window, U"/", menu, U"/", title, U".");
							Melder_assert (parentCommand -> button -> classInfo == classGuiMenuItem);
							parentMenu = (static_cast <GuiMenuItem> (parentCommand -> button)) -> d_menu;
						}
						break;
					}
				}
				if (! parentMenu)
					parentMenu = windowMenuToWidget (window, menu);   // fallback: a subitem without a menu title
			}
			if (parentMenu) {
				/*
					WHAT TO PUT THERE?
				*/
				if (title [0] == U'\0' || title [0] == U'-') {
					command -> button = GuiMenu_addSeparator (parentMenu);
				} else if (script [0] == U'\0') {
					command -> button = GuiMenu_createInMenu (parentMenu, Melder_cat (U"\u207A", title), 0) -> d_menuItem.get();
				} else {
					command -> button = GuiMenu_addItem (parentMenu, Melder_cat (U"\u207A", title), 0, gui_cb_menu, command.get());
				}
			}
		}
		theCommands. addItemAtPosition_move (command.move(), position);

		if (praatP.phase >= praat_HANDLING_EVENTS)
			praat_sortMenuCommands ();
	} catch (MelderError) {
		Melder_throw (U"Script menu command not added.");
	}
}

void praat_hideMenuCommand (conststring32 window, conststring32 menu, conststring32 title) {
	if (theCurrentPraatApplication -> batch || ! window || ! menu || ! title)
		return;
	const integer found = lookUpMatchingMenuCommand_0 (window, menu, title);
	if (found == 0)
		return;
	Praat_Command command = theCommands.at [found];
	if (! command -> hidden && ! command -> unhidable) {
		command -> hidden = true;
		if (praatP.phase >= praat_READING_BUTTONS)
			command -> toggled = ! command -> toggled;
		if (command -> button)
			GuiThing_hide (command -> button);
	}
}

void praat_showMenuCommand (conststring32 window, conststring32 menu, conststring32 title) {
	if (theCurrentPraatApplication -> batch || ! window || ! menu || ! title)
		return;
	const integer found = lookUpMatchingMenuCommand_0 (window, menu, title);
	if (found == 0)
		return;
	Praat_Command command = theCommands.at [found];
	if (command -> hidden) {
		command -> hidden = false;
		if (praatP.phase >= praat_READING_BUTTONS)
			command -> toggled = ! command -> toggled;
		if (command -> button)
			GuiThing_show (command -> button);
	}
}

void praat_saveAddedMenuCommands (MelderString *buffer) {
	/*
		The procedure as it is now, runs in M*N time,
		where M is the number of added commands and N is the total number of commands.
		Sorting first instead runs in N log N time and will therefore be faster if M >> log N ≈ 10.
	*/
	integer maxID = 0;
	for (integer i = 1; i <= theCommands.size; i ++) {
		Praat_Command command = theCommands.at [i];
		if (command -> uniqueID > maxID)
			maxID = command -> uniqueID;
	}
	for (integer id = 1; id <= maxID; id ++)   // sorted
		for (integer i = 1; i <= theCommands.size; i ++) {
			Praat_Command me = theCommands.at [i];
			if (my uniqueID == id && ! my hidden && my window && my menu && my title) {
				MelderString_append (buffer, U"Add menu command... \"", my window.get(), U"\" \"", my menu.get(), U"\" \"", my title.get(), U"\" \"",
					( my after ? my after.get() : U"" ), U"\" ", my depth, U" ", ( my script ? my script.get() : U"" ), U"\n");
				break;
			}
		}
}

void praat_saveToggledMenuCommands (MelderString *buffer) {
	for (integer i = 1; i <= theCommands.size; i ++) {
		Praat_Command me = theCommands.at [i];
		if (my toggled && my window && my menu && my title && ! my uniqueID && ! my script)
			MelderString_append (buffer, my hidden ? U"Hide" : U"Show", U" menu command... \"",
				my window.get(), U"\" \"", my menu.get(), U"\" ", my title.get(), U"\n");
	}
}

/***** FIXED BUTTONS *****/

void praat_addFixedButtonCommand_ (GuiForm parent, conststring32 title, UiCallback callback, conststring32 nameOfCallback, int x, int y) {
	autoPraat_Command me = Thing_new (Praat_Command);
	my window = Melder_dup_f (U"Objects");
	my title = Melder_dup_f (title);
	my callback = callback;
	my nameOfCallback = nameOfCallback;
	my unhidable = true;
	my noApi = ( str32equ (title, U"Inspect") );
	if (theCurrentPraatApplication -> batch) {
		my button = nullptr;
	} else {
		GuiThing button = my button = GuiButton_create (parent, x, x + 82, -y - Gui_PUSHBUTTON_HEIGHT, -y,
			title, gui_button_cb_menu, me.get(), 0);
		conststring32 summary =
			str32equ (title, U"Rename...") ? U"Change the name shown for the selected object. Its data stays the same." :
			str32equ (title, U"Copy...") ? U"Make a separate copy of the selected object. You can edit the copy without changing the original." :
			str32equ (title, U"Inspect") ? U"Look inside the selected object's stored data and structure. This is mainly useful for detailed inspection." :
			str32equ (title, U"Info") ? U"Show information about the selected object, such as its properties and measurements." :
			str32equ (title, U"Remove") ? U"Remove the selected object from this Objects list. This does not delete its original file from your computer." :
			U"Run this command on the selected object.";
		conststring32 guide =
			str32equ (title, U"Rename...") ? U"What happens in Praat? Praat changes the selected object's label in the Objects list. The sound, annotation, or measurements inside the object stay the same, and an original file on disk is not renamed.\nWhy use it? Clear object names help you track an original recording and the results you create from it.\nHow do I use it? Select one object, click Rename, type a new name, and confirm. Look for the same object under its new label." :
			str32equ (title, U"Copy...") ? U"What happens in Praat? Praat creates another object in the list with the same current contents. The original remains, so you can compare the two or change one while keeping the other. This is a copy in Praat's memory, not a saved disk file.\nWhy use it? Make a copy before experimenting with commands that modify an object.\nHow do I use it? Select an object, click Copy, give the copy a useful name, and select the copy before editing it." :
			str32equ (title, U"Inspect") ? U"What happens in Praat? Praat opens an inspector of the selected object's internal fields and stored values. This is a technical view of the object in memory, not the normal waveform or annotation editor.\nWhy use it? It helps when you need to examine underlying data or diagnose an unexpected result. For everyday listening or labeling, View & Edit is usually the relevant window.\nHow do I use it? Select an object, click Inspect, and expand fields to look deeper. Closing the inspector does not change the object." :
			str32equ (title, U"Info") ? U"What happens in Praat? Praat writes properties of the selected object in the Info window. For a Sound, this can include its duration and sampling information; other object types report properties relevant to them.\nWhy use it? The Info window gives you numbers and details before you choose an analysis or compare results. It does not open the object for editing.\nHow do I use it? Select an object and click Info. Read the values and units. You can copy the reported text if you need it elsewhere." :
			str32equ (title, U"Remove") ? U"What happens in Praat? Praat removes the selected object from its current Objects list. It does not delete the source file from your computer. Changes kept only in that object can be lost if you have not saved or exported them.\nWhy use it? Clear temporary results and keep the list easy to navigate.\nHow do I use it? Select the exact object and click Remove. Save work you need to keep before removing it." :
			U"What happens in Praat? This button acts on the selected object in the Objects list.\nHow do I use it? Select an object, click the button, and read any settings before confirming.";
		GuiButton_setTooltip ((GuiButton) button, Melder_cat (summary, U"\n____________________________\n",
			praat_boldTooltipQuestions (guide). c_str()));
		GuiThing_setSensitive (button, false);
		GuiThing_show (button);
	}
	my executable = false;
	theCommands. addItemAtPosition_move (me.move(), 0);
}

void praat_sensitivizeFixedButtonCommand (conststring32 title, bool sensitive) {
	Praat_Command commandFound = nullptr;
	for (integer i = 1; i <= theCommands.size; i ++) {
		Praat_Command command = theCommands.at [i];
		if (str32equ (command -> title.get(), title)) {
			commandFound = command;
			break;
		}
	}
	if (! commandFound)
		Melder_crash (U"Unkown fixed button <<", title, U">>");
	commandFound -> executable = sensitive;
	if (! theCurrentPraatApplication -> batch && ! Melder_backgrounding)
		GuiThing_setSensitive (commandFound -> button, sensitive);
}

int praat_doMenuCommand (conststring32 title, conststring32 arguments, Interpreter interpreter) {
	Praat_Command commandFound = nullptr;
	for (integer i = 1; i <= theCommands.size; i ++) {
		Praat_Command command = theCommands.at [i];
		if (command -> executable && str32equ (command -> title.get(), title) &&
			(str32equ (command -> window.get(), U"Objects") || str32equ (command -> window.get(), U"Picture")))
		{
			commandFound = command;
			break;
		}
	}
	if (! commandFound)
		return 0;
	if (commandFound -> callback == DO_RunTheScriptFromAnyAddedMenuCommand) {
		const conststring32 scriptPath = commandFound -> script.get();
		const conststring32 preferencesFolderPath = MelderFolder_peekPath (Melder_preferencesFolder());
		const bool scriptIsInPlugin = Melder_startsWith (scriptPath, preferencesFolderPath);
		Melder_throw (
			U"From a script you cannot directly call a menu command that calls another script. Use instead: \nrunScript: ",
			scriptIsInPlugin ? U"preferencesDirectory$ + " : U"",
			U"\"",
			scriptIsInPlugin ? scriptPath + Melder_length (preferencesFolderPath) : scriptPath,
			U"\"",
			arguments && arguments [0] ? U", " : U"",
			arguments && arguments [0] ? arguments : U"",
			U"\n"
		);
	}
	commandFound -> callback (nullptr, 0, nullptr, arguments, interpreter, title, false, nullptr, nullptr);
	return 1;
}

int praat_doMenuCommand (conststring32 title, integer narg, Stackel args, Interpreter interpreter) {
	Praat_Command commandFound = nullptr;
	for (integer i = 1; i <= theCommands.size; i ++) {
		Praat_Command command = theCommands.at [i];
		if (command -> executable && str32equ (command -> title.get(), title) &&
			(str32equ (command -> window.get(), U"Objects") || str32equ (command -> window.get(), U"Picture")))
		{
			commandFound = command;
			break;
		}
	}
	if (! commandFound)
		return 0;
	if (commandFound -> callback == DO_RunTheScriptFromAnyAddedMenuCommand) {
		const conststring32 scriptPath = commandFound -> script.get();
		const conststring32 preferencesFolderPath = MelderFolder_peekPath (Melder_preferencesFolder());
		const bool scriptIsInPlugin = Melder_startsWith (scriptPath, preferencesFolderPath);
		Melder_throw (
			U"From a script you cannot directly call a menu command that calls another script. Use instead: \nrunScript: ",
			scriptIsInPlugin ? U"preferencesDirectory$ + " : U"",
			U"\"",
			scriptIsInPlugin ? scriptPath + Melder_length (preferencesFolderPath) : scriptPath,
			U"\"",
			narg > 0 ? U", ..." : U"",
			U"\n"
		);
	}
	commandFound -> callback (nullptr, narg, args, nullptr, interpreter, title, false, nullptr, nullptr);
	return 1;
}

integer praat_getNumberOfMenuCommands () { return theCommands.size; }

Praat_Command praat_getMenuCommand (integer i)
	{ return i < 1 || i > theCommands.size ? nullptr : theCommands.at [i]; }

void praat_addCommandsToEditor (Editor me) {
	conststring32 windowClassName = my classInfo -> className;
	for (integer i = 1; i <= theCommands.size; i ++) {
		Praat_Command command = theCommands.at [i];
		if (str32equ (command -> window.get(), windowClassName))
			Editor_addCommandScript (me, command -> menu.get(), command -> title.get(),
					((uint32) command -> depth) << 16, command -> script.get());
	}
}

static bool commandIsToBeIncluded (Praat_Command command, bool deprecated, bool includeCreateAPI, bool includeReadAPI,
	bool includeRecordAPI, bool includePlayAPI, bool includeDrawAPI, bool includeHelpAPI, bool includeWindowAPI)
{
	const bool obsolete = ( deprecated && (command -> deprecationYear < Melder_appYear() - 10 || command -> deprecationYear < 2023) );
	const bool hiddenByDefault = ( command -> hidden != command -> toggled );
	const bool explicitlyHidden = hiddenByDefault && ! deprecated;
	const bool hidden = explicitlyHidden || ! command -> callback || command -> noApi || obsolete ||
		(! includeWindowAPI && Melder_nequ (command -> nameOfCallback, U"WINDOW_", 7)) ||
		(! includeHelpAPI && Melder_nequ (command -> nameOfCallback, U"HELP_", 5)) ||
		(! includeDrawAPI && Melder_nequ (command -> nameOfCallback, U"GRAPHICS_", 9)) ||
		(! includePlayAPI && Melder_nequ (command -> nameOfCallback, U"PLAY_", 5)) ||
		(! includeRecordAPI && Melder_nequ (command -> nameOfCallback, U"RECORD_", 7)) ||
		(! includeReadAPI && Melder_nequ (command -> nameOfCallback, U"READ_MULTIPLE__", 5)) ||
		(! includeReadAPI && Melder_nequ (command -> nameOfCallback, U"READ_ONE__", 10)) ||
		(! includeCreateAPI && Melder_nequ (command -> nameOfCallback, U"NEW1_", 5));
	return (command -> forceApi || ! hidden) && command -> callback != DO_RunTheScriptFromAnyAddedMenuCommand;
}

static bool commandHasFileNameArgument (Praat_Command command) {
	const bool hasFileNameArgument =
		Melder_nequ (command -> nameOfCallback, U"READ_ONE__", 10) ||
		Melder_nequ (command -> nameOfCallback, U"SAVE_", 5)
	;
	return hasFileNameArgument;
}

static conststring32 getReturnType (Praat_Command command) {
	const conststring32 returnType =
		Melder_nequ (command -> nameOfCallback, U"NEW1_", 5) ? U"PraatObject" :
		Melder_nequ (command -> nameOfCallback, U"READ_ONE__", 10) ? U"PraatObject" :
		Melder_nequ (command -> nameOfCallback, U"REAL_", 5) ? U"double" :
		Melder_nequ (command -> nameOfCallback, U"INTEGER_", 8) ? U"int64_t" :
		Melder_nequ (command -> nameOfCallback, U"STRING_", 7) ? U"char *" :
		Melder_nequ (command -> nameOfCallback, U"REPORT_", 7) ? U"char *" :
		Melder_nequ (command -> nameOfCallback, U"LIST_", 5) ? U"char *" :
		Melder_nequ (command -> nameOfCallback, U"INFO_", 5) ? U"char *" :
		Melder_nequ (command -> nameOfCallback, U"HINT_", 5) ? U"char *" :
		U"void";
	return returnType;
}

void praat_menuCommands_writeC (bool isInHeaderFile, bool includeCreateAPI, bool includeReadAPI,
	bool includeRecordAPI, bool includePlayAPI, bool includeDrawAPI, bool includeHelpAPI, bool includeWindowAPI)
{
	try {
		integer numberOfApiMenuCommands = 0;
		for (integer i = 1; i <= theCommands.size; i ++) {
			Praat_Command command = theCommands.at [i];
			const bool deprecated = ( command -> deprecationYear > 0 );
			if (! commandIsToBeIncluded (command, deprecated, includeCreateAPI, includeReadAPI,
				includeRecordAPI, includePlayAPI, includeDrawAPI, includeHelpAPI, includeWindowAPI)) continue;
			MelderInfo_writeLine (U"\n/* Menu command \"", command -> title.get(), U"\"",
				deprecated ? U", deprecated " : U"", deprecated ? Melder_integer (command -> deprecationYear) : U"",
				U" */");
			conststring32 returnType = getReturnType (command);
			MelderInfo_writeLine (returnType, U" Praat", str32chr (command -> nameOfCallback, U'_'), U" (");
			const bool isDirect = ! str32str (command -> title.get(), U"...");
			if (isDirect) {
			} else {
				command -> callback (nullptr, -1, nullptr, nullptr, nullptr, nullptr, false, nullptr, nullptr);
			}
			if (commandHasFileNameArgument (command))
				MelderInfo_writeLine (U"\tconst char *fileName");
			MelderInfo_write (U")");
			if (isInHeaderFile) {
				MelderInfo_writeLine (U";");
			} else {
				MelderInfo_writeLine (U" {");
				MelderInfo_writeLine (U"}");
			}
			numberOfApiMenuCommands += 1;
		}
	} catch (MelderError) {
		Melder_throw (U"Menu commands not written to C library.");
	}
}

/* End of file praat_menuCommands.cpp */

bool praat_searchActivateFixed (integer id) {
    for (integer i = 1; i <= theCommands.size; ++ i) {
        Praat_Command command = theCommands.at [i];
        if (command -> searchId == id && command -> executable) {
            do_menu (command, false); return true;
        }
    }
    return false;
}
