#include "CommandSearchWindow.h"
#include "CommandSearch.h"
#include "praatP.h"
#include <algorithm>

namespace {
using Text = std::u32string;
struct Result {
    integer id;
    int source, score = 0;
    Text title, path, summary, guide, requirements;
    bool eligible;
};
std::vector<Editor> editors;
integer nextId = 0;
Text copy (conststring32 s) { return s ? Text (s) : Text (); }
Text normalized (Text s) { for (auto &c : s) c = Melder_toLowerCase (c); return CommandSearch_normalize (s); }
integer identity (integer &id) { if (! id) id = ++ nextId; return id; }
Text keywords (const Result &r) {
    if (r.path.find (U"Sound") == Text::npos) return {};
    if (r.title == U"Concatenate") return U"join recordings combine recordings end to end append audio";
    if (r.title == U"Combine to stereo") return U"combine recordings stereo left right channels";
    if (r.title == U"To TextGrid...") return U"label speech annotate recording transcribe tiers";
    if (r.title == U"Play") return U"listen hear audio";
    return {};
}
Text requirements (Praat_Command c) {
    Text out;
    const ClassInfo classes [] = { c -> class1, c -> class2, c -> class3, c -> class4 };
    const integer counts [] = { c -> n1, c -> n2, c -> n3, c -> n4 };
    for (int i = 0; i < 4; ++ i) if (classes [i]) {
        if (! out.empty()) out += U", ";
        out += counts [i] ? copy (Melder_integer (counts [i])) : U"one or more";
        out += U" "; out += classes [i] -> className;
    }
    return out.empty() ? U"Uses the current window or Objects selection." : U"Select " + out + U" in Objects, with no other object types selected.";
}
std::vector<Result> catalogue (bool hidden) {
    std::vector<Result> out;
    for (int source = 0; source < 2; ++ source) {
        Text parents [8], scope;
        integer count = source ? praat_getNumberOfMenuCommands () : praat_getNumberOfActions ();
        for (integer i = 1; i <= count; ++ i) {
            Praat_Command c = source ? praat_getMenuCommand (i) : praat_getAction (i);
            if (! c -> title || c -> title [0] == U'-') continue;
            Text root = source ? copy (c -> window.get()) + U" > " + (c -> menu ? copy (c -> menu.get()) : U"Object controls") : U"Objects > " + copy (c -> class1 ? c -> class1 -> className : U"Object");
            if (source && c -> menu && str32equ (c -> menu.get(), U"ApplicationHelp")) root = copy (c -> window.get()) + U" > Help";
            if (! source) {
                if (c -> class2) root += U" + " + copy (c -> class2 -> className);
                if (c -> class3) root += U" + " + copy (c -> class3 -> className);
                if (c -> class4) root += U" + " + copy (c -> class4 -> className);
            }
            if (root != scope) { for (auto &p : parents) p.clear(); scope = root; }
            int depth = std::clamp (int (c -> depth), 0, 7);
            parents [depth] = c -> callback ? Text () : copy (c -> title.get());
            if (! c -> callback && parents [depth].size() >= 2 &&
                (parents [depth].back() == U'>' || parents [depth].back() == U'-'))
                parents [depth].resize (parents [depth].size() - 2);
            for (int d = depth + 1; d < 8; ++ d) parents [d].clear();
            if (! c -> callback || (c -> hidden && ! hidden)) continue;
            if (source && ! str32equ (c -> window.get(), U"Objects") && ! str32equ (c -> window.get(), U"Picture")) continue;
            Text path = root;
            for (int d = 0; d < depth; ++ d) if (! parents [d].empty()) path += U" > " + parents [d];
            Result r { identity (c -> searchId), source, 0, copy (c -> title.get()), path, {}, {}, requirements (c), bool (c -> executable) };
            if (! source) { r.summary = copy (praat_actionSummary (c)); r.guide = copy (praat_actionGuide (c)); }
            if (c -> hidden) r.path += U" [hidden]";
            out.push_back (std::move (r));
        }
    }
    for (Editor e : editors) for (integer m = 1; m <= e -> menus.size; ++ m) {
        EditorMenu menu = e -> menus.at [m];
        for (integer j = 1; j <= menu -> commands.size; ++ j) {
            EditorCommand c = menu -> commands.at [j];
            if (! c -> commandCallback || ! c -> itemWidget || ! c -> itemTitle) continue;
            out.push_back ({ identity (c -> searchId), 2, 0, copy (c -> itemTitle.get()),
                copy (Thing_className (e)) + U" [" + copy (e -> name.get()) + U"] > " + copy (menu -> menuTitle.get()),
                U"Run this command in the named editor window.", {}, U"Uses the selection and settings in that editor.", c -> itemWidget -> searchSensitive });
        }
    }
    return out;
}
}

Thing_define (CommandSearchWindow, Thing) {
    GuiWindow dialog;
    GuiText query, detail;
    GuiList list;
    GuiButton run;
    GuiCheckButton hidden;
    std::vector<Result> results;
    integer selectedId = 0;
    bool updating = false, visible = false;
};
Thing_implement (CommandSearchWindow, Thing, 0);
static CommandSearchWindow window;

static void showDetail (CommandSearchWindow me) {
    auto positions = GuiList_getSelectedPositions (my list);
    if (! positions.size || positions [1] > integer (my results.size())) {
        GuiText_setString (my detail, U"No matching commands. Try a command name, a partial word, or a task such as join recordings.");
        GuiThing_setSensitive (my run, false); return;
    }
    const Result &r = my results [positions [1] - 1]; my selectedId = r.id;
    Text text = r.title + U"\n" + r.path + U"\n\n" + (r.eligible ? U"Available for the current context." : U"Unavailable for the current context.");
    text += U"\n\n" + r.requirements;
    if (! r.summary.empty()) text += U"\n\n" + r.summary;
    if (! r.guide.empty()) text += U"\n____________________________\n\n" + r.guide;
    GuiText_setString (my detail, text.c_str());
    GuiThing_setSensitive (my run, r.eligible);
}
void CommandSearchWindow_refresh () {
    if (! window || ! window -> visible || window -> updating) return;
    auto me = window;
    my updating = true;
    try {
        auto query = GuiText_getString (my query);
        const Text q = normalized (copy (query.get()));
        my results = catalogue (GuiCheckButton_getValue (my hidden));
        for (auto &r : my results) r.score = CommandSearch_score (q, normalized (r.title), normalized (keywords (r)), normalized (r.summary + U" " + r.guide + U" " + r.path));
        my results.erase (std::remove_if (my results.begin(), my results.end(), [] (const Result &r) { return r.score == 0; }), my results.end());
        std::stable_sort (my results.begin(), my results.end(), [] (const Result &a, const Result &b) { return a.score != b.score ? a.score > b.score : a.eligible > b.eligible; });
        GuiList_deleteAllItems (my list);
        integer selected = 1;
        for (size_t i = 0; i < my results.size(); ++ i) {
            const auto &r = my results [i];
            Text label = r.title + U" — " + r.path + (r.eligible ? U"" : U" [unavailable]");
            GuiList_insertItem (my list, label.c_str(), 0);
            if (r.id == my selectedId) selected = integer (i) + 1;
        }
        if (! my results.empty()) GuiList_selectItem (my list, selected);
        showDetail (me);
    } catch (MelderError) { Melder_flushError (); }
    my updating = false;
}
static void changed (CommandSearchWindow, GuiTextEvent) { CommandSearchWindow_refresh (); }
static void scopeChanged (CommandSearchWindow, GuiCheckButtonEvent) { CommandSearchWindow_refresh (); }
static void selected (CommandSearchWindow me, GuiList_SelectionChangedEvent) { if (! my updating) showDetail (me); }
static void closeSearch (CommandSearchWindow me) { my visible = false; GuiThing_hide (my dialog); }
static void closeButton (CommandSearchWindow me, GuiButtonEvent) { closeSearch (me); }
static void activate (CommandSearchWindow me, GuiButtonEvent) {
    const integer id = my selectedId;
    praat_show ();  // refresh fixed buttons and current selection before resolving identity
    bool done = praat_searchActivateAction (id) || praat_searchActivateFixed (id);
    if (! done) for (Editor e : editors) {
        for (integer m = 1; m <= e -> menus.size; ++ m) {
            EditorMenu menu = e -> menus.at [m];
            for (integer j = 1; j <= menu -> commands.size; ++ j) {
                EditorCommand c = menu -> commands.at [j];
                if (c -> searchId == id && c -> itemWidget && c -> itemWidget -> searchSensitive) {
                    Editor_activateSearchCommand (c); CommandSearchWindow_refresh (); return;
                }
            }
        }
    }
    if (! done) { Melder_flushError (U"This command is no longer available. Check the Objects selection and editor window."); }
    CommandSearchWindow_refresh ();
}
static void searchManual (CommandSearchWindow, GuiButtonEvent) { Melder_search (); }
void CommandSearchWindow_show (Editor) {
    if (theCurrentPraatApplication -> batch) return;
    if (! window) {
        auto me = Thing_new (CommandSearchWindow);
        my dialog = GuiWindow_create (100, 100, 900, 700, 700, 580, U"Search Praat", closeSearch, me.get(), 0);
        GuiLabel_createShown (my dialog, 12, -12, 10, 32, U"Find a command or describe a task", 0);
        my query = GuiText_createShown (my dialog, 12, -12, 36, 64, 0);
        GuiText_setChangedCallback (my query, changed, me.get());
        my hidden = GuiCheckButton_createShown (my dialog, 12, -12, 70, 94, U"Include hidden commands", scopeChanged, me.get(), 0);
        my list = GuiList_createShown (my dialog, 12, -12, 100, 315, false, nullptr);
        GuiList_setSelectionChangedCallback (my list, selected, me.get());
        my detail = GuiText_createShown (my dialog, 12, -12, 325, -52, GuiText_SCROLLED | GuiText_ANYWRAP | GuiText_NONEDITABLE);
        my run = GuiButton_createShown (my dialog, 12, 180, -42, -12, U"Run command", activate, me.get(), GuiButton_DEFAULT);
        GuiButton_createShown (my dialog, 195, 365, -42, -12, U"Search manual...", searchManual, me.get(), 0);
        GuiButton_createShown (my dialog, -110, -12, -42, -12, U"Close", closeButton, me.get(), GuiButton_CANCEL);
        window = me.releaseToAmbiguousOwner ();
    }
    window -> visible = true;
    CommandSearchWindow_refresh ();
    GuiThing_show (window -> dialog);
}
void CommandSearch_registerEditor (Editor editor) {
    if (std::find (editors.begin(), editors.end(), editor) == editors.end()) editors.push_back (editor);
    CommandSearchWindow_refresh ();
}
void CommandSearch_unregisterEditor (Editor editor) {
    editors.erase (std::remove (editors.begin(), editors.end(), editor), editors.end());
    CommandSearchWindow_refresh ();
}
