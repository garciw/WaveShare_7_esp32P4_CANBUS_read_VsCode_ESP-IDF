#include "BrightnessControl.h"
#include "globalVariables.h"
#include "lvgl.h"
#include "ui.h"
#include <stdio.h>
#include <math.h>       // For fabs()
#include "nvs_flash.h"  // Replaces Arduino Preferences
#include "nvs.h"        // Replaces Arduino Preferences

lv_obj_t* dim_layer;

// ==========================================
// 1. CREATE OVERLAY
// ==========================================
void createBrightnessOverlay(lv_obj_t* parent) {
    // Create the Dim Layer on the TOP layer (Covers everything)
    dim_layer = lv_obj_create(lv_layer_top());
    lv_obj_set_size(dim_layer, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(dim_layer, 0, 0);
    
    // Style: Black, No Border
    lv_obj_set_style_bg_color(dim_layer, lv_color_black(), 0);
    lv_obj_set_style_border_width(dim_layer, 0, 0);
    lv_obj_set_style_outline_width(dim_layer, 0, 0);
    
    // Flags: MUST allow clicks to pass through!
    lv_obj_add_flag(dim_layer, LV_OBJ_FLAG_IGNORE_LAYOUT); 
    lv_obj_clear_flag(dim_layer, LV_OBJ_FLAG_CLICKABLE); // <--- Critical: Lets you touch buttons underneath
    lv_obj_clear_flag(dim_layer, LV_OBJ_FLAG_SCROLLABLE);

    // 🛑 ESP-IDF NVS READ (Replaces Arduino Preferences)
    int savedBrightness = 50; // Default fallback
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("RaceDash", NVS_READONLY, &my_handle);
    if (err == ESP_OK) {
        int32_t val = 50;
        if (nvs_get_i32(my_handle, "Brightness", &val) == ESP_OK) {
            savedBrightness = (int)val;
        }
        nvs_close(my_handle);
    }

    printf("🔄 Brightness LOADED: %d%%\n", savedBrightness);

    // Apply initial state
    setBrightness(savedBrightness); 
    
    // Sync UI Slider
    if (ui_Slider1) {
        lv_slider_set_value(ui_Slider1, savedBrightness, LV_ANIM_OFF);
        
        // Sync UI Label
        if (ui_Slider1Value) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d%%", savedBrightness);
            lv_label_set_text(ui_Slider1Value, buf);
        }
    }
}   

// ==========================================
// 2. SET BRIGHTNESS (Optimized)
// ==========================================
void setBrightness(int level) {
    if (!dim_layer) return;

    // ESP-IDF Math: Replaces Arduino map(level, 0, 100, 200, 0);
    // Slider 100 = Opacity 0 (Clear)
    // Slider 0   = Opacity 200 (Dark)
    int opacity = 200 - (level * 2); 
    
    // 🚀 PERFORMANCE TRICK: 
    // If Brightness is 100% (Opacity 0), HIDE the layer completely.
    // This stops the ESP32 from calculating transparency for a clear layer.
    if (opacity <= 0) {
        if (!lv_obj_has_flag(dim_layer, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_add_flag(dim_layer, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        if (lv_obj_has_flag(dim_layer, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(dim_layer, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_set_style_bg_opa(dim_layer, opacity, 0);
    }
}

// ==========================================
// 3. SAVE SETTING
// ==========================================
void my_saveBrightnessSetting(lv_event_t * e) {
    if (!ui_Slider1) return;
    int brightness = lv_slider_get_value(ui_Slider1);
    
    // 🛑 ESP-IDF NVS WRITE (Replaces Arduino Preferences)
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("RaceDash", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        nvs_set_i32(my_handle, "Brightness", (int32_t)brightness);
        nvs_commit(my_handle); // This actually saves it to memory
        nvs_close(my_handle);
    }
    
    printf("✅ Brightness SAVED: %d%%\n", brightness);
}

// ==========================================
// 4. EVENTS
// ==========================================
void slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
    int brightness = lv_slider_get_value(slider);

    // Apply Immediately
    setBrightness(brightness);

    // Update Label Text
    if (ui_Slider1Value) {
        char buf[16]; // Stack buffer (Fast, No memory leak)
        snprintf(buf, sizeof(buf), "%d%%", brightness);
        lv_label_set_text(ui_Slider1Value, buf);

        // Update Label Position
        int knob_x = lv_obj_get_x(slider) + (lv_obj_get_width(slider) * brightness / 100) - 240; 
        int knob_y = lv_obj_get_y(slider) - 210;
        
        lv_obj_set_pos(ui_Slider1Value, knob_x, knob_y);
    }
}

void save_button_cb(lv_event_t* e) {
    saveBrightnessSetting(e); 
}

// ==========================================
// 5. ODOMETER 
// ==========================================
void updateOdometer() {
    static float lastOdometer = -1;
    float totalOdometer = STATIC_ODOMETER_BASE + receivedData.userChannel2;

    // Dirty Check: Only update text if value changed by 0.1
    // ESP-IDF uses fabs() for float math instead of abs()
    if (fabs(totalOdometer - lastOdometer) > 0.1) {
        if (ui_ODOMETER) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f", totalOdometer);
            lv_label_set_text(ui_ODOMETER, buf);
        }
        lastOdometer = totalOdometer;
    }
}

// ==========================================
// 6. SETUP HOOKS
// ==========================================
void setupUIEvents() {
    if (ui_Slider1) {
        lv_obj_add_event_cb(ui_Slider1, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }
    if (ui_dimmerSaveBTN) {
        lv_obj_add_event_cb(ui_dimmerSaveBTN, save_button_cb, LV_EVENT_CLICKED, NULL);
    }
}
// working✅
