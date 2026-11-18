#ifndef _GuiTheme_h_
#define _GuiTheme_h_
#include "Graphics.h"

// One palette for native widgets and opted-in editor canvases; exports never opt in.
void GuiTheme_prefs ();
integer GuiTheme_get ();
void GuiTheme_set (integer theme);
bool GuiTheme_buttonIcons ();
void GuiTheme_setButtonIcons (bool show);
void GuiTheme_apply ();
bool GuiTheme_active ();
void GuiTheme_enableEditor (Graphics graphics);
bool GuiTheme_editor (Graphics graphics);
MelderColour GuiTheme_colour (Graphics graphics, MelderColour colour);
#if defined (_WIN32)
#include <windows.h>
void GuiTheme_attach (HWND window);
HBRUSH GuiTheme_backgroundBrush ();
bool GuiTheme_drawCanvasButton (Graphics graphics, HDC dc, RECT rect);
#endif
#endif
