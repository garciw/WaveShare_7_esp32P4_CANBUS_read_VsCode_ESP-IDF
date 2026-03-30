#include "ControlHub.h"
#include "esp_timer.h"
#include "driver/uart.h"   // Replaces HardwareSerial
#include "nvs_flash.h"     // Replaces Preferences
#include "nvs.h"           // Replaces Preferences

// Initialize default state
ControlPacket outgoingControl = {0, 50, false, false};

// NETWORK OPTIMIZATION FLAG
bool dataChanged = false; 

// ======================================================
// 1. SCREEN SWITCHING LOGIC
// ======================================================
/*
void open_hub_handler(lv_event_t * e) {
    // 100ms fade is fast enough to feel "instant" but hides the DSI rendering lag
    lv_screen_load(ui_Screen6);
}

void close_hub_handler(lv_event_t * e) {
    lv_screen_load(ui_Screen1);
}
*/
// ======================================================
// 2. BOOST MODE LOGIC
// ======================================================
void set_boost_mode(int mode) {
    if (outgoingControl.boostMode != mode) {
        outgoingControl.boostMode = mode;
        
        // 🛑 ESP-IDF NVS SAVE (Replaces Preferences)
        nvs_handle_t my_handle;
        if (nvs_open("maxxecu", NVS_READWRITE, &my_handle) == ESP_OK) {
            nvs_set_i32(my_handle, "boostMode", (int32_t)mode);
            nvs_commit(my_handle);
            nvs_close(my_handle);
        }
        
        dataChanged = true; 
    }

    lv_color_t color_OFF = lv_color_hex(0x202020); 
    lv_color_t color_ON  = lv_color_hex(0xFF9000); 
    lv_color_t text_OFF  = lv_color_hex(0x808080); 
    lv_color_t text_ON   = lv_color_hex(0xFFFFFF); 

    lv_obj_set_style_bg_color(ui_LightStreet, color_OFF, 0);
    lv_obj_set_style_shadow_width(ui_LightStreet, 0, 0); 
    lv_obj_set_style_text_color(ui_LabelStreet, text_OFF, 0); 
    
    lv_obj_set_style_bg_color(ui_LightSport, color_OFF, 0);
    lv_obj_set_style_shadow_width(ui_LightSport, 0, 0);
    lv_obj_set_style_text_color(ui_LabelSport, text_OFF, 0); 

    lv_obj_set_style_bg_color(ui_LightRace, color_OFF, 0);
    lv_obj_set_style_shadow_width(ui_LightRace, 0, 0);
    lv_obj_set_style_text_color(ui_LabelRace, text_OFF, 0); 

    lv_obj_t* activeLight = NULL;
    lv_obj_t* activeLabel = NULL;
    
    if (mode == 0) { activeLight = ui_LightStreet; activeLabel = ui_LabelStreet; }
    if (mode == 1) { activeLight = ui_LightSport;  activeLabel = ui_LabelSport; }
    if (mode == 2) { activeLight = ui_LightRace;   activeLabel = ui_LabelRace; }

    if (activeLight) {
        lv_obj_set_style_bg_color(activeLight, color_ON, 0);
        lv_obj_set_style_shadow_color(activeLight, color_ON, 0);
        lv_obj_set_style_shadow_width(activeLight, 20, 0); 
        lv_obj_set_style_text_color(activeLabel, text_ON, 0); 
    }
}

// ======================================================
// 3. EVENT CALLBACKS
// ======================================================

void ui_event_SliderTraction(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    int val = lv_slider_get_value(ui_SliderTraction);

    if(code == LV_EVENT_VALUE_CHANGED) {
        if (outgoingControl.tractionVal != val) {
            outgoingControl.tractionVal = val;
            dataChanged = true; 
        }
        lv_label_set_text_fmt(ui_LabelTractionValue, "%d%%", val);
    }

    if(code == LV_EVENT_RELEASED) {
        outgoingControl.tractionVal = val; 
        
        // 🛑 ESP-IDF NVS SAVE
        nvs_handle_t my_handle;
        if (nvs_open("maxxecu", NVS_READWRITE, &my_handle) == ESP_OK) {
            nvs_set_i32(my_handle, "traction", (int32_t)val);
            nvs_commit(my_handle);
            nvs_close(my_handle);
        }
        
        dataChanged = true; 
    }
}

void ui_event_ScrambleBtn(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    
    if(event_code == LV_EVENT_PRESSED) {
        if (!outgoingControl.scrambleActive) {
            outgoingControl.scrambleActive = true;
            dataChanged = true; 
        }
    }
    if(event_code == LV_EVENT_RELEASED || event_code == LV_EVENT_PRESS_LOST) {
        if (outgoingControl.scrambleActive) {
            outgoingControl.scrambleActive = false;
            dataChanged = true; 
        }
    }
}

void ui_event_ValetSwitch(lv_event_t * e) {
    bool newState = lv_obj_has_state(ui_ValetSwitch, LV_STATE_CHECKED);
    if (outgoingControl.valetActive != newState) {
        outgoingControl.valetActive = newState;
        
        // 🛑 ESP-IDF NVS SAVE (Booleans are saved as 8-bit ints in NVS)
        nvs_handle_t my_handle;
        if (nvs_open("maxxecu", NVS_READWRITE, &my_handle) == ESP_OK) {
            nvs_set_u8(my_handle, "valet", newState ? 1 : 0);
            nvs_commit(my_handle);
            nvs_close(my_handle);
        }
        
        dataChanged = true; 
    }
}

// ======================================================
// 4. SETUP FUNCTION
// ======================================================
void setupControlHub() {
    
    // 🛑 ESP-IDF NVS LOAD ALL SETTINGS
    int savedTraction = 50; 
    int savedBoost = 0; 
    bool savedValet = false;
    
    nvs_handle_t my_handle;
    if (nvs_open("maxxecu", NVS_READONLY, &my_handle) == ESP_OK) {
        int32_t tempI32;
        uint8_t tempU8;
        
        if (nvs_get_i32(my_handle, "traction", &tempI32) == ESP_OK) savedTraction = (int)tempI32;
        if (nvs_get_i32(my_handle, "boostMode", &tempI32) == ESP_OK) savedBoost = (int)tempI32;
        if (nvs_get_u8(my_handle, "valet", &tempU8) == ESP_OK) savedValet = (tempU8 == 1);
        
        nvs_close(my_handle);
    }
    
    lv_slider_set_value(ui_SliderTraction, savedTraction, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui_LabelTractionValue, "%d%%", savedTraction);
    outgoingControl.tractionVal = savedTraction;

    outgoingControl.valetActive = savedValet;
    if (savedValet) {
        lv_obj_add_state(ui_ValetSwitch, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(ui_ValetSwitch, LV_STATE_CHECKED);
    }

    //lv_obj_add_event_cb(ui_Button7, open_hub_handler, LV_EVENT_SHORT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_BtnStreet, [](lv_event_t* e){ set_boost_mode(0); }, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_BtnSport,  [](lv_event_t* e){ set_boost_mode(1); }, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_BtnRace,   [](lv_event_t* e){ set_boost_mode(2); }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_SliderTraction, ui_event_SliderTraction, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_BtnScramble, ui_event_ScrambleBtn, LV_EVENT_ALL, NULL); 
    lv_obj_add_event_cb(ui_ValetSwitch, ui_event_ValetSwitch, LV_EVENT_VALUE_CHANGED, NULL);
    
    set_boost_mode(savedBoost); 
}

// ======================================================
// 5. SERIAL SENDER (NATIVE UART)
// ======================================================
void send_Serial_Control_Packet() {
    static uint32_t lastHeartbeat = 0;
    uint32_t now = esp_timer_get_time() / 1000;

    // We use UART_NUM_1 because we defined that for our ECU in main.cpp
    if (dataChanged) {
        uart_write_bytes(UART_NUM_1, (const char*)&outgoingControl, sizeof(ControlPacket));
        dataChanged = false; 
        lastHeartbeat = now; 
        return;
    }

    if (now - lastHeartbeat > 1000) {
        uart_write_bytes(UART_NUM_1, (const char*)&outgoingControl, sizeof(ControlPacket));
        lastHeartbeat = now;
    }
}
