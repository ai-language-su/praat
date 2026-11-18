#ifndef _GuiHoverHelp_h_
#define _GuiHoverHelp_h_
#include "Graphics.h"

// Regions use the current Graphics transform, including DPI and panel viewport.
// Register them while drawing; begin() discards regions from the previous frame.
void GuiHoverHelp_begin (Graphics graphics);
void GuiHoverHelp_rectangle (Graphics graphics, double x1, double x2, double y1, double y2, conststring32 help);
void GuiHoverHelp_label (Graphics graphics, double x, double y, conststring32 label, conststring32 help);
void GuiControl_setHoverHelp (GuiControl control, conststring32 help);

template <typename... Args>
void Graphics_textWithHelp (Graphics graphics, conststring32 help, double x, double y, const Args... args) {
	autostring32 label = Melder_dup (Melder_cat (args...));
	GuiHoverHelp_label (graphics, x, y, label.get(), help);
	Graphics_text (graphics, x, y, label.get());
}
#endif
