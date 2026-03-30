#ifndef SPEEDLIMIT_H
#define SPEEDLIMIT_H

#include "lvgl.h"
#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- GPS & Speed Limit Logic ---
/**
 * @brief Updates the dashboard with current GPS speed and local speed limits.
 * Usually called by an LVGL timer every 500-1000ms.
 */
void updateLocationData(); // 🛑 FIX: Removed the timer argument!

#ifdef __cplusplus
}
#endif

#endif // SPEEDLIMIT_H