#ifndef CUSTOM_DASHBOARD_H
#define CUSTOM_DASHBOARD_H

#include "lvgl.h"
#include "ui.h" 
#include "globalVariables.h" 
#include "SpeedUnitControl.h" 

#ifdef __cplusplus
extern "C" {
#endif

// --- Screen & Navigation ---
extern lv_obj_t *ui_Screen7;
extern lv_obj_t *ui_BtnBack;

// --- Turn Signals ---
extern lv_obj_t *ui_ImgLeftTurn;
extern lv_obj_t *ui_ImgRightTurn;

// --- Dynamic Updates ---
extern lv_obj_t *ui_ScaleSpeed; 
extern lv_obj_t *ui_LabelSpeedUnit;     

// --- Assets (Images & Fonts) ---
LV_IMG_DECLARE(ui_img_left_png);
LV_IMG_DECLARE(ui_img_right_png);
LV_FONT_DECLARE(ui_font_orbitron25);
LV_FONT_DECLARE(ui_font_orbitron40);

// --- Gauges ---
extern lv_obj_t *ui_MeterSpeed;
extern lv_obj_t *ui_NeedleSpeed;
extern lv_obj_t *ui_MeterRPM;
extern lv_obj_t *ui_NeedleRPM;
extern lv_obj_t *ui_MeterTemp;
extern lv_obj_t *ui_NeedleTemp;
extern lv_obj_t *ui_MeterFuel;
extern lv_obj_t *ui_NeedleFuel;

// --- Digital Readouts ---
extern lv_obj_t *ui_LabelGear;
extern lv_obj_t *ui_LabelRPMDigit;
extern lv_obj_t *ui_LabelOdo;

// --- Dashboard Functions ---
void Create_Dashboard_Screen7();
void Update_Dashboard7(const Maxxecu &data); 

#ifdef __cplusplus
}
#endif

#endif // CUSTOM_DASHBOARD_H