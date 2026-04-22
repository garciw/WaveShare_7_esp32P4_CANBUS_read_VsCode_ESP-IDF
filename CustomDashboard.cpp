#include "CustomDashboard.h"
#include "globalVariables.h"
#include "SpeedUnitControl.h" 
#include <cstdio>
#include <cmath>
#include "ui.h" 
#include "esp_timer.h"

// Standard ESP-IDF millisecond helper
#define millis() (uint32_t)(esp_timer_get_time() / 1000)

// This forces the gauge to only print single digits
static const char * rpm_labels[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", NULL};

// 1. IMAGE DECLARATIONS
LV_IMG_DECLARE(ui_img_left_png);
LV_IMG_DECLARE(ui_img_right_png);
LV_IMG_DECLARE(ui_img_lowfuel_png);

// GLOBAL POINTERS
lv_obj_t *ui_MeterSpeed = NULL;      
lv_obj_t *ui_MeterRPM = NULL;        
lv_obj_t *ui_NeedleSpeed = NULL;
lv_obj_t *ui_NeedleRPM = NULL;

lv_obj_t *ui_BarTempBG;
lv_obj_t *ui_BarTemp;
lv_obj_t *ui_LabelTempReadout;
lv_obj_t *ui_LabelTempTitle;
lv_obj_t *ui_BarFuelBG;
lv_obj_t *ui_BarFuel;
lv_obj_t *ui_LabelFuelReadout;
lv_obj_t *ui_LabelFuelTitle;
lv_obj_t *ui_ImgLowFuel_Screen7;

lv_obj_t *ui_LabelGear;
lv_obj_t *ui_LabelOdo;
lv_obj_t *ui_ImgLeftTurn;
lv_obj_t *ui_ImgRightTurn;
lv_obj_t *ui_LabelSpeedUnit = NULL;

// =================================================================================
// 🔴 REDLINE STYLES (LVGL 9.5 Official Method)
// =================================================================================
static lv_style_t section_minor_tick_style;
static lv_style_t section_label_style;
static lv_style_t section_main_line_style;
static bool redline_styles_initialized = false;

void Init_Redline_Styles() {
    if (redline_styles_initialized) return;

    lv_style_init(&section_label_style);
    lv_style_init(&section_minor_tick_style);
    lv_style_init(&section_main_line_style);

    /* Label & Major Tick style properties */
    lv_style_set_text_color(&section_label_style, lv_color_hex(0xFF2424));
    lv_style_set_line_color(&section_label_style, lv_color_hex(0xFF2424));
    lv_style_set_line_width(&section_label_style, 4); // Match base major tick width

    /* Minor Tick style properties */
    lv_style_set_line_color(&section_minor_tick_style, lv_color_hex(0xFF2424));
    lv_style_set_line_width(&section_minor_tick_style, 2); // Match base minor tick width

    /* Main arc line properties */
    lv_style_set_arc_color(&section_main_line_style, lv_color_hex(0xFF2424));
    lv_style_set_arc_width(&section_main_line_style, 4); // Match base arc width2

    redline_styles_initialized = true;
}

// =================================================================================
// 🎨 STYLE HELPERS (Subaru White-Face Look)
// =================================================================================
void Style_Scale_Subaru(lv_obj_t *scale) {
    // 🛑 KILL THE GREY BOX
    lv_obj_set_style_bg_opa(scale, 0, 0); 
   // lv_obj_set_style_border_width(scale, 0, 0);
    //lv_obj_set_style_outline_width(scale, 0, 0);
  //  lv_obj_set_style_shadow_width(scale, 0, 0);
    //lv_obj_set_style_radius(scale, LV_RADIUS_CIRCLE, 0);
    
    // Minor Ticks (Black)
    lv_obj_set_style_length(scale, 15, LV_PART_ITEMS);      
    lv_obj_set_style_line_color(scale, lv_color_hex(0x000000), LV_PART_ITEMS);
    lv_obj_set_style_line_opa(scale, 255, LV_PART_ITEMS);
    lv_obj_set_style_line_width(scale, 2, LV_PART_ITEMS);

    // Major Ticks (Black)
    lv_obj_set_style_length(scale, 23, LV_PART_INDICATOR);  
    lv_obj_set_style_line_color(scale, lv_color_hex(0x000000), LV_PART_INDICATOR);
    lv_obj_set_style_line_opa(scale, 255, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(scale, 6, LV_PART_INDICATOR);
    
    // Labels (Black)
    lv_obj_set_style_text_color(scale, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_font(scale, &lv_font_montserrat_24, LV_PART_MAIN);

    // Main Arc (Black)
    lv_obj_set_style_arc_color(scale, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_arc_width(scale, 2, LV_PART_MAIN);
}

void Style_3D_Hub(lv_obj_t *hub) {
    lv_obj_set_style_bg_color(hub, lv_color_hex(0x888888), 0);     //202020 
    lv_obj_set_style_bg_grad_color(hub, lv_color_hex(0x000000), 0); //000000
    lv_obj_set_style_bg_grad_dir(hub, LV_GRAD_DIR_VER, 0);
    //lv_obj_set_style_shadow_color(hub, lv_color_hex(0xFF2424), 0);
  //  lv_obj_set_style_shadow_width(hub, 50, 0); 
    lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(hub, lv_color_hex(0xFF2424), 0);
    lv_obj_set_style_border_width(hub, 3, 0);
}

// =================================================================================
// 📺 POPULATE SCREEN 7 (Starlet Cluster - 450x450)
// =================================================================================
void Create_Dashboard_Screen7() {
    if (ui_Screen7 == NULL) return; 

    // Initialize the redline styles we created above
    Init_Redline_Styles();

    // 1. SIGNALS
    ui_ImgLeftTurn = lv_image_create(ui_Screen7);
    lv_image_set_src(ui_ImgLeftTurn, &ui_img_left_png);
    lv_obj_align(ui_ImgLeftTurn, LV_ALIGN_TOP_LEFT, 310, 20); 
    lv_obj_add_flag(ui_ImgLeftTurn, LV_OBJ_FLAG_HIDDEN);

    ui_ImgRightTurn = lv_image_create(ui_Screen7);
    lv_image_set_src(ui_ImgRightTurn, &ui_img_right_png);
    lv_obj_align(ui_ImgRightTurn, LV_ALIGN_TOP_RIGHT, -310, 20);
    lv_obj_add_flag(ui_ImgRightTurn, LV_OBJ_FLAG_HIDDEN);

    // =================================================================================
    // 2. RPM GAUGE 
    // =================================================================================
    
    // 🛑 THE FIX: Stop the sliding and the square outline!

	
    lv_obj_t * rpm_face = lv_obj_create(ui_Screen7);
    lv_obj_clear_flag(rpm_face, LV_OBJ_FLAG_SCROLLABLE); 
    lv_obj_clear_flag(rpm_face, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(rpm_face, 450, 450);
    lv_obj_align(rpm_face, LV_ALIGN_CENTER, -250, -20);
    lv_obj_set_style_bg_color(rpm_face, lv_color_hex(0xFFFFFF), 0); 
    lv_obj_set_style_bg_opa(rpm_face, 255, 0);
    lv_obj_set_style_radius(rpm_face, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(rpm_face, 6, 0);
    lv_obj_set_style_border_color(rpm_face, lv_color_hex(0x666666), 0);
    //lv_obj_set_style_shadow_width(rpm_face, 0, 0);
    //lv_obj_set_style_outline_width(rpm_face, 0, 0);

    ui_MeterRPM = lv_scale_create(rpm_face);
    lv_obj_clear_flag(ui_MeterRPM, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ui_MeterRPM, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(ui_MeterRPM, 450, 450);
    lv_obj_center(ui_MeterRPM);
    lv_scale_set_mode(ui_MeterRPM, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_total_tick_count(ui_MeterRPM, 91);
    lv_scale_set_major_tick_every(ui_MeterRPM, 10);
    lv_scale_set_label_show(ui_MeterRPM, true);
    
    // Expand the math range to 9000 for high resolution
    lv_scale_set_range(ui_MeterRPM, 0, 9000); 
    
    //Apply the custom text so it still reads 0-9
    lv_scale_set_text_src(ui_MeterRPM, rpm_labels); 
    
    lv_scale_set_angle_range(ui_MeterRPM, 270);
    lv_scale_set_rotation(ui_MeterRPM, 135);
    Style_Scale_Subaru(ui_MeterRPM);

    // 🔴 UPDATE THE REDLINE🛑 : It must match the new 9000 range!
    lv_scale_section_t * section = lv_scale_add_section(ui_MeterRPM);
    lv_scale_section_set_range(section, 7000, 9000); 
    lv_scale_section_set_style(section, LV_PART_INDICATOR, &section_label_style);
    lv_scale_section_set_style(section, LV_PART_ITEMS, &section_minor_tick_style);
    lv_scale_section_set_style(section, LV_PART_MAIN, &section_main_line_style);

    // Needle
    ui_NeedleRPM = lv_line_create(ui_MeterRPM);
    lv_obj_set_style_line_width(ui_NeedleRPM, 7, 0);
    lv_obj_set_style_line_color(ui_NeedleRPM, lv_color_hex(0xFF2424), 0);
    lv_obj_set_style_line_rounded(ui_NeedleRPM, true, 0);

    lv_obj_t * rpm_hub = lv_obj_create(ui_MeterRPM);
    lv_obj_set_size(rpm_hub, 170, 170);
    lv_obj_center(rpm_hub);
    Style_3D_Hub(rpm_hub);

    ui_LabelGear = lv_label_create(rpm_hub);
    lv_obj_center(ui_LabelGear);
    lv_label_set_text(ui_LabelGear, "N");
    lv_obj_set_style_text_font(ui_LabelGear, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(ui_LabelGear, lv_color_hex(0x00AAFF), 0); 

    // =================================================================================
    // 3. SPEED GAUGE
    // =================================================================================
    lv_obj_t * speed_face = lv_obj_create(ui_Screen7);
	lv_obj_clear_flag(speed_face, LV_OBJ_FLAG_SCROLLABLE); 
    lv_obj_clear_flag(speed_face, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(speed_face, 450, 450);
    lv_obj_align(speed_face, LV_ALIGN_CENTER, 250, -20);
    lv_obj_set_style_bg_color(speed_face, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(speed_face, 255, 0);
    lv_obj_set_style_radius(speed_face, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(speed_face, 6, 0);                        //4
    lv_obj_set_style_border_color(speed_face, lv_color_hex(0x666666), 0);  //0x666666  0x888888
   // lv_obj_set_style_shadow_width(speed_face, 0, 0);
   // lv_obj_set_style_outline_width(speed_face, 0, 0);

    ui_MeterSpeed = lv_scale_create(speed_face);
	lv_obj_clear_flag(ui_MeterSpeed, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ui_MeterSpeed, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(ui_MeterSpeed, 450, 450);
    lv_obj_center(ui_MeterSpeed);
    lv_scale_set_mode(ui_MeterSpeed, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_total_tick_count(ui_MeterSpeed, 111);
    lv_scale_set_major_tick_every(ui_MeterSpeed, 10);
    lv_scale_set_label_show(ui_MeterSpeed, true);
    lv_scale_set_angle_range(ui_MeterSpeed, 270);
    lv_scale_set_rotation(ui_MeterSpeed, 135);
    lv_scale_set_range(ui_MeterSpeed, 0, useMPH ? 140 : 220);
    Style_Scale_Subaru(ui_MeterSpeed);

    ui_NeedleSpeed = lv_line_create(ui_MeterSpeed);
    lv_obj_set_style_line_width(ui_NeedleSpeed, 7, 0); 
    lv_obj_set_style_line_color(ui_NeedleSpeed, lv_color_hex(0xFF2424), 0);

    lv_obj_t * speed_hub = lv_obj_create(ui_MeterSpeed);
    lv_obj_set_size(speed_hub, 170, 170);
    lv_obj_center(speed_hub);
    Style_3D_Hub(speed_hub);

    ui_LabelOdo = lv_label_create(speed_hub);
    lv_label_set_text(ui_LabelOdo, "000000");
    lv_obj_center(ui_LabelOdo);
    lv_obj_set_style_text_color(ui_LabelOdo, lv_color_white(), 0);
	
    ui_LabelSpeedUnit = lv_label_create(ui_MeterSpeed);
    lv_label_set_text(ui_LabelSpeedUnit, useMPH ? "MPH" : "km/h");
    lv_obj_align(ui_LabelSpeedUnit, LV_ALIGN_CENTER, 300, 150);
    // =================================================================================
    // 4. COOLANT & FUEL BARS
    // =================================================================================

    ui_BarTempBG = lv_bar_create(ui_Screen7);
    lv_obj_set_size(ui_BarTempBG, 300, 2);    //240,4
    lv_obj_align(ui_BarTempBG, LV_ALIGN_CENTER, -250, 250);    //200,210
    lv_obj_set_style_bg_color(ui_BarTempBG, lv_color_hex(0x999999), LV_PART_MAIN);

    ui_BarTemp = lv_bar_create(ui_Screen7);
    lv_obj_set_size(ui_BarTemp, 300, 2);    //240,4
    lv_obj_align(ui_BarTemp, LV_ALIGN_CENTER, -250, 250);    //200,210
    lv_bar_set_range(ui_BarTemp, 50, 130);
    lv_obj_set_style_bg_color(ui_BarTemp, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);

    ui_LabelTempReadout = lv_label_create(ui_Screen7);
    lv_obj_set_style_text_font(ui_LabelTempReadout, &lv_font_montserrat_22, 0);
    lv_obj_align_to(ui_LabelTempReadout, ui_BarTemp, LV_ALIGN_OUT_TOP_MID, 0, -2);

    ui_LabelTempTitle = lv_label_create(ui_Screen7);
    lv_label_set_text(ui_LabelTempTitle, "COOLANT");
    lv_obj_set_style_text_font(ui_LabelTempTitle, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(ui_LabelTempTitle, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(ui_LabelTempTitle, LV_ALIGN_CENTER, -280, 270);

     //--------------------------------------------------------------------------
    //  FUEL GAUGE 
   //--------------------------------------------------------------
   
    ui_BarFuelBG = lv_bar_create(ui_Screen7);
    lv_obj_set_size(ui_BarFuelBG, 300, 2);    //240,4
    lv_obj_align(ui_BarFuelBG, LV_ALIGN_CENTER, 250, 250);    //200,210
    lv_obj_set_style_bg_color(ui_BarFuelBG, lv_color_hex(0x999999), LV_PART_MAIN);
   
    ui_BarFuel = lv_bar_create(ui_Screen7);
    lv_obj_set_size(ui_BarFuel, 300, 2);
    lv_obj_align(ui_BarFuel, LV_ALIGN_CENTER, 250, 250);
    lv_bar_set_range(ui_BarFuel, 0, 100);
    lv_obj_set_style_bg_color(ui_BarFuel, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR); //0xFF2424

    ui_LabelFuelReadout = lv_label_create(ui_Screen7);
    lv_obj_set_style_text_font(ui_LabelFuelReadout, &lv_font_montserrat_22, 0);
    lv_obj_align_to(ui_LabelFuelReadout, ui_BarFuel, LV_ALIGN_OUT_TOP_MID, 0, -2);  //-5
	
	ui_LabelFuelTitle = lv_label_create(ui_Screen7);
    lv_label_set_text(ui_LabelFuelTitle, "FUEL");
    lv_obj_set_style_text_font(ui_LabelFuelTitle, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(ui_LabelFuelTitle, lv_color_hex(0xAAAAAA), 0);
	lv_obj_align(ui_LabelFuelTitle, LV_ALIGN_CENTER, 260, 270);

    ui_ImgLowFuel_Screen7 = lv_image_create(ui_Screen7);
    lv_image_set_src(ui_ImgLowFuel_Screen7, &ui_img_lowfuel_png);
    lv_obj_align_to(ui_ImgLowFuel_Screen7, ui_BarFuel, LV_ALIGN_OUT_RIGHT_MID, -350, -8);   //10
    lv_obj_add_flag(ui_ImgLowFuel_Screen7, LV_OBJ_FLAG_HIDDEN);

    
}

// =================================================================================
// 🚀 UPDATE LOOP
// =================================================================================
void Update_Dashboard7(const Maxxecu &data) {
    if (lv_screen_active() != ui_Screen7) return;

    uint32_t now = millis();
    static uint32_t lastSlowUpdate = 0;
    
    // Needle lengths updated to 220 for the massive 450px gauges
    static int lastDrawnRPM = -1;
    if (abs(receivedData.rpm - lastDrawnRPM) > 10) {
    lv_scale_set_line_needle_value(ui_MeterRPM, ui_NeedleRPM, 220, data.rpm);
    lastDrawnRPM = receivedData.rpm; 
    }

    float speedVal = (float)data.vehicleSpeed;
    if (useMPH) speedVal /= 1.609f;
    lv_scale_set_line_needle_value(ui_MeterSpeed, ui_NeedleSpeed, 220, (int32_t)speedVal);

    if (now - lastSlowUpdate < 100) return; 
    lastSlowUpdate = now;

    static bool lastUnit = !useMPH;
    if (useMPH != lastUnit) {
        lv_scale_set_range(ui_MeterSpeed, 0, useMPH ? 140 : 220);
        lv_label_set_text(ui_LabelSpeedUnit, useMPH ? "MPH" : "km/h");
        lastUnit = useMPH;
    }
    //COOLANT TEMP UPDATE
        lv_bar_set_value(ui_BarTemp, (int)data.coolantTemp, LV_ANIM_ON);
        lv_label_set_text_fmt(ui_LabelTempReadout, "%.0f °C", data.coolantTemp);
	
    // Dynamic Color Logic for Coolant
    if (data.coolantTemp >= 105) {
        lv_obj_set_style_bg_color(ui_BarTemp, lv_color_hex(0xFF2424), LV_PART_INDICATOR); // Red if Hot
    } else if (data.coolantTemp <= 65) {
        lv_obj_set_style_bg_color(ui_BarTemp, lv_color_hex(0x00AAFF), LV_PART_INDICATOR); // Blue if Cold
    } else {
        lv_obj_set_style_bg_color(ui_BarTemp, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR); // White if Normal
    }
   
    
    // 5. FUEL BAR UPDATE
    lv_bar_set_value(ui_BarFuel, (int)data.fuelLevelPercent, LV_ANIM_ON);
	lv_label_set_text_fmt(ui_LabelFuelReadout, "%.0f %%", data.fuelLevelPercent);
	
            // Dynamic Color Logic for Fuel
    if (data.fuelLevelPercent <= 15) {
      lv_obj_set_style_bg_color(ui_BarFuel, lv_color_hex(0xFF2424), LV_PART_INDICATOR); // Red if Low
    } else {
      lv_obj_set_style_bg_color(ui_BarFuel, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR); // White if Good
    }

    if (data.fuelLowWarning && (now % 800 < 400)) {
        lv_obj_remove_flag(ui_ImgLowFuel_Screen7, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui_ImgLowFuel_Screen7, LV_OBJ_FLAG_HIDDEN);
    }

    if (data.gear == 0) lv_label_set_text(ui_LabelGear, "N");
    else if (data.gear == -1) lv_label_set_text(ui_LabelGear, "R");
    else lv_label_set_text_fmt(ui_LabelGear, "%d", data.gear);

    float totalOdo = STATIC_ODOMETER_BASE + data.userChannel2;
    lv_label_set_text_fmt(ui_LabelOdo, "%.0f km", totalOdo);
}