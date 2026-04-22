#include "Screen2.h"
#include "TorqueHorsepowerChart.h" 
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
// 🛠️ HELPER VARIABLES & STRUCTS
// ==========================================

char ecuLoggingText2[8]; 

void updateEcuLoggingText2() {
    snprintf(ecuLoggingText2, sizeof(ecuLoggingText2), "%s", receivedData.ecuLogging ? "ON" : "OFF");
}

typedef struct {
    const char* name;
    void* value;    
    char format[10]; 
} VariableMap2;

VariableMap2 variableMap2[] = {
    {"Lambda A", &receivedData.lambdaA, "%.2f"},
    {"Afr Target", &receivedData.lambdaTarget, "%.2f"},
    {"Afr", &receivedData.Afr, "%.2f"},
    {"Lambda Corr %", &receivedData.lambdaCorrA, "%.2f"},
    {"Total F Corr %", &receivedData.totalFuelTrim, "%.1f"},
    {"Boost Tar", &receivedData.boostTarget, "%.1f"},
    {"Oil Press kPa", &receivedData.oilPressure, "%.1f"},
    {"Fuel Press kPa", &receivedData.fuelPressure, "%.1f"},
    {"Boost kPa", &receivedData.MAP, "%.1f"},
    {"User CH1", &receivedData.userChannel1, "%.1f"},
    {"User CH2", &receivedData.userChannel2, "%.1f"},
    {"User CH3", &receivedData.userChannel3, "%.1f"},
    {"User CH4", &receivedData.userChannel4, "%.1f"},
    {"User CH5", &receivedData.userChannel5, "%.2f"},
    {"User CH6", &receivedData.userChannel6, "%.1f"},
    {"ECU Logging", ecuLoggingText2, "%s"},  
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


// ==========================================
// 🛠️ HELPER FUNCTIONS
// ==========================================

void updateSlotValue2(const char* selectedItem, char* outputBuffer, size_t bufferSize) {
    memset(outputBuffer, 0, bufferSize);
    
    if (selectedItem == NULL) return;

    if (strcmp(selectedItem, "Iat Temp °C") == 0) {
        if (receivedData.intakeAirTemp > 200.0) {
             snprintf(outputBuffer, bufferSize, "%.1f", receivedData.intakeAirTemp / 100.0);
        } else {
             snprintf(outputBuffer, bufferSize, "%.1f", receivedData.intakeAirTemp);
        }
        return; 
    }

    for (int i = 0; i < (int)(sizeof(variableMap2) / sizeof(variableMap2[0])); i++) {
        if (strcmp(selectedItem, variableMap2[i].name) == 0) {

            if (strcmp(variableMap2[i].name, "Iat Temp °C") == 0) {
                 snprintf(outputBuffer, bufferSize, "%.1f", receivedData.intakeAirTemp);
            }
            
            if (strcmp(variableMap2[i].name, "ECU Logging") == 0) {
                 updateEcuLoggingText2(); 
                 snprintf(outputBuffer, bufferSize, "%s", ecuLoggingText2);
            } 
            else if (strcmp(variableMap2[i].format, "%d") == 0) {
                snprintf(outputBuffer, bufferSize, variableMap2[i].format, *(int16_t*)variableMap2[i].value);
            } 
            else {
                snprintf(outputBuffer, bufferSize, variableMap2[i].format, *(float*)variableMap2[i].value);
            }
            return;
        }
    }
}

void updateSlot2(lv_obj_t *slotLabel2, const char* selectedItem) {
    if (slotLabel2 == NULL || selectedItem == NULL) return;

    char newValue[32];
    updateSlotValue2(selectedItem, newValue, sizeof(newValue));
    lv_label_set_text(slotLabel2, newValue);

    if (strcmp(selectedItem, "ECU Logging") == 0) {
        if (receivedData.ecuLogging) {
            lv_obj_set_style_text_color(slotLabel2, lv_color_hex(0x00FF00), 0); 
        } else {
            lv_obj_set_style_text_color(slotLabel2, lv_color_hex(0xFFFFFF), 0); 
        }
    } 
    else if (strcmp(selectedItem, "Coolant °C") == 0) {
        if (receivedData.coolantTemp >= 105.0) {
            lv_obj_set_style_text_color(slotLabel2, lv_color_hex(0xFF2424), 0); 
        } else {
            lv_obj_set_style_text_color(slotLabel2, lv_color_hex(0xFFFFFF), 0); 
        }
    }
    else {
        lv_obj_set_style_text_color(slotLabel2, lv_color_hex(0xFFFFFF), 0);
    }
}

void dropdown_event_cb2(lv_event_t* e) {
    // FIXED: Added explicit cast (lv_obj_t*)
    lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* slot = (lv_obj_t*)lv_event_get_user_data(e);
    
    char selectedItem[32];
    lv_dropdown_get_selected_str(dropdown, selectedItem, sizeof(selectedItem));
    updateSlot2(slot, selectedItem);
}

void setupScreen2() {
    lv_obj_add_event_cb(ui_Dropdown6, dropdown_event_cb2, LV_EVENT_VALUE_CHANGED, ui_Slot6);
    lv_obj_add_event_cb(ui_Dropdown7, dropdown_event_cb2, LV_EVENT_VALUE_CHANGED, ui_Slot7);
    lv_obj_add_event_cb(ui_Dropdown8, dropdown_event_cb2, LV_EVENT_VALUE_CHANGED, ui_Slot8);
    lv_obj_add_event_cb(ui_Dropdown9, dropdown_event_cb2, LV_EVENT_VALUE_CHANGED, ui_Slot9);
    lv_obj_add_event_cb(ui_Dropdown10, dropdown_event_cb2, LV_EVENT_VALUE_CHANGED, ui_Slot10);

    updateSlot2(ui_Slot6, "Lambda A");
    updateSlot2(ui_Slot7, "Boost kPa");
    updateSlot2(ui_Slot8, "Error Codes");
    updateSlot2(ui_Slot9, "Coolant °C");
    updateSlot2(ui_Slot10, "Battery V+");
    updateEcuLoggingText2();
    
    initialize_chart_series();  
    start_chart_timer();
}

void updateDisplay2() {
    static int lastGear = -1;
    int currentGear = receivedData.gear; 
    if (currentGear < 0 || currentGear > 5) currentGear = 0; 

    if (currentGear != lastGear) {
        if (ui_GEAR3) lv_roller_set_selected(ui_GEAR3, currentGear, LV_ANIM_ON);
        lastGear = currentGear;
    }

    static uint32_t lastSlowUpdate = 0;
    if (millis() - lastSlowUpdate < 200) return; 
    lastSlowUpdate = millis();

    static float lastFuelLevel = -1.0;
    float currentFuelLevel = receivedData.fuelLevelPercent;

    // FIXED: Use fabs() for floating point
    if (fabs(currentFuelLevel - lastFuelLevel) > 0.5) {
        char Fuel_text[32];
        snprintf(Fuel_text, sizeof(Fuel_text), "%.0f%%", currentFuelLevel);
        if (ui_FUEL1) lv_label_set_text(ui_FUEL1, Fuel_text);
        
        if (ui_FUELBAR1) {
            lv_bar_set_value(ui_FUELBAR1, (int)currentFuelLevel, LV_ANIM_OFF);
            
            if (currentFuelLevel <= 15) {
                 lv_obj_set_style_bg_color(ui_FUELBAR1, lv_color_hex(0xFF2424), LV_PART_INDICATOR);
            } else {
                 lv_obj_set_style_bg_color(ui_FUELBAR1, lv_color_hex(0x0BF438), LV_PART_INDICATOR);
            }
        }
        lastFuelLevel = currentFuelLevel;
    }

    static char lastSlotValues[5][32]; 
    lv_obj_t* dropdownArray[] = {ui_Dropdown6, ui_Dropdown7, ui_Dropdown8, ui_Dropdown9, ui_Dropdown10};
    lv_obj_t* slotArray[] = {ui_Slot6, ui_Slot7, ui_Slot8, ui_Slot9, ui_Slot10};

    for (int i = 0; i < 5; i++) {
        if (!dropdownArray[i] || !slotArray[i]) continue;

        char selectedItem[32];
        lv_dropdown_get_selected_str(dropdownArray[i], selectedItem, sizeof(selectedItem));

        char newValue[32];
        updateSlotValue2(selectedItem, newValue, sizeof(newValue));

        if (strcmp(newValue, lastSlotValues[i]) != 0) { 
            lv_label_set_text(slotArray[i], newValue);
            strcpy(lastSlotValues[i], newValue);

            if (strcmp(selectedItem, "ECU Logging") == 0) {
                 if (receivedData.ecuLogging) lv_obj_set_style_text_color(slotArray[i], lv_color_hex(0x00FF00), 0); 
                 else lv_obj_set_style_text_color(slotArray[i], lv_color_hex(0xFFFFFF), 0); 
            } 
            else if (strcmp(selectedItem, "Coolant °C") == 0) {
                 if (receivedData.coolantTemp >= 105.0) lv_obj_set_style_text_color(slotArray[i], lv_color_hex(0xFF2424), 0); 
                 else lv_obj_set_style_text_color(slotArray[i], lv_color_hex(0xFFFFFF), 0); 
            }
            else {
                 lv_obj_set_style_text_color(slotArray[i], lv_color_hex(0xFFFFFF), 0);
            }
        }
    }
}

void updateScreen2Speed() {
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
        if (ui_VSS1) lv_label_set_text(ui_VSS1, vss_text);                  
        if (ui_speedunit2) lv_label_set_text(ui_speedunit2, useMPH ? "MPH" : "km/h"); 
        
        lastRenderedSpeed = finalSpeed;
        lastUnitState = useMPH; 
    }
}