/* Objects-window macro slots. GPL v3 or later. */
#include "praatP.h"

#if motif
#include "GuiP.h"
#include "praat_script.h"
#include <algorithm>
#include <array>
#include <commctrl.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
namespace fs = std::filesystem;
constexpr integer slotsCount = 9;
constexpr size_t maximumFileBytes = 256 * 1024;
constexpr const char *fileHeader = "Praat Macro 1";

struct Slot {
	std::u32string label, script, loadError;
	GuiButton button = nullptr;
};

Thing_define (MacroPad, Thing) {
	std::array<Slot, slotsCount> slots;
	fs::path directory;
	GuiWindow editor;
	GuiText labelField, scriptField;
	GuiButton clearButton;
	integer editing = 0;
	bool running = false;
};
Thing_implement (MacroPad, Thing, 0);

fs::path slotPath (MacroPad me, integer index) {
	return my directory / ("slot-" + std::to_string (index) + ".PraatMacro");
}

void loadSlot (MacroPad me, integer index) {
	auto &slot = my slots [index - 1];
	if (my directory.empty()) return;
	try {
		const auto path = slotPath (me, index);
		if (! fs::exists (path)) return;
		std::ifstream file (path, std::ios::binary);
		Melder_require (file.is_open(), U"Could not read the saved macro.");
		file.exceptions (std::ios::badbit);
		std::string bytes (maximumFileBytes + 1, '\0');
		file.read (bytes.data(), bytes.size());
		bytes.resize (file.gcount());
		Melder_require (bytes.size() <= maximumFileBytes && bytes.find ('\0') == bytes.npos && Melder_str8IsValidUtf8 (bytes.c_str()),
			U"The saved macro is too large or its text is invalid.");
		const std::u32string text = Melder_peek8to32_u (bytes.c_str());
		const auto headerEnd = text.find (U'\n');
		const auto labelEnd = headerEnd == text.npos ? text.npos : text.find (U'\n', headerEnd + 1);
		Melder_require (labelEnd != text.npos && text.substr (0, headerEnd) == Melder_peek8to32_u (fileHeader),
			U"The saved macro format is unrecognized.");
		auto label = text.substr (headerEnd + 1, labelEnd - headerEnd - 1);
		auto script = text.substr (labelEnd + 1);
		Melder_require (Melder_findInk (label.c_str()) && Melder_findInk (script.c_str()), U"The saved macro has no name or script.");
		slot.label = std::move (label);
		slot.script = std::move (script);
	} catch (const std::exception &error) {
		slot.loadError = Melder_peek8to32_u (error.what());
	} catch (MelderError) {
		slot.loadError = Melder_getError ();
		Melder_clearError ();
	}
}

void storeSlot (MacroPad me, integer index, const std::u32string &label, const std::u32string &script) {
	if (my directory.empty()) return;
	const auto destination = slotPath (me, index);
	if (script.empty()) {
		fs::remove (destination);
		return;
	}
	const std::string bytes = std::string (fileHeader) + '\n' + Melder_peek32to8 (label.c_str()) + '\n' + Melder_peek32to8 (script.c_str());
	Melder_require (bytes.size() <= maximumFileBytes, U"The macro is too large. Shorten it to less than 256 KiB.");
	fs::create_directories (my directory);
	wchar_t temporary [MAX_PATH + 1] {};
	Melder_require (GetTempFileNameW (my directory.c_str(), L"prm", 0, temporary), U"Could not create a temporary file for this macro.");
	try {
		std::ofstream out (fs::path (temporary), std::ios::binary);
		out.exceptions (std::ios::badbit | std::ios::failbit);
		out << bytes;
		out.close ();
		Melder_require (MoveFileExW (temporary, destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH),
			U"Could not save this macro (Windows error ", (integer) GetLastError (), U").");
	} catch (...) {
		DeleteFileW (temporary);
		throw;
	}
}

std::u32string fittedLabel (const Slot &slot) {
	const HWND native = slot.button -> d_widget -> window;
	const HDC dc = GetDC (native);
	if (! dc) return slot.label;
	const auto font = (HFONT) SendMessageW (native, WM_GETFONT, 0, 0);
	const auto previous = font ? SelectObject (dc, font) : nullptr;
	RECT area {};
	GetClientRect (native, & area);
	const auto fits = [&] (const std::u32string &text) {
		const auto wide = Melder_peek32toW (text.c_str());
		SIZE size {};
		return GetTextExtentPoint32W (dc, wide, (int) wcslen (wide), & size) && size.cx <= area.right - 12;
	};
	auto label = slot.label;
	if (! fits (label)) {
		label.resize (std::min (label.size(), size_t (24)));
		while (! label.empty() && ! fits (label + U'…')) label.pop_back();
		label += U'…';
	}
	if (previous) SelectObject (dc, previous);
	ReleaseDC (native, dc);
	return label;
}

void updateButton (MacroPad me, integer index) {
	const auto &slot = my slots [index - 1];
	if (! slot.loadError.empty()) {
		GuiButton_setText (slot.button, Melder_cat (index, U" !"));
		GuiButton_setTooltip (slot.button, Melder_cat (U"This macro could not be loaded.\n", slot.loadError.c_str()));
	} else if (slot.script.empty()) {
		GuiButton_setText (slot.button, Melder_cat (index, U" +"));
		GuiButton_setTooltip (slot.button, Melder_cat (U"Slot ", index, U" is empty. Click to add a macro."));
	} else {
		GuiButton_setText (slot.button, fittedLabel (slot).c_str());
		GuiButton_setTooltip (slot.button, Melder_cat (slot.label.c_str(), U"\nClick to run. Right-click to edit."));
	}
}

void closeEditor (MacroPad me) {
	my editing = 0;
	GuiThing_hide (my editor);
}

void cancel (MacroPad me, GuiButtonEvent) { closeEditor (me); }

void apply (MacroPad me, std::u32string label, std::u32string script) {
	try {
		const auto index = my editing;
		if (! index) return;
		storeSlot (me, index, label, script);
		auto &slot = my slots [index - 1];
		slot.label = std::move (label);
		slot.script = std::move (script);
		slot.loadError.clear ();
		updateButton (me, index);
		closeEditor (me);
	} catch (const std::exception &error) {
		Melder_flushError (U"Could not update this macro.\n", Melder_peek8to32_u (error.what()));
	} catch (MelderError) {
		Melder_flushError ();
	}
}

void save (MacroPad me, GuiButtonEvent) {
	const auto name = GuiText_getString (my labelField), script = GuiText_getString (my scriptField);
	if (! Melder_findInk (name.get()) || ! Melder_findInk (script.get())) {
		Melder_flushError (U"Enter a name and a Praat script for this slot.");
		return;
	}
	if (str32chr (name.get(), U'\n') || str32chr (name.get(), U'\r')) {
		Melder_flushError (U"Keep the button name on one line.");
		return;
	}
	apply (me, name.get(), script.get());
}

void clear (MacroPad me, GuiButtonEvent) {
	const HWND owner = my editor -> d_xmShell -> window;
	if (MessageBoxW (owner, L"Clear the saved macro from this slot?", L"Clear macro slot", MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) == IDOK)
		apply (me, {}, {});
}

void edit (MacroPad me, integer index) {
	if (my running) return;
	if (my editing) {
		GuiThing_show (my editor);   // Keep in-progress edits when another slot is clicked.
		return;
	}
	if (! my editor) {
		my editor = GuiWindow_create (160, 150, 650, 460, 510, 355, U"Edit macro", closeEditor, me, 0);
		GuiLabel_createShown (my editor, 14, -14, 12, 32, U"Button name", 0);
		my labelField = GuiText_createShown (my editor, 14, -14, 35, 62, 0);
		SendMessageW (my labelField -> d_widget -> window, EM_SETLIMITTEXT, 80, 0);
		GuiLabel_createShown (my editor, 14, -14, 74, 94, U"Praat script to run on click", 0);
		my scriptField = GuiText_createShown (my editor, 14, -14, 97, -66, GuiText_SCROLLED);
		my clearButton = GuiButton_createShown (my editor, 14, 137, -48, -15, U"Clear slot…", clear, me, 0);
		GuiButton_createShown (my editor, -208, -116, -48, -15, U"Cancel", cancel, me, GuiButton_CANCEL);
		GuiButton_createShown (my editor, -106, -14, -48, -15, U"Save", save, me, 0);
	}
	my editing = index;
	const auto &slot = my slots [index - 1];
	GuiShell_setTitle (my editor, Melder_cat (U"Edit macro slot ", index));
	GuiText_setString (my labelField, slot.label.c_str(), false);
	GuiText_setString (my scriptField, slot.script.c_str(), false);
	GuiThing_setSensitive (my clearButton, ! slot.script.empty() || ! slot.loadError.empty());
	GuiThing_show (my editor);
}

void clickSlot (MacroPad me, GuiButtonEvent event) {
	if (my running) return;
	for (integer index = 1; index <= slotsCount; ++ index) {
		const auto &slot = my slots [index - 1];
		if (slot.button != event -> button) continue;
		if (slot.script.empty() || event -> shiftKeyPressed || my editing) {
			edit (me, index);
			return;
		}
		my running = true;
		try {
			praat_executeScriptFromText (slot.script.c_str());
		} catch (MelderError) {
			Melder_flushError (U"Could not finish running the macro “", slot.label.c_str(), U"”.");
		}
		my running = false;
		return;
	}
}

LRESULT CALLBACK macroButtonSubclass (HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR index, DWORD_PTR data) {
	if (message == WM_NCDESTROY)
		RemoveWindowSubclass (window, macroButtonSubclass, index);
	if (message == WM_RBUTTONUP || message == WM_CONTEXTMENU) {
		try {
			edit (reinterpret_cast<MacroPad> (data), (integer) index);
		} catch (MelderError) {
			Melder_flushError (U"Could not open this macro slot.");
		}
		return 0;
	}
	return DefSubclassProc (window, message, wParam, lParam);
}

} // namespace

void praat_macroPad_init (GuiWindow window) {
	auto me = Thing_new (MacroPad);
	if (! praatP.ignorePreferenceFiles && ! MelderFolder_isNull (Melder_preferencesFolder ()))
		my directory = fs::u8path (Melder_peek32to8 (MelderFolder_peekPath (Melder_preferencesFolder ()))) / "Macros";
	GuiLabel_createShown (window, 8, 199, -221, -200, U"Macro pad · right-click to edit", 0);
	for (integer index = 1; index <= slotsCount; ++ index) {
		const int x = 8 + 63 * int ((index - 1) % 3), y = -198 + 28 * int ((index - 1) / 3);
		auto &slot = my slots [index - 1];
		slot.button = GuiButton_createShown (window, x, x + 59, y, y + 24, U"", clickSlot, me.get(), 0);
		loadSlot (me.get(), index);
		updateButton (me.get(), index);
		SetWindowSubclass (slot.button -> d_widget -> window, macroButtonSubclass, (UINT_PTR) index, (DWORD_PTR) me.get());
	}
	me.releaseToAmbiguousOwner ();
}
#endif
