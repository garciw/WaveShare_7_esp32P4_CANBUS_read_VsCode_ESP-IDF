#ifndef WARNING_BANNER_H
#define WARNING_BANNER_H

#include "lvgl.h"
#include "ui.h" 

#ifdef __cplusplus
extern "C" {
#endif

// --- External Assets ---
// Using Orbitron for that high-visibility "Racing" look
LV_FONT_DECLARE(ui_font_orbitron25);

// --- Warning Banner Functions ---
// These handle the global overlay that punches through all screens
void init_warning_banner();
void check_warnings();
void show_warning_banner();
void hide_warning_banner();

#ifdef __cplusplus
}
#endif

#endif // WARNING_BANNER_H