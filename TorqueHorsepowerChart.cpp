#include "TorqueHorsepowerChart.h"
#include "globalVariables.h"
#include "lvgl.h"
#include "ui.h"

// ------------ Chart Variables ----------------
static lv_chart_series_t* series_torque = nullptr;
static lv_chart_series_t* series_hp = nullptr;
static lv_timer_t* chart_timer = nullptr;

// ------------ Chart Initialization ----------------
void initialize_chart_series() {
    if (ui_Chart3 == nullptr) return;

    // LVGL 9: Ensure we have pointers to the series
    if (series_torque == nullptr) {
        series_torque = lv_chart_add_series(ui_Chart3, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    }
    
    if (series_hp == nullptr) {
        series_hp = lv_chart_add_series(ui_Chart3, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    }
}

// ------------ Chart Update ----------------
void update_chart_from_ecu() {
    // 1. VISIBILITY CHECK (The CPU Saver)
    if (lv_screen_active() != ui_Screen2) {
        return; 
    }

    // 2. POINTER SAFETY
    if (ui_Chart3 == nullptr) return;
    
    // Auto-init if missing
    if (series_torque == nullptr || series_hp == nullptr) {
        initialize_chart_series();
    }

    // 3. GET DATA
    // We cast to int32_t to ensure compatibility with LVGL 9's chart value types
    int32_t torque_nm = (int32_t)receivedData.Torque_Nm;
    int32_t hp        = (int32_t)receivedData.HorsePowerSC10; 

    // 4. PUSH TO CHART
    // LVGL 9: lv_chart_set_next_value is still the standard for rolling charts
    lv_chart_set_next_value(ui_Chart3, series_torque, torque_nm);
    lv_chart_set_next_value(ui_Chart3, series_hp, hp);
}

// ------------ Chart Timer Callback ----------------
static void chart_timer_cb(lv_timer_t* timer) {
    update_chart_from_ecu(); 
}

// ------------ Start Chart Timer ----------------
void start_chart_timer(void) {
    if (!chart_timer) {
        // 100ms = 10 updates per second for a smoother line
        chart_timer = lv_timer_create(chart_timer_cb, 100, NULL); 
    }
}

// ------------ Stop Chart Timer ----------------
void my_hide_chart_timer(lv_event_t * e) {
    if (chart_timer) {
        lv_timer_delete(chart_timer);
        chart_timer = nullptr;
    }
}