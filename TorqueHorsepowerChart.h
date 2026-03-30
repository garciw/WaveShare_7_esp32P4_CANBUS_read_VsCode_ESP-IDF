#ifndef TORQUE_HORSEPOWER_CHART_H
#define TORQUE_HORSEPOWER_CHART_H

#include "lvgl.h"
#include "ui.h" // Access to ui_Chart3

#ifdef __cplusplus
extern "C" {
#endif

// --- Live Dyno Chart Functions ---
// These handle the series creation and data pushing for the 5E-FTE powerband
void initialize_chart_series();
void update_chart_from_ecu();
void start_chart_timer(void);
void my_hide_chart_timer(lv_event_t * e);    //extern

#ifdef __cplusplus
}
#endif

#endif // TORQUE_HORSEPOWER_CHART_H