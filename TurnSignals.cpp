#include "TurnSignals.h"
#include "globalVariables.h"
#include "ui.h"
#include "lvgl.h"
#include "CustomDashboard.h"

lv_timer_t *turnSignalTimer = nullptr;
const uint32_t blinkInterval = 500; 
static bool blinkState = false;

// The actual timer logic
void turn_signal_timer_cb(lv_timer_t *timer) {
    bool leftActive = (receivedData.leftTurnSignal == 1);   
    bool rightActive = (receivedData.rightTurnSignal == 1); 

    if (!leftActive && !rightActive) {
        if (ui_LTURN) lv_obj_add_flag(ui_LTURN, LV_OBJ_FLAG_HIDDEN);
        if (ui_RTURN) lv_obj_add_flag(ui_RTURN, LV_OBJ_FLAG_HIDDEN);
        if (ui_ImgLeftTurn) lv_obj_add_flag(ui_ImgLeftTurn, LV_OBJ_FLAG_HIDDEN);
        if (ui_ImgRightTurn) lv_obj_add_flag(ui_ImgRightTurn, LV_OBJ_FLAG_HIDDEN);
        blinkState = false;
        return;
    }

    blinkState = !blinkState;

    // Main Screen Update
    if (ui_LTURN) (leftActive && blinkState) ? lv_obj_remove_flag(ui_LTURN, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(ui_LTURN, LV_OBJ_FLAG_HIDDEN);
    if (ui_RTURN) (rightActive && blinkState) ? lv_obj_remove_flag(ui_RTURN, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(ui_RTURN, LV_OBJ_FLAG_HIDDEN);

    // Custom Dash (Screen 7) Update
    if (ui_ImgLeftTurn) (leftActive && blinkState) ? lv_obj_remove_flag(ui_ImgLeftTurn, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(ui_ImgLeftTurn, LV_OBJ_FLAG_HIDDEN);
    if (ui_ImgRightTurn) (rightActive && blinkState) ? lv_obj_remove_flag(ui_ImgRightTurn, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(ui_ImgRightTurn, LV_OBJ_FLAG_HIDDEN);
}

// RESTORING THE DELETED FUNCTIONS
extern "C" void setup_turn_signal_timer() {
    if (turnSignalTimer == nullptr) {
        turnSignalTimer = lv_timer_create(turn_signal_timer_cb, blinkInterval, NULL);
    }
}

extern "C" void notify_turn_signal_update() {
    // Copilot deleted this, but main.cpp needs it!
    // Even if it's empty, it keeps the Linker happy.
}