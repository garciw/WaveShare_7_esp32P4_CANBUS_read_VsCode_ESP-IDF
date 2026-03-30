#ifndef SCREEN1_H
#define SCREEN1_H

#include "lvgl.h" 
#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

// Function declarations
void setupScreen1();
void updateDisplay();
void updateScreen1Speed();
void updateEcuLoggingText();
void updateWiFiStatusIcon();

// Event Handlers
void dropdown_event_cb(lv_event_t *e);

// Internal helper for dropdown logic
void updateSlotInternal(lv_obj_t *slotLabel, const char* selectedItem);

#ifdef __cplusplus
}
#endif

#endif // SCREEN1_H