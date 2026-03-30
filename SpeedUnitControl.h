#ifndef SPEEDUNITCONTROL_H
#define SPEEDUNITCONTROL_H

#include <stdbool.h>
#include "lvgl.h"
#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Global Unit State ---
// This is used by Screen1, Screen2, and Screen 7 to decide 
// whether to multiply speed by 0.621371
extern bool useMPH;  

// --- Function Prototypes ---
void setupSpeedUnitEvents();

// UI Callbacks for your Settings Checkboxes
void mph_checkbox_cb(lv_event_t* e);
void kph_checkbox_cb(lv_event_t* e);

#ifdef __cplusplus
}
#endif

#endif // SPEEDUNITCONTROL_H