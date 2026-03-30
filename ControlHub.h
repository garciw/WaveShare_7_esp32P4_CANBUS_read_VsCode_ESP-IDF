#ifndef CONTROLHUB_H
#define CONTROLHUB_H

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"
#include "ui.h" // Access to your UI objects

#ifdef __cplusplus
extern "C" {
#endif

// --- DATA STRUCTURE ---
typedef struct {
    int boostMode;       // 0=Street, 1=Sport, 2=Race
    int tractionVal;     // 0-100%
    bool scrambleActive; // true = BUTTON HELD
    bool valetActive;    // true = ON
} ControlPacket;

// Global instance (accessible by main.cpp)
extern ControlPacket outgoingControl;

// --- FUNCTION PROTOTYPES ---

// Call this in setup() to link buttons to functions
void setupControlHub();

// Matches your Serial logic in main.cpp
void send_Serial_Control_Packet();

// UI Helper functions
void set_boost_mode(int mode);
void update_traction_ui(int value);

#ifdef __cplusplus
}
#endif

#endif // CONTROLHUB_H