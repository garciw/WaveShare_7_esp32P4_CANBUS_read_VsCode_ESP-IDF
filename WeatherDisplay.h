#ifndef WEATHER_DISPLAY_H
#define WEATHER_DISPLAY_H

#include "lvgl.h"
#include "ui.h" // Access to ui_weatherLabel and ui_weatherImage

#ifdef __cplusplus
extern "C" {
#endif

// --- Weather Logic Functions ---
/**
 * @brief Updates the temperature and weather icon on Screen 1.
 * Called by a timer in main.cpp when new data arrives from the bridge.
 */
void updateWeatherDisplay();

#ifdef __cplusplus
}
#endif

#endif // WEATHER_DISPLAY_H