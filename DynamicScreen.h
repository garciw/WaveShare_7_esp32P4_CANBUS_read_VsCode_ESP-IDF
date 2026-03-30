#ifndef DYNAMICSCREEN_H
#define DYNAMICSCREEN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// ✅ Function Prototypes
// These handle the logic for switching between sub-menus (Containers)
void setupDynamicScreen();
void hideAllContainers();

// ✅ UI Event Callbacks
// These are triggered by SquareLine buttons
void showBrightness(lv_event_t *e);
void showSpeedUnit(lv_event_t *e);
void showDiagnostics(lv_event_t *e);

#ifdef __cplusplus
}
#endif

#endif // DYNAMICSCREEN_H