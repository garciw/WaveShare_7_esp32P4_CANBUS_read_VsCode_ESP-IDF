#ifndef RADIO_MANAGER_H
#define RADIO_MANAGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- Radio Logic Functions ---
// These are called by the LVGL events in your main.cpp
void setupRadio();
void nextRadioStation();
void prevRadioStation();
void togglePlayPause();
void setRadioVolume(uint8_t vol);

#ifdef __cplusplus
}
#endif

#endif // RADIO_MANAGER_H