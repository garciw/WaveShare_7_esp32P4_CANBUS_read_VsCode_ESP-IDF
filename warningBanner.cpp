#include "WarningBanner.h"
#include "lvgl.h"
#include "globalVariables.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

lv_obj_t *warning_banner = nullptr;
lv_obj_t *warnings[11];
lv_timer_t *warning_timer = nullptr;
bool warning_active = false;
int current_warning = 0;
int active_warnings[11];
int num_active_warnings = 0;

// Forward declaration
void warning_timer_cb(lv_timer_t * timer);

// Initialize warning banner UI
void init_warning_banner() {
    if (warning_banner != nullptr) return; // Prevent double initialization

    // Create banner on top layer (Global Overlay)
    warning_banner = lv_obj_create(lv_layer_top());
    lv_obj_set_size(warning_banner, 800, 60);
    lv_obj_align(warning_banner, LV_ALIGN_BOTTOM_MID, 0, -10); // 0,20Moved up slightly for better visibility
    
    lv_obj_set_style_bg_color(warning_banner, lv_color_hex(0xFFFF00), LV_PART_MAIN); 
    lv_obj_set_style_bg_opa(warning_banner, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_width(warning_banner, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(warning_banner, lv_color_hex(0x000000), LV_PART_MAIN);
    
    lv_obj_add_flag(warning_banner, LV_OBJ_FLAG_HIDDEN);  

    // Style for high-contrast black text
    static lv_style_t style_warn;
    lv_style_init(&style_warn);
    lv_style_set_text_color(&style_warn, lv_color_hex(0x000000));  
    lv_style_set_text_font(&style_warn, &lv_font_montserrat_28);   

    for (int i = 0; i < 11; i++) {
        warnings[i] = lv_label_create(warning_banner);
        lv_obj_add_style(warnings[i], &style_warn, LV_PART_MAIN);  
        lv_obj_add_flag(warnings[i], LV_OBJ_FLAG_HIDDEN); 
        lv_obj_center(warnings[i]); // Center text in banner
    }
    
    // Initialize timer (750ms flash)
    warning_timer = lv_timer_create(warning_timer_cb, 750, NULL);
    lv_timer_pause(warning_timer);
}

void warning_timer_cb(lv_timer_t * timer) {
    if (!warning_active) {
        lv_timer_pause(timer);
        hide_warning_banner();
        return;
    }

    static bool banner_toggle = false;
    banner_toggle = !banner_toggle;

    if (banner_toggle) {
        show_warning_banner();
    } else {
        hide_warning_banner();
    }
}

void check_warnings() {
    num_active_warnings = 0;

    // --- Safety Logic (Starlet 5E-FTE Specific) ---
    if (receivedData.rpm >= 7500) active_warnings[num_active_warnings++] = 0;
    if (receivedData.oilPressure < 10 && receivedData.vehicleSpeed > 10) active_warnings[num_active_warnings++] = 1;
    if (receivedData.fuelLevelPercent < 15.0f) active_warnings[num_active_warnings++] = 2; 
    if (receivedData.vehicleSpeed >= 130) active_warnings[num_active_warnings++] = 3;
    if (receivedData.fuelDuty >= 90.0f) active_warnings[num_active_warnings++] = 4;
    if (receivedData.batteryVoltage < 12.5f && receivedData.rpm > 500) active_warnings[num_active_warnings++] = 5;
    if (receivedData.MAP >= 290.0f) active_warnings[num_active_warnings++] = 6;
    if (receivedData.lambdaA < 0.700f && receivedData.rpm > 1000) active_warnings[num_active_warnings++] = 7;
    if (receivedData.coolantTemp >= 104.0f) active_warnings[num_active_warnings++] = 8;
    if (receivedData.MAP > 140.0f && receivedData.lambdaA >= 0.90f) active_warnings[num_active_warnings++] = 9;
    if (receivedData.rpm >= 3500 && receivedData.oilPressure < 35.0f) active_warnings[num_active_warnings++] = 10;  

    if (num_active_warnings > 0) {
        warning_active = true;
        if (warning_timer) lv_timer_resume(warning_timer);
    } else {
        warning_active = false;
        if (warning_timer) lv_timer_pause(warning_timer);
        hide_warning_banner();
    }
}

void show_warning_banner() {
    if (!warning_banner) return;
    lv_obj_remove_flag(warning_banner, LV_OBJ_FLAG_HIDDEN);

    // Hide all labels first
    for (int i = 0; i < 11; i++) lv_obj_add_flag(warnings[i], LV_OBJ_FLAG_HIDDEN);  

    if (num_active_warnings > 0) {  
        char buffer[64];  
        if (current_warning >= num_active_warnings) current_warning = 0;
        int warn_idx = active_warnings[current_warning];

        switch (warn_idx) {
            case 0: snprintf(buffer, sizeof(buffer), "WARNING:     RPM HIGH!      %d", receivedData.rpm); break;
            case 1: snprintf(buffer, sizeof(buffer), "CRITICAL:    OIL PRESS      %.1f psi", receivedData.oilPressure); break;
            case 2: snprintf(buffer, sizeof(buffer), "LOW FUEL:     %.1f%%", receivedData.fuelLevelPercent); break;
            case 3: snprintf(buffer, sizeof(buffer), "SPEED ALERT:     %d km/h", receivedData.vehicleSpeed); break;
            case 4: snprintf(buffer, sizeof(buffer), "INJECTOR DUTY:   %.1f%%", receivedData.fuelDuty); break;
            case 5: snprintf(buffer, sizeof(buffer), "LOW VOLTAGE:     %.1f v", receivedData.batteryVoltage); break;
            case 6: snprintf(buffer, sizeof(buffer), "BOOST SPIKE!     %.1f kPa", receivedData.MAP); break;
            case 7: snprintf(buffer, sizeof(buffer), "RICH AFR:        %.3f", receivedData.lambdaA); break;
            case 8: snprintf(buffer, sizeof(buffer), "OVERHEATING!     %.1f °C", receivedData.coolantTemp); break;
            case 9: snprintf(buffer, sizeof(buffer), "LEAN AFR:        %.3f", receivedData.lambdaA); break;
            case 10: snprintf(buffer, sizeof(buffer), "OIL PRESS LOW:     %.1f psi", receivedData.oilPressure); break;
        }

        lv_label_set_text(warnings[warn_idx], buffer);
        lv_obj_remove_flag(warnings[warn_idx], LV_OBJ_FLAG_HIDDEN);

        // Prepare next warning for next blink cycle
        current_warning = (current_warning + 1) % num_active_warnings;
    }
}

void hide_warning_banner() {
    if (warning_banner) lv_obj_add_flag(warning_banner, LV_OBJ_FLAG_HIDDEN);
}