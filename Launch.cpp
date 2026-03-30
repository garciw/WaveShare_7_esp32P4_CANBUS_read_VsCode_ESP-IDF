#include "Launch.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <math.h>      // Better for fabs() if needed
#include "lvgl.h"
#include "ui.h"

// -------------------------------------------------------------------------
// LAUNCH CONTROL POPUP LOGIC (Panel Overlay)
// -------------------------------------------------------------------------
void Update_Launch_Control(const Maxxecu &data) {
    
    // 1. Check Status
    bool isActive = (data.launchControlActive == 1); 
    static bool wasActive = false;

    // ==========================================================
    // A. VISIBILITY TOGGLE (Show/Hide the Popup)
    // ==========================================================
    if (isActive != wasActive) {
        if (isActive) {
            // SHOW the panel (LVGL 9 prefers remove_flag)
            if(ui_PanelLaunch) lv_obj_remove_flag(ui_PanelLaunch, LV_OBJ_FLAG_HIDDEN);
        } else {
            // HIDE the panel
            if(ui_PanelLaunch) lv_obj_add_flag(ui_PanelLaunch, LV_OBJ_FLAG_HIDDEN);
        }
        wasActive = isActive;
    }

    // ==========================================================
    // B. EXIT IF NOT LAUNCHING (Saves CPU)
    // ==========================================================
    if (!isActive) {
        return; 
    }

    // ==========================================================
    // C. ACTIVE LAUNCH LOOP
    // ==========================================================
    
    // 2. TARGET MATCHING LOGIC (Green Border + Text)
    // Target is usually the Launch RPM set in MaxxECU
    int target = data.revLimitRPM; 
    int diff = abs((int)data.rpm - (int)target);
    static bool wasReady = false;
    
    // Threshold: 150 RPM window for "Ready" status
    bool isReady = (diff < 150);

    if (isReady != wasReady) {
        if (isReady) {
            // STATE: READY (Starlet is ready to rip)
            if(ui_PanelLaunch) lv_obj_set_style_border_color(ui_PanelLaunch, lv_color_hex(0x00FF00), LV_PART_MAIN); 
            if(ui_LabelLaunchReady) {
                lv_label_set_text(ui_LabelLaunchReady, "READY");
                lv_obj_set_style_text_color(ui_LabelLaunchReady, lv_color_hex(0x00FF00), LV_PART_MAIN);
            }
        } else {
            // STATE: SPOOLING (Building Boost)
            if(ui_PanelLaunch) lv_obj_set_style_border_color(ui_PanelLaunch, lv_color_hex(0xFF0000), LV_PART_MAIN); 
            if(ui_LabelLaunchReady) {
                lv_label_set_text(ui_LabelLaunchReady, "SPOOLING");
                lv_obj_set_style_text_color(ui_LabelLaunchReady, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
            }
        }
        wasReady = isReady;
    }

    // 3. Update Boost (PSI)
    static float lastBoostPsi = -1.0f;
    float boost_kpa = (float)data.MAP - 100.0f; // Barometric offset
    if (boost_kpa < 0) boost_kpa = 0;
    float currentBoostPsi = boost_kpa * 0.145038f; 

    // Only update label if value shifted by 0.1 PSI to prevent flicker
    if (fabs(currentBoostPsi - lastBoostPsi) > 0.1f) {
        if (ui_LabelLaunchBoost) {
            char buff[16];
            snprintf(buff, sizeof(buff), "%.1f psi", currentBoostPsi);
            lv_label_set_text(ui_LabelLaunchBoost, buff);
        }
        lastBoostPsi = currentBoostPsi;
    }
}