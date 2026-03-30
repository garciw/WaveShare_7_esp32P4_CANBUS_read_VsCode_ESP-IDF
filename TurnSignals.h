#ifndef TURNSIGNALS_H
#define TURNSIGNALS_H

#include "globalVariables.h"
#include "lvgl.h"
#include "ui.h" 

#ifdef __cplusplus
extern "C" {
#endif

// --- Turn Signal Logic ---

/**
 * @brief Initializes the LVGL timer that handles the flashing logic (500ms blink)
 */
void setup_turn_signal_timer();  

/**
 * @brief Optional: Manually trigger a state check when new data is received
 */
void notify_turn_signal_update(); 

#ifdef __cplusplus
}
#endif

#endif // TURNSIGNALS_H