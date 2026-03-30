#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// 🏎️ NEW: ESP32 CAN-BUS DRIVER
#include "driver/twai.h" 

// LVGL + BSP
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"

extern "C" {
#include "ui/ui.h"
#include "ui/ui_events.h"
}

// --- YOUR MODULES ---
#include "globalVariables.h"
#include "Screen1.h"
#include "Screen2.h"
#include "Launch.h"
#include "CustomDashboard.h"
#include "DynamicScreen.h"
#include "Diagnostics.h"
#include "BrightnessControl.h"
#include "ControlHub.h"
#include "TurnSignals.h"
#include "warningBanner.h"
#include "RadioManager.h"
#include "WeatherDisplay.h"
#include "DragRaceMode.h"
#include "SpeedLimit.h"

#define millis() (uint32_t)(esp_timer_get_time() / 1000)
#define delay(ms) vTaskDelay(pdMS_TO_TICKS(ms))

// ==========================================
// 🔌 CAN BUS PINS (Check your schematic!)
// ==========================================
// Trace the CTX and CRX pins from the TJA1051 on your diagram 
// to find these GPIO numbers:
#define CAN_TX_PIN GPIO_NUM_22 // Replace with your board's actual TX pin
#define CAN_RX_PIN GPIO_NUM_21 // Replace with your board's actual RX pin

static const char *TAG = "CAN_DASH";
SemaphoreHandle_t dataMutex;

// EXTERNS
extern void notify_turn_signal_update();
extern void Update_Launch_Control(const Maxxecu &data);
extern void check_warnings();
extern void updateOdometer();
extern void checkECUErrors();
extern void updateDisplay();
extern void updateWeatherDisplay();
extern void updateScreen1Speed();
extern void updateEcuLoggingText();
extern void updateWiFiStatusIcon(); 
extern void updateLocationData();   
extern void Create_Dashboard_Screen7(); 
extern void startDragRace();      
extern void updateDragRaceUI();  
extern void start_chart_timer(); //{
    // Re-enable actual logic if chart is needed
//}

void Event_NextStation(lv_event_t * e) { nextRadioStation(); }
void Event_PrevStation(lv_event_t * e) { prevRadioStation(); }
void Event_TogglePlay(lv_event_t * e) { togglePlayPause(); }
void Event_VolumeChange(lv_event_t * e) {
    lv_obj_t * arc = (lv_obj_t *)lv_event_get_target(e); 
    setRadioVolume(lv_arc_get_value(arc));
}

void slip_warning_timer(lv_timer_t * timer) {
    if (lv_screen_active() == ui_Screen8) { 
        lv_obj_add_flag(ui_Slip, LV_OBJ_FLAG_HIDDEN);
        return; 
    }
    if (receivedData.wheelSlip > 3.0) {
        if ((millis() / 100) % 2 == 0) lv_obj_clear_flag(ui_Slip, LV_OBJ_FLAG_HIDDEN); 
        else lv_obj_add_flag(ui_Slip, LV_OBJ_FLAG_HIDDEN);   
    } else {
        lv_obj_add_flag(ui_Slip, LV_OBJ_FLAG_HIDDEN);
    }
}

void updateRunningLogic() {
    Update_Launch_Control(receivedData);
    notify_turn_signal_update();
    check_warnings();
    updateOdometer();
    checkECUErrors();

    lv_obj_t* activeScreen = lv_screen_active();
    if (activeScreen == ui_Screen1) {
        updateDisplay();
        updateWeatherDisplay();
        updateScreen1Speed();
        updateEcuLoggingText();
    }
    else if (activeScreen == ui_Screen2) {
        updateDisplay2(); 
        updateScreen2Speed(); 
        updateEcuLoggingText2(); 
    }
	else if (activeScreen == ui_Screen3) {
        startDragRace();      
        updateDragRaceUI();   
    }
    else if (activeScreen == ui_Screen7) {
        Update_Dashboard7(receivedData);
    }
}

// ==========================================
// 🏎️ CAN BUS RECEIVE TASK (Replaces UART)
// ==========================================
void CAN_Task(void *pvParameters) {
    ESP_LOGI(TAG, "CAN Task Started. Listening to the ECU...");
    twai_message_t rx_msg;

    for (;;) {
        // Wait for a CAN message to arrive
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(10)) == ESP_OK) {
            
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                lastPacketTime = millis();
                    
                    //  PARSE EVERY MAXXECU & MCNODE CAN ID
                switch(rx_msg.identifier) {
                    
                    // --- STREET NAME (From BridgeNODE/ECU) ---
                    case 0x112: memcpy(receivedData.displayName + 0,  rx_msg.data, 8); break;
                    case 0x113: memcpy(receivedData.displayName + 8,  rx_msg.data, 8); break;
                    case 0x114: memcpy(receivedData.displayName + 16, rx_msg.data, 8); break;
                    case 0x115: memcpy(receivedData.displayName + 24, rx_msg.data, 8); break;
                    case 0x116: receivedData.displayName[31] = '\0'; break;

                    // --- WEATHER ---
                    case 0x120: 
                        receivedData.weatherTemp = (int8_t)rx_msg.data[0];
                        receivedData.weatherHumidity = rx_msg.data[1];
                        memcpy(receivedData.weatherCond, &rx_msg.data[2], 6);
                        receivedData.weatherCond[6] = '\0';
                        break;

                    // --- MAXXECU ENGINE DATA ---
                    case 0x520:
                        receivedData.rpm = (rx_msg.data[1] << 8) | rx_msg.data[0];
                        receivedData.throttlePos = ((rx_msg.data[3] << 8) | rx_msg.data[2]) * 0.1f;
                        receivedData.MAP = ((rx_msg.data[5] << 8) | rx_msg.data[4]) * 0.1f;
                        receivedData.lambda = ((rx_msg.data[7] << 8) | rx_msg.data[6]) * 0.001f;
                        break;

                    case 0x521:
                        receivedData.lambdaA = ((rx_msg.data[1] << 8) | rx_msg.data[0]) * 0.001f;
                        receivedData.Afr = receivedData.lambdaA * 14.7f;
                        receivedData.lambdaB = ((rx_msg.data[3] << 8) | rx_msg.data[2]) * 0.001f;
                        receivedData.ignitionAngle = ((rx_msg.data[5] << 8) | rx_msg.data[4]) * 0.1f;
                        receivedData.ignitionCut = (rx_msg.data[7] << 8) | rx_msg.data[6];
                        break;

                    case 0x522:
                        receivedData.fuelPulseWidth = ((rx_msg.data[1] << 8) | rx_msg.data[0]) * 0.001f;
                        receivedData.fuelDuty = ((rx_msg.data[3] << 8) | rx_msg.data[2]) * 0.1f;
                        receivedData.fuelCut = (rx_msg.data[5] << 8) | rx_msg.data[4];
                        receivedData.vehicleSpeed = ((rx_msg.data[7] << 8) | rx_msg.data[6]) / 10;
                        break;

                    case 0x523:
                        receivedData.drivenSpeed = ((rx_msg.data[3] << 8) | rx_msg.data[2]) * 0.1f;
                        receivedData.wheelSlip = ((rx_msg.data[5] << 8) | rx_msg.data[4]) * 0.1f;
                        receivedData.targetSlip = ((rx_msg.data[7] << 8) | rx_msg.data[6]) * 0.1f;
                        break;

                    case 0x524:
                        receivedData.tractionCtrl = ((rx_msg.data[1] << 8) | rx_msg.data[0]) * 0.1f;
                        receivedData.lambdaCorrA = ((rx_msg.data[3] << 8) | rx_msg.data[2]) * 0.1f;
                        receivedData.lambdaCorrB = ((rx_msg.data[5] << 8) | rx_msg.data[4]) * 0.1f;
                        receivedData.firmwareVersion = ((rx_msg.data[7] << 8) | rx_msg.data[6]) * 0.001f;
                        break;

                    case 0x526: 
                        receivedData.shiftcutActive = (rx_msg.data[0] & 0x01) ? 1 : 0;
                        receivedData.revLimitActive = (rx_msg.data[0] & 0x02) ? 1 : 0;
                        receivedData.antiLagActive  = (rx_msg.data[0] & 0x04) ? 1 : 0;
                        receivedData.launchControlActive = (rx_msg.data[0] & 0x08) ? 1 : 0;
                        receivedData.tractionPowerLimiterActive = (rx_msg.data[0] & 0x10) ? 1 : 0;
                        receivedData.throttleBlipActive = (rx_msg.data[0] & 0x20) ? 1 : 0;
                        receivedData.acIdleUpActive     = (rx_msg.data[0] & 0x40) ? 1 : 0;
                        receivedData.knockDetected      = (rx_msg.data[0] & 0x80) ? 1 : 0;

                        receivedData.brakePedalActive  = (rx_msg.data[1] & 0x01) ? 1 : 0;
                        receivedData.clutchPedalActive = (rx_msg.data[1] & 0x02) ? 1 : 0;
                        receivedData.speedLimitActive  = (rx_msg.data[1] & 0x04) ? 1 : 0;
                        receivedData.gpLimiterActive   = (rx_msg.data[1] & 0x08) ? 1 : 0;
                        receivedData.userCutActive     = (rx_msg.data[1] & 0x10) ? 1 : 0;
                        receivedData.ecuLogging        = (rx_msg.data[1] & 0x20) ? 1 : 0;
                        receivedData.nitrousActive     = (rx_msg.data[1] & 0x40) ? 1 : 0;
                        
                        receivedData.revLimitRPM = (rx_msg.data[5] << 8) | rx_msg.data[4];
                        break;

                    case 0x530:
                        receivedData.batteryVoltage = ((rx_msg.data[1] << 8) | rx_msg.data[0]) * 0.01f;
                        receivedData.baroPressure = ((rx_msg.data[3] << 8) | rx_msg.data[2]) * 0.1f;
                        receivedData.intakeAirTemp = ((rx_msg.data[5] << 8) | rx_msg.data[4]) * 0.1f;
                        receivedData.coolantTemp = ((rx_msg.data[7] << 8) | rx_msg.data[6]) * 0.1f;
                        break;

                    case 0x534:
                        receivedData.egtDifference = (rx_msg.data[1] << 8) | rx_msg.data[0];
                        receivedData.cpuTemp = (rx_msg.data[3] << 8) | rx_msg.data[2];
                        receivedData.errorCodeCount = (rx_msg.data[5] << 8) | rx_msg.data[4];
                        receivedData.syncLostCount = (rx_msg.data[7] << 8) | rx_msg.data[6];
                        break;

                    case 0x536:
                        receivedData.gear = (rx_msg.data[1] << 8) | rx_msg.data[0];
                        receivedData.boostSolenoidDuty = ((rx_msg.data[3] << 8) | rx_msg.data[2]) * 0.1f;
                        receivedData.oilTemp = ((rx_msg.data[7] << 8) | rx_msg.data[6]) * 0.1f;
                        break;

                    case 0x537:
                        receivedData.wastegatePressure = ((rx_msg.data[3] << 8) | rx_msg.data[2]) * 0.1f;
                        receivedData.coolantPressure = ((rx_msg.data[5] << 8) | rx_msg.data[4]) * 0.1f;
                        receivedData.boostTarget = ((rx_msg.data[7] << 8) | rx_msg.data[6]) * 0.1f;
                        break;

                    case 0x538:
                        receivedData.userChannel1 = ((rx_msg.data[1] << 8) | rx_msg.data[0]) * 0.1f; 
                        receivedData.userChannel2 = ((rx_msg.data[3] << 8) | rx_msg.data[2]) * 0.1f; 
                        receivedData.userChannel3 = ((rx_msg.data[5] << 8) | rx_msg.data[4]) * 0.1f; 
                        receivedData.userChannel4 = ((rx_msg.data[7] << 8) | rx_msg.data[6]) * 0.1f; 
                        break;

                    case 0x542:
                        receivedData.ecuErrorCodes = ((rx_msg.data[5] << 8) | rx_msg.data[4]);
                        break;

                    case 0x543:
                        receivedData.tachoRPM = (rx_msg.data[1] << 8) | rx_msg.data[0];
                        receivedData.warningCH = ((rx_msg.data[3] << 8) | rx_msg.data[2]); 
                        receivedData.tripmeter = ((rx_msg.data[5] << 8) | rx_msg.data[4]) * 0.1f;
                        receivedData.radFan = (rx_msg.data[7] << 8) | rx_msg.data[6];
                        break;

                    // --- MCNODE SENSORS ---
                    case 0x750: // Brakes & Fuel %
                        receivedData.parkingBrakeActive = rx_msg.data[0];
                        receivedData.oilPressureWarning = rx_msg.data[1];
                        receivedData.fuelLowWarning = rx_msg.data[2];
                        receivedData.fuelLevelPercent = ((rx_msg.data[4] << 8) | rx_msg.data[3]) / 10.0f;
                        break;

                    case 0x751: // Oil/Fuel Pressure & Undriven Speed
                        receivedData.oilPressure = (int16_t)((rx_msg.data[1] << 8) | rx_msg.data[0]);
                        receivedData.fuelPressure = (int16_t)((rx_msg.data[3] << 8) | rx_msg.data[2]);
                        receivedData.undrivenSpeed = ((rx_msg.data[5] << 8) | rx_msg.data[4]) / 10.0f;
                        break;

                    case 0x777: // 🟢 TURN SIGNALS🟢 
                        receivedData.leftTurnSignal = rx_msg.data[0];
                        receivedData.rightTurnSignal = rx_msg.data[1];
                        break;
                }
                xSemaphoreGive(dataMutex);
            }
        }
    }
}

// ==========================================
// 🕹️ TRANSMIT CONTROL DATA TO MAXXECU
// ==========================================
// You can call this function from your UI buttons to send Boost/Traction changes!
void send_CAN_Control_Packet(int boostMode, int tractionVal, bool scramble, bool valet) {
    twai_message_t tx_msg;
    tx_msg.identifier = 0x666;
    tx_msg.extd = 0;
    tx_msg.data_length_code = 3;
    tx_msg.flags = 0;
    
    tx_msg.data[0] = boostMode;
    tx_msg.data[1] = tractionVal;
    tx_msg.data[2] = (scramble ? 0x01 : 0) | (valet ? 0x02 : 0);
    
    twai_transmit(&tx_msg, pdMS_TO_TICKS(10));
}

// ==========================================
// 🎯 LVGL UI TASK
// ==========================================
void LVGLTask(void *pvParameters) {
    uint32_t lastLogic = 0;
    while (true) {
        if (bsp_display_lock(pdMS_TO_TICKS(10))) {
            uint32_t now = millis();
            if (now - lastLogic >= 30) {
                if (xSemaphoreTake(dataMutex, 0) == pdTRUE) {
                    updateRunningLogic(); 
                    xSemaphoreGive(dataMutex);
                }
                lastLogic = now;
            }
            bsp_display_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(16)); 
    }
}

// ==========================================
// 🚀 MAIN ENTRY
// ==========================================
extern "C" void app_main(void) {
    dataMutex = xSemaphoreCreateMutex();

    // -------------------------------
    // 🏎️ INITIALIZE TWAI (CAN BUS)
    // -------------------------------
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    g_config.rx_queue_len = 50;
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); // MaxxECU defaults to 500kbps
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGI(TAG, "CAN Driver installed");
        if (twai_start() == ESP_OK) {
            ESP_LOGI(TAG, "CAN Driver started successfully");
        }
    } else {
        ESP_LOGE(TAG, "Failed to install CAN driver! Check pin definitions.");
    }
    
    // -------------------------------
    // 📺 DISPLAY & UI INIT
    // -------------------------------
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = true,
        }
    };

    bsp_display_start_with_config(&cfg);
    bsp_display_brightness_set(50);

    ui_init();
    setupRadio();
    
    lv_obj_add_event_cb(ui_BtnNext, Event_NextStation, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_BtnPrev, Event_PrevStation, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_BtnPlay, Event_TogglePlay, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_ArcVolume, Event_VolumeChange, LV_EVENT_VALUE_CHANGED, NULL);
    
    lv_obj_set_parent(ui_Slip, lv_layer_top()); 
    lv_obj_add_flag(ui_Slip, LV_OBJ_FLAG_HIDDEN);
    lv_timer_create(slip_warning_timer, 50, NULL);

    setup_turn_signal_timer(); 
    setupUIEvents(); 
    setupSpeedUnitEvents(); 
    createBrightnessOverlay(ui_Screen5);      
    init_warning_banner(); 

    Create_Dashboard_Screen7();   
    setupDynamicScreen();
    setupControlHub();
    hideAllContainers(); 

    lv_timer_create([](lv_timer_t*) { updateWiFiStatusIcon(); }, 1000, NULL);
    lv_timer_create([](lv_timer_t*) { updateLocationData(); }, 10000, NULL); 
    
    // -------------------------------
    // 🚦 START TASKS
    // -------------------------------
    xTaskCreatePinnedToCore(CAN_Task, "CAN_Task", 8192, NULL, 4, NULL, 1); 
    xTaskCreatePinnedToCore(LVGLTask, "LVGL_Task", 16384, NULL, 5, NULL, 0);    
}