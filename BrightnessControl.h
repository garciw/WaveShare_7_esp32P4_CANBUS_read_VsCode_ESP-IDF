#ifndef BRIGHTNESSCONTROL_H
#define BRIGHTNESSCONTROL_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Logic Functions
void createBrightnessOverlay(lv_obj_t* parent);
void setBrightness(int level);
void my_saveBrightnessSetting(lv_event_t* e);
void updateOdometer();

// LVGL Event Callbacks
void slider_event_cb(lv_event_t *e);
void save_button_cb(lv_event_t* e);
void setupUIEvents();
void brightness_timer_cb(lv_timer_t* timer);   //extern

#ifdef __cplusplus
}
#endif

#endif // BRIGHTNESSCONTROL_H