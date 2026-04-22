#include "Screen1.h"
#include "globalVariables.h"
#include "SpeedUnitControl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>       
#include "esp_timer.h" 
#include "lvgl.h"
#include "ui.h"

// Replaces Arduino millis() with ESP-IDF high-res timer
#define millis() (uint32_t)(esp_timer_get_time() / 1000)

// ==========================================
// 🛠️ 1. HELPER LOGIC
// ==========================================

char ecuLoggingText[8]; 

void updateEcuLoggingText() {
    snprintf(ecuLoggingText, sizeof(ecuLoggingText), "%s", receivedData.ecuLogging ? "ON" : "OFF");
}

typedef struct {
    const char* name;
    void* value;    
    char format[10]; 
} VariableMap;

VariableMap variableMap[] = {
    {"Lambda A", &receivedData.lambdaA, "%.2f"},
    {"Afr Target", &receivedData.lambdaTarget, "%.2f"},
    {"Afr", &receivedData.Afr, "%.2f"},
    {"Lambda Corr %", &receivedData.lambdaCorrA, "%.2f"},
    {"Total F Corr %", &receivedData.totalFuelTrim, "%.1f"},
    {"Boost Tar", &receivedData.boostTarget, "%.1f"},
    {"Oil Press kPa", &receivedData.oilPressure, "%.1f"},
    {"Fuel Press kPa", &receivedData.userChannel1, "%.1f"},     
    {"Boost kPa", &receivedData.MAP, "%.1f"},
    {"User CH1", &receivedData.userChannel1, "%.1f"},
    {"User CH2", &receivedData.userChannel2, "%.1f"},
    {"User CH3", &receivedData.userChannel3, "%.1f"},
    {"User CH4", &receivedData.userChannel4, "%.1f"},
    {"User CH5", &receivedData.userChannel5, "%.2f"},
    {"User CH6", &receivedData.userChannel6, "%.1f"},
    {"ECU Logging", ecuLoggingText, "%s"},  
    {"User CH7", &receivedData.userChannel7, "%.1f"},
    {"User CH8", &receivedData.userChannel8, "%.1f"},
    {"User CH9", &receivedData.userChannel9, "%.1f"},
    {"User CH10", &receivedData.userChannel10, "%.1f"},
    {"User CH11", &receivedData.userChannel11, "%.1f"},
    {"User CH12", &receivedData.userChannel12, "%.1f"},
    {"Coolant °C", &receivedData.coolantTemp, "%.1f"},
    {"Iat Temp °C", &receivedData.intakeAirTemp, "%.1f"},
    {"CPU Temp °C", &receivedData.cpuTemp, "%d"},
    {"Throttle%", &receivedData.throttlePos, "%.1f"},
    {"Baro kPa", &receivedData.baroPressure, "%.1f"},
    {"Battery V+", &receivedData.batteryVoltage, "%.1f"},
    {"Rev Limiter", &receivedData.revLimitRPM, "%d"},
    {"Boost Duty %", &receivedData.boostSolenoidDuty, "%.1f"},
    {"Error Codes", &receivedData.errorCodeCount, "%d"},
    {"Fuel Tank", &receivedData.fuelLevelPercent, "%.1f"} 
};

static void set_rpm_label_style(lv_obj_t* label, bool active, bool isHighRPM) {
    if (!label) return; 
    if (active) {
        lv_obj_set_style_text_color(label, isHighRPM ? lv_color_hex(0xFF2424) : lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &ui_font_orbitron40, LV_PART_MAIN);  
    } else {
        lv_obj_set_style_text_font(label, &ui_font_orbitron25, LV_PART_MAIN);  
    }
}

void updateSlotValue(const char* selectedItem, char* outputBuffer, size_t bufferSize) {
    memset(outputBuffer, 0, bufferSize);
    if (selectedItem == NULL) return;

    if (strcmp(selectedItem, "Iat Temp °C") == 0) {
        float iat = receivedData.intakeAirTemp;
        if (iat > 200.0f) iat /= 100.0f;
        snprintf(outputBuffer, bufferSize, "%.1f", iat);
        return; 
    }

    for (int i = 0; i < (int)(sizeof(variableMap) / sizeof(variableMap[0])); i++) {
        if (strcmp(selectedItem, variableMap[i].name) == 0) {
            if (strcmp(variableMap[i].name, "ECU Logging") == 0) {
                 updateEcuLoggingText(); 
                 snprintf(outputBuffer, bufferSize, "%s", ecuLoggingText);
            } else if (strcmp(variableMap[i].format, "%d") == 0) {
                snprintf(outputBuffer, bufferSize, variableMap[i].format, *(int16_t*)variableMap[i].value);
            } else {
                snprintf(outputBuffer, bufferSize, variableMap[i].format, *(float*)variableMap[i].value);
            }
            return;
        }
    }
}

// ==========================================
// 🚀 2. UI ACTION FUNCTIONS
// ==========================================

// Renamed to updateSlotInternal to avoid conflict with SquareLine's updateSlot
void updateSlotInternal(lv_obj_t *slotLabel, const char* selectedItem) {
    if (slotLabel == NULL || selectedItem == NULL) return;

    char newValue[32];
    updateSlotValue(selectedItem, newValue, sizeof(newValue)); 
    lv_label_set_text(slotLabel, newValue);

    if (strcmp(selectedItem, "ECU Logging") == 0) {
        lv_obj_set_style_text_color(slotLabel, receivedData.ecuLogging ? lv_color_hex(0x00FF00) : lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(slotLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    }
}

void dropdown_event_cb(lv_event_t* e) {
    lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* slot = (lv_obj_t*)lv_event_get_user_data(e);
    
    char selectedItem[32];
    lv_dropdown_get_selected_str(dropdown, selectedItem, sizeof(selectedItem));
    updateSlotInternal(slot, selectedItem);
}

void setupScreen1() {
    lv_obj_add_event_cb(ui_Dropdown1, dropdown_event_cb, LV_EVENT_VALUE_CHANGED, ui_Slot1);
    lv_obj_add_event_cb(ui_Dropdown2, dropdown_event_cb, LV_EVENT_VALUE_CHANGED, ui_Slot2);
    lv_obj_add_event_cb(ui_Dropdown3, dropdown_event_cb, LV_EVENT_VALUE_CHANGED, ui_Slot3);
    lv_obj_add_event_cb(ui_Dropdown4, dropdown_event_cb, LV_EVENT_VALUE_CHANGED, ui_Slot4);
    lv_obj_add_event_cb(ui_Dropdown5, dropdown_event_cb, LV_EVENT_VALUE_CHANGED, ui_Slot5);

    // Fixed: Call the internal function
    updateSlotInternal(ui_Slot1, "Lambda A");
    updateSlotInternal(ui_Slot2, "Boost kPa");
    updateSlotInternal(ui_Slot3, "Error Codes");
    updateSlotInternal(ui_Slot4, "Coolant °C");
    updateSlotInternal(ui_Slot5, "Battery V+");
    updateEcuLoggingText();
}

// ==========================================
// 🚀 OPTIMIZED UPDATE DISPLAY
// ==========================================
void updateDisplay() {
    static int lastRPM = -1;
    int currentRPM = receivedData.rpm; 
    
    if (currentRPM != lastRPM) {
        if (ui_RPMBAR1) lv_bar_set_value(ui_RPMBAR1, currentRPM, LV_ANIM_OFF);
        
        static bool isRedline = false; 
        int limit = (receivedData.revLimitRPM < 1000) ? 7200 : receivedData.revLimitRPM;
        bool hittingLimit = (currentRPM >= limit - 50) || receivedData.revLimitActive;

        if (hittingLimit) {
            if (!isRedline && ui_RPMBAR1) {
                lv_obj_set_style_bg_color(ui_RPMBAR1, lv_color_hex(0xFF2424), LV_PART_INDICATOR);
                isRedline = true;
            }
        } else {
            if (isRedline && ui_RPMBAR1) {
                lv_obj_set_style_bg_color(ui_RPMBAR1, lv_color_hex(0x3592DF), LV_PART_INDICATOR); 
                isRedline = false;
            }
        }

        static int lastMilestone = -1;
        int currentMilestone = currentRPM / 1000; 
        
        if (currentMilestone != lastMilestone) {
            set_rpm_label_style(ui_rpm1k, (currentMilestone == 1), false);
            set_rpm_label_style(ui_rpm2k, (currentMilestone == 2), false);
            set_rpm_label_style(ui_rpm3k, (currentMilestone == 3), false);
            set_rpm_label_style(ui_rpm4k, (currentMilestone == 4), false);
            set_rpm_label_style(ui_rpm5k, (currentMilestone == 5), false);
            set_rpm_label_style(ui_rpm6k, (currentMilestone == 6), true);
            set_rpm_label_style(ui_rpm7k, (currentMilestone == 7), true);
            set_rpm_label_style(ui_rpm8k, (currentMilestone == 8), true);
            lastMilestone = currentMilestone;
        }
        lastRPM = currentRPM;
    }

    static float lastFuelLevel = -1.0; 
    float currentFuelLevel = receivedData.fuelLevelPercent;

    if (fabs(currentFuelLevel - lastFuelLevel) > 0.5) {
        char Fuel_text[32];
        snprintf(Fuel_text, sizeof(Fuel_text), "%.0f%%", currentFuelLevel);
        if (ui_FUEL) lv_label_set_text(ui_FUEL, Fuel_text);
        
        if (ui_FUELBAR3) {
            lv_bar_set_value(ui_FUELBAR3, (int)currentFuelLevel, LV_ANIM_OFF);
            if (currentFuelLevel <= 15) {
                 lv_obj_set_style_bg_color(ui_FUELBAR3, lv_color_hex(0xFF2424), LV_PART_INDICATOR);
            } else {
                 lv_obj_set_style_bg_color(ui_FUELBAR3, lv_color_hex(0x0BF438), LV_PART_INDICATOR);
            }
        }
        lastFuelLevel = currentFuelLevel; 
    }

    static uint32_t lastSlowUpdate = 0;
    if (millis() - lastSlowUpdate < 200) return; 
    lastSlowUpdate = millis();

    static int lastGear = -1;
    int currentGear = receivedData.gear;
    if (currentGear < 0 || currentGear > 5) currentGear = 0; 

    if (currentGear != lastGear) {
        if (ui_GEAR1) lv_roller_set_selected(ui_GEAR1, currentGear, LV_ANIM_ON);
        int nextGear = (currentGear < 5) ? currentGear + 1 : 5;
        if (ui_GEAR2) lv_roller_set_selected(ui_GEAR2, nextGear, LV_ANIM_OFF);
        lastGear = currentGear;
    }

    static bool parkFlashState = false;
    
    if (receivedData.parkingBrakeActive) {
        if (receivedData.vehicleSpeed > 3) { 
            parkFlashState = !parkFlashState; 
            if (ui_PARKING) {
                if (parkFlashState) lv_obj_clear_flag(ui_PARKING, LV_OBJ_FLAG_HIDDEN);
                else lv_obj_add_flag(ui_PARKING, LV_OBJ_FLAG_HIDDEN);
            }
        } else {
            if (ui_PARKING) lv_obj_clear_flag(ui_PARKING, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        if (ui_PARKING) lv_obj_add_flag(ui_PARKING, LV_OBJ_FLAG_HIDDEN);
    }
    
    static bool lastOil = false;
    if (receivedData.oilPressureWarning != lastOil) {
         if (ui_LOWOIL) {
             if (receivedData.oilPressureWarning) lv_obj_clear_flag(ui_LOWOIL, LV_OBJ_FLAG_HIDDEN);
             else lv_obj_add_flag(ui_LOWOIL, LV_OBJ_FLAG_HIDDEN);
         }
         lastOil = receivedData.oilPressureWarning;
    }

    static bool fuelIconVisible = false;
    if (receivedData.fuelLowWarning) {
        fuelIconVisible = !fuelIconVisible; 
        if (ui_LOWFUEL) {
            if(fuelIconVisible) lv_obj_clear_flag(ui_LOWFUEL, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(ui_LOWFUEL, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        if (ui_LOWFUEL) lv_obj_add_flag(ui_LOWFUEL, LV_OBJ_FLAG_HIDDEN);
    }

    static char lastSlotValues[5][32]; 
    lv_obj_t* dropdownArray[] = {ui_Dropdown1, ui_Dropdown2, ui_Dropdown3, ui_Dropdown4, ui_Dropdown5};
    lv_obj_t* slotArray[] = {ui_Slot1, ui_Slot2, ui_Slot3, ui_Slot4, ui_Slot5};

    for (int i = 0; i < 5; i++) {
        if (!dropdownArray[i] || !slotArray[i]) continue;

        char selectedItem[32];
        lv_dropdown_get_selected_str(dropdownArray[i], selectedItem, sizeof(selectedItem));
        
        char newValue[32];
        updateSlotValue(selectedItem, newValue, sizeof(newValue));
        
        if (strcmp(newValue, lastSlotValues[i]) != 0) { 
            lv_label_set_text(slotArray[i], newValue);
            strcpy(lastSlotValues[i], newValue);
        }

        if (strcmp(selectedItem, "ECU Logging") == 0) {
             if (receivedData.ecuLogging) lv_obj_set_style_text_color(slotArray[i], lv_color_hex(0x00FF00), 0); 
             else lv_obj_set_style_text_color(slotArray[i], lv_color_hex(0xFFFFFF), 0); 
        } 
        else if (strcmp(selectedItem, "Coolant °C") == 0) {
             if (receivedData.coolantTemp < 65.0) lv_obj_set_style_text_color(slotArray[i], lv_color_hex(0x00AAFF), 0); 
             else if (receivedData.coolantTemp >= 105.0) lv_obj_set_style_text_color(slotArray[i], lv_color_hex(0xFF2424), 0); 
             else lv_obj_set_style_text_color(slotArray[i], lv_color_hex(0xFFFFFF), 0); 
        }
        else {
             lv_obj_set_style_text_color(slotArray[i], lv_color_hex(0xFFFFFF), 0);
        }
    }
}

void updateWiFiStatusIcon() {
    static uint32_t lastWifiCheck = 0;
    if (millis() - lastWifiCheck < 1000) return; 
    lastWifiCheck = millis();

    static bool lastWifi = false;
    if (receivedData.wifiConnected != lastWifi) {
        if (ui_CASTWIFI) {
            if (receivedData.wifiConnected) lv_obj_clear_flag(ui_CASTWIFI, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(ui_CASTWIFI, LV_OBJ_FLAG_HIDDEN);
        }
        lastWifi = receivedData.wifiConnected;
    }
}

void updateScreen1Speed() {
    int rawSpeed = receivedData.vehicleSpeed;  
    if (useMPH) rawSpeed /= 1.60934; 

    static float displayedSpeed = 0.0; 
    static bool lastUnitState = !useMPH; 
    float smoothingFactor = 0.5; 
    
    float diff = rawSpeed - displayedSpeed;
    if (fabs(diff) < 0.5) displayedSpeed = (float)rawSpeed;
    else displayedSpeed += diff * smoothingFactor;

    int finalSpeed = (int)round(displayedSpeed);
    static int lastRenderedSpeed = -1; 

    if (finalSpeed != lastRenderedSpeed || useMPH != lastUnitState) {    
        char vss_text[16];
        snprintf(vss_text, sizeof(vss_text), "%d", finalSpeed);
        if (ui_VSS) lv_label_set_text(ui_VSS, vss_text);                  
        if (ui_speedunit1) lv_label_set_text(ui_speedunit1, useMPH ? "MPH" : "km/h"); 
        
        lastRenderedSpeed = finalSpeed;
        lastUnitState = useMPH; 
    }
}