#ifndef _CommandSearchWindow_h_
#define _CommandSearchWindow_h_
#include "Editor.h"
void CommandSearchWindow_show (Editor origin = nullptr);
void CommandSearchWindow_refresh ();
void CommandSearch_registerEditor (Editor editor);
void CommandSearch_unregisterEditor (Editor editor);
#endif
