#pragma once

#include "clay.h"

#define COLOR_LIGHT (Clay_Color) { 224, 215, 210, 255 }
#define COLOR_RED (Clay_Color) { 168, 66, 28, 255 }
#define COLOR_ORANGE (Clay_Color) { 225, 138, 50, 255 }
#define COLOR_BLACK (Clay_Color) { 0, 0, 0, 255 }

#define COLOR_CHAT_BACKGROUND (Clay_Color) { 55, 100, 55, 255 }
#define COLOR_SIDEBAR_BACKGROUND (Clay_Color) { 80, 120, 80, 255 }

#define COLOR_HOVER_HIGHLIGHT (Clay_Color) { 255, 255, 255, 100 }
#define COLOR_HOVERLESS_HIGHLIGHT (Clay_Color) { 255, 255, 255, 20 }
#define COLOR_TAB_TEXT COLOR_BLACK

typedef enum {
    REGULAR_24,
    MONO_16,
} FontIDs;
