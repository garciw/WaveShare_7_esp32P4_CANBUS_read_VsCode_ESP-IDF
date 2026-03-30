#include "SpeedUnitControl.h"
#include "lvgl.h"
#include "ui.h"
#include "globalVariables.h"
#include "nvs_flash.h" 
#include "nvs.h" 

// Definition of the global variable
bool useMPH = false;  

void mph_checkbox_cb(lv_event_t* e) {
    useMPH = true;
    
    // Update UI state
    if(ui_KMHCheckbox) lv_obj_remove_state(ui_KMHCheckbox, LV_STATE_CHECKED);  
    if(ui_MPHCheckbox) lv_obj_add_state(ui_MPHCheckbox, LV_STATE_CHECKED);  
    
    // 🛑 ESP-IDF NVS SAVE
    nvs_handle_t my_handle;
    if (nvs_open("RaceDash", NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_set_u8(my_handle, "speedUnit", 1); // 1 = true/MPH
        nvs_commit(my_handle);
        nvs_close(my_handle);
    }
}

void kph_checkbox_cb(lv_event_t* e) {
    useMPH = false;
    
    // Update UI state
    if(ui_MPHCheckbox) lv_obj_remove_state(ui_MPHCheckbox, LV_STATE_CHECKED);  
    if(ui_KMHCheckbox) lv_obj_add_state(ui_KMHCheckbox, LV_STATE_CHECKED);  

    // 🛑 ESP-IDF NVS SAVE
    nvs_handle_t my_handle;
    if (nvs_open("RaceDash", NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_set_u8(my_handle, "speedUnit", 0); // 0 = false/KPH
        nvs_commit(my_handle);
        nvs_close(my_handle);
    }
}

void setupSpeedUnitEvents() {
    // 🛑 ESP-IDF NVS LOAD
    uint8_t tempMPH = 0; 
    nvs_handle_t my_handle;
    if (nvs_open("RaceDash", NVS_READONLY, &my_handle) == ESP_OK) {
        nvs_get_u8(my_handle, "speedUnit", &tempMPH);
        nvs_close(my_handle);
    }
    useMPH = (tempMPH == 1);

    // Apply loaded state to UI
    if (useMPH) {
        if(ui_MPHCheckbox) lv_obj_add_state(ui_MPHCheckbox, LV_STATE_CHECKED);
        if(ui_KMHCheckbox) lv_obj_remove_state(ui_KMHCheckbox, LV_STATE_CHECKED);
    } else {
        if(ui_KMHCheckbox) lv_obj_add_state(ui_KMHCheckbox, LV_STATE_CHECKED);
        if(ui_MPHCheckbox) lv_obj_remove_state(ui_MPHCheckbox, LV_STATE_CHECKED);
    }

    // Attach Event Callbacks
    if(ui_MPHCheckbox) lv_obj_add_event_cb(ui_MPHCheckbox, mph_checkbox_cb, LV_EVENT_VALUE_CHANGED, NULL);
    if(ui_KMHCheckbox) lv_obj_add_event_cb(ui_KMHCheckbox, kph_checkbox_cb, LV_EVENT_VALUE_CHANGED, NULL);
}