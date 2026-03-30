#include "WeatherDisplay.h"
#include "globalVariables.h"
#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "ui.h"

// ⛅ Import image assets
LV_IMAGE_DECLARE(ui_img_sunny_png);
LV_IMAGE_DECLARE(ui_img_cloud_png);
LV_IMAGE_DECLARE(ui_img_drizzle_png);
LV_IMAGE_DECLARE(ui_img_rain_png);
LV_IMAGE_DECLARE(ui_img_storm_png);
LV_IMAGE_DECLARE(ui_img_fog_png);
LV_IMAGE_DECLARE(ui_img_windy_png);

void updateWeatherDisplay() {
    
    // 1. VISIBILITY CHECK (LVGL 9 standard)
    if (lv_screen_active() != ui_Screen1) return;

    // Static variables to remember previous state
    static int lastTemp = -999;
    static int lastHum = -1;
    static char lastCond[64] = ""; 

    // ==========================================
    // 🌡️ UPDATE TEXT (Only if numbers change)
    // ==========================================
    if (receivedData.weatherTemp != lastTemp || receivedData.weatherHumidity != lastHum) {
        char labelText[32];
        // Ensure units are clearly formatted for the Hamilton, ON weather
        snprintf(labelText, sizeof(labelText), "%d°C | %d%%", 
                 receivedData.weatherTemp, receivedData.weatherHumidity);
        
        if (ui_weatherLabel) lv_label_set_text(ui_weatherLabel, labelText);
        
        lastTemp = receivedData.weatherTemp;
        lastHum = receivedData.weatherHumidity;
    }

    // ==========================================
    // 🌤️ UPDATE ICON (Only if string changes)
    // ==========================================
    if (strncmp(receivedData.weatherCond, lastCond, 63) != 0) {
        
        const void* icon = &ui_img_cloud_png; // Default fallback
        const char* cond = receivedData.weatherCond;

        // --- CONDITION LOGIC ---
        if (strstr(cond, "Clear"))        icon = &ui_img_sunny_png;
        else if (strstr(cond, "Cloud"))   icon = &ui_img_cloud_png;
        else if (strstr(cond, "Drizzle")) icon = &ui_img_drizzle_png;
        else if (strstr(cond, "Rain"))    icon = &ui_img_rain_png;
        else if (strstr(cond, "Thunder")) icon = &ui_img_storm_png;
        else if (strstr(cond, "Fog") || 
                 strstr(cond, "Mist") || 
                 strstr(cond, "Haze"))    icon = &ui_img_fog_png;
        else if (strstr(cond, "Wind"))    icon = &ui_img_windy_png;

        // LVGL 9: use lv_image_set_src
        if (ui_weatherImage) lv_image_set_src(ui_weatherImage, icon);

        // Update History safely
        strncpy(lastCond, receivedData.weatherCond, sizeof(lastCond) - 1);
        lastCond[sizeof(lastCond) - 1] = '\0'; 
    }
}