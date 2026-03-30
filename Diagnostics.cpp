#include "Diagnostics.h"
#include "lvgl.h"
#include "ui.h"
#include "globalVariables.h"
#include <stdio.h>
#include "esp_timer.h" // Replaces Arduino millis()

// Define a timer for blinking
uint32_t lastBlinkTime = 0; // ESP-IDF change
bool alertVisible = false;

// ---------------------------------------------------------
// HELPER: Convert Code to Text (Efficiently)
// ---------------------------------------------------------
const char* getEcureturn(int code) {
    switch (code) {
        case 11: return "IAT sensor error"; break;
        case 12: return "CLT sensor error"; break;
        case 13: return "TPS error"; break;
        case 14: return "E-Throttle overtemperature/short circuit"; break;
        case 15: return "VVT position sensor error"; break;
        case 16: return "VVT control error"; break;
        case 17: return "5V output error"; break;
        case 18: return "12V2 voltage missing"; break;
        case 19: return "Electromagnetic interference detected"; break;
        case 21: return "Incorrect fuel/ignition-mode"; break;
        case 22: return "Wrong cylinder count with current fuel/ignition-mode"; break;
        case 23: return "O2 sensor: Reference short circuit"; break;
        case 24: return "O2 sensor: Incorrect drive signal"; break;
        case 25: return "O2 sensor heating error"; break;
        case 26: return "O2 sensor too cold"; break;
        case 27: return "O2 sensor too hot"; break;
        case 28: return "Flex fuel sensor error"; break;
        case 29: return "MAP sensor error"; break;
        case 32: return "Trigger error: Timeout"; break;
        case 33: return "Trigger error: Too few trigger-teeth before sync signal"; break;
        case 34: return "Trigger error: Too many trigger-teeth before sync signal"; break;
        case 35: return "Trigger error: Trigger signal too early"; break;
        case 36: return "Trigger error: Trigger signal too late"; break;
        case 37: return "Trigger error: Error in trigger pattern"; break;
        case 38: return "Trigger/CAM pattern expected error"; break;
        case 39: return "Trigger error: No cam signal / incorrect pattern"; break;
        case 41: return "Boost limit activated"; break;
        case 42: return "Lean power cut"; break;
        case 43: return "EGT power cut"; break;
        case 44: return "Warning system: Level 1 reached"; break;
        case 45: return "Warning system: Level 2 reached"; break;
        case 46: return "N2O Protection Activated"; break;
        case 47: return "External lambda sensor error"; break;
        case 48: return "Overvoltage protection cut"; break;
        case 51: return "High load on injector output"; break;
        case 54: return "O2 sensor: Invalid pump signal (sensor 2)"; break;
        case 55: return "O2 sensor heating error (sensor 2)"; break;
        case 56: return "O2 sensor too cold (sensor 2)"; break;
        case 57: return "O2 sensor too hot (sensor 2)"; break;
        case 61: return "E-Throttle: TPS 1 error"; break;
        case 62: return "E-Throttle: Pedal error"; break;
        case 63: return "E-Throttle: Position control error throttle 1"; break;
        case 64: return "E-Throttle: TPS 2 error"; break;
        case 65: return "E-Throttle: Position control error throttle 2"; break;
        case 81: return "Water/Methanol Injection, low level"; break;
        case 82: return "CAN-bus: No connection with input module"; break;
        case 83: return "CAN-bus: No connection with PWM module"; break;
        case 84: return "CAN-bus: PWM module, over temperature or overcurrent"; break;
        case 85: return "CAN-bus: No connection with E-Throttle module"; break;
        case 86: return "CAN-bus: No connection with Traction module"; break;
        case 87: return "CAN-bus: No connection with PDM module"; break;
        case 91: return "MaxxECU internal error: I2C-error"; break;
        case 92: return "MaxxECU internal error: High internal temperature"; break;
        case 93: return "MaxxECU internal error: Debug error"; break;
        case 94: return "MaxxECU internal error: Slave CPU error"; break;
        case 95: return "Table CRC error"; break;
        case 97: return "Hard fault / Memory protection"; break;
        case 98: return "MaxxECU internal error: Watchdog reset"; break;
        case 99: return "MaxxECU internal error: Bootloader error"; break;
        default: return "Unknown Error";
    }
}

// ---------------------------------------------------------
// MAIN CHECK LOOP
// ---------------------------------------------------------
void checkECUErrors() {
    int ecuError = receivedData.ecuErrorCodes;  
    static int lastError = -1;

    // 1. NO ERROR LOGIC
    if (ecuError == 0) {
        // Only update UI if we *just* switched from Error to No-Error
        if (lastError != 0) {
            // Hide Alert
            if (!lv_obj_has_flag(ui_MainAlertPanel, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_add_flag(ui_MainAlertPanel, LV_OBJ_FLAG_HIDDEN);
            }
            // Reset Text
            if (ui_ErrorCodeLabel) {
                lv_label_set_text(ui_ErrorCodeLabel, "No Errors - All Systems OK");
            }
            lastError = 0;
        }
        return; 
    }

    uint32_t now = esp_timer_get_time() / 1000;

    // 2. BLINKING ALERT (Runs every 600ms)
    // This handles the flashing Red Box on the main screen
    if (now - lastBlinkTime > 600) {
        lastBlinkTime = now;
        alertVisible = !alertVisible;
            
        if (alertVisible) {
            lv_obj_clear_flag(ui_MainAlertPanel, LV_OBJ_FLAG_HIDDEN); // Show
                
            // Update the small alert text
            if (ui_MainAlertText) {
                char smallBuf[32];
                snprintf(smallBuf, sizeof(smallBuf), "Warning: P%d", ecuError);
                lv_label_set_text(ui_MainAlertText, smallBuf);
            }
        } else {
            lv_obj_add_flag(ui_MainAlertPanel, LV_OBJ_FLAG_HIDDEN); // Hide
        }
    }

    // 3. FULL ERROR TEXT (Runs ONLY when error code changes)
    // This saves massive CPU by not running the switch statement every frame
    if (ecuError != lastError) {
        if (ui_ErrorCodeLabel) {
            const char* msg = getEcureturn(ecuError);
            char fullBuf[256];  //128 for short string
            snprintf(fullBuf, sizeof(fullBuf), "Error P%d: %s", ecuError, msg);
            lv_label_set_text(ui_ErrorCodeLabel, fullBuf);
        }
        lastError = ecuError;
    }
}


// to be tested ✅