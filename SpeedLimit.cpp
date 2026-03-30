#include "SpeedLimit.h"
#include "lvgl.h"
#include "ui.h"
#include "globalVariables.h"
#include <stdio.h>
#include <string.h>

// 🛑 FIX 1: Added the missing ESP-IDF timer library
#include "esp_timer.h" 

// Replaces Arduino millis() with ESP-IDF high-res timer
#define millis() (uint32_t)(esp_timer_get_time() / 1000)

// 🛑 FIX 2: Removed 'lv_timer_t * timer' so it matches the main.cpp declaration exactly
void updateLocationData() {
    
    // 1. VISIBILITY CHECK
    // LVGL 9 recommendation: use lv_screen_active()
    if (lv_screen_active() != ui_Screen1) return;

    // ==========================================
    // STREET NAME UPDATE
    // ==========================================
    static char lastDisplayedName[32] = ""; 
    
    // Check if the MaxxECU/GPS data has actually changed
    if (strncmp(receivedData.displayName, lastDisplayedName, 31) != 0) {
        
        if (receivedData.displayName[0] != '\0') {
            // Valid Name -> Show it
            lv_label_set_text(ui_StreetName, receivedData.displayName);
            
            // Update local cache safely
            strncpy(lastDisplayedName, receivedData.displayName, sizeof(lastDisplayedName) - 1);
            lastDisplayedName[sizeof(lastDisplayedName) - 1] = '\0'; 
        } else {
            // Empty Name -> Show "Locating..."
            if (strcmp(lastDisplayedName, "Locating...") != 0) {
                lv_label_set_text(ui_StreetName, "Locating...");
                strncpy(lastDisplayedName, "Locating...", sizeof(lastDisplayedName) - 1);
            }
        }
    }

    // ==========================================
    // SPEED LIMIT UPDATE
    // ==========================================
    static int lastgpsSpeedLimit = -1;
    int currentLimit = receivedData.gpsSpeedLimit;

    // Only update the label if the speed limit changed (e.g., crossing zones)
    if (currentLimit != lastgpsSpeedLimit) {
        
        if (currentLimit > 0 && currentLimit < 200) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", currentLimit);
            lv_label_set_text(ui_LabelMaxSpeedLimit, buf);
        } else {
            // No limit found or invalid data
            lv_label_set_text(ui_LabelMaxSpeedLimit, "--");
        }
        
        lastgpsSpeedLimit = currentLimit;
    }
}