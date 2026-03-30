#ifndef SCREEN2_H
#define SCREEN2_H

#include "lvgl.h"
#include "ui.h" 

#ifdef __cplusplus
extern "C" {
#endif

// Function declarations
void setupScreen2();
void updateDisplay2();
void updateScreen2Speed();
void updateEcuLoggingText2();

// Event Handlers
void dropdown_event_cb2(lv_event_t *e);

// Internal helper (matching the renamed version in .cpp)
void updateSlot2Internal(lv_obj_t *slotLabel2, const char* selectedItem);

#ifdef __cplusplus
}
#endif

#endif // SCREEN2_H