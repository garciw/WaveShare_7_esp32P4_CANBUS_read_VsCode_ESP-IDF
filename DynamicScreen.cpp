#include "DynamicScreen.h"
#include "ui.h"
#include "lvgl.h" 

// 1. Externs (These must match SquareLine's ui.h exactly)
extern lv_obj_t *ui_Screen5;
extern lv_obj_t *ui_MainAlertPanel;
extern lv_obj_t *ui_diagnosticContainer;
extern lv_obj_t *ui_brightnessContainer;
extern lv_obj_t *ui_speedUnitContainer;
extern lv_obj_t *ui_btnDiagnostics;
extern lv_obj_t *ui_btnSpeedunit;
extern lv_obj_t *ui_btnDisplay;

// 2. Helper Functions
void hideAllContainers() {
    if(ui_brightnessContainer) lv_obj_add_flag(ui_brightnessContainer, LV_OBJ_FLAG_HIDDEN);
    if(ui_speedUnitContainer) lv_obj_add_flag(ui_speedUnitContainer, LV_OBJ_FLAG_HIDDEN);
    if(ui_diagnosticContainer) lv_obj_add_flag(ui_diagnosticContainer, LV_OBJ_FLAG_HIDDEN);
}

// 3. The "Tab" Functions
void showBrightness(lv_event_t *e) {
    hideAllContainers();
    if(ui_brightnessContainer) lv_obj_remove_flag(ui_brightnessContainer, LV_OBJ_FLAG_HIDDEN);
}

void showSpeedUnit(lv_event_t *e) {
    hideAllContainers();
    if(ui_speedUnitContainer) lv_obj_remove_flag(ui_speedUnitContainer, LV_OBJ_FLAG_HIDDEN);
}

void showDiagnostics(lv_event_t *e) {
    hideAllContainers();
    if(ui_diagnosticContainer) lv_obj_remove_flag(ui_diagnosticContainer, LV_OBJ_FLAG_HIDDEN);
}

// 4. The "Shortcut" Function
void onAlertClick(lv_event_t *e) {
    // Step A: Go to Screen 5 (Diagnostics/Settings Screen)
    if(ui_Screen5) {
        lv_screen_load_anim(ui_Screen5, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
    }
    
    // Step B: Call showDiagnostics directly
    showDiagnostics(NULL); 
}

// 5. Setup (Attach the events)
void setupDynamicScreen() {
    if(ui_btnSpeedunit) lv_obj_add_event_cb(ui_btnSpeedunit, showSpeedUnit, LV_EVENT_CLICKED, NULL);
    if(ui_btnDiagnostics) lv_obj_add_event_cb(ui_btnDiagnostics, showDiagnostics, LV_EVENT_CLICKED, NULL);
    if(ui_btnDisplay) lv_obj_add_event_cb(ui_btnDisplay, showBrightness, LV_EVENT_CLICKED, NULL);
    
    // Attach the shortcut to the main alert panel (popup)
    if(ui_MainAlertPanel) {
        lv_obj_add_event_cb(ui_MainAlertPanel, onAlertClick, LV_EVENT_CLICKED, NULL);
    }
}