#ifndef DRAG_RACE_MODE_H
#define DRAG_RACE_MODE_H

#include "lvgl.h"
#include "globalVariables.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Drag Race Logic Functions ---
void startDragRace();
void stopDragRace();
void updateDragRaceUI();
void resetDragRaceData();

// Optional: For future G-force logging
// void updateGForceData(); 

#ifdef __cplusplus
}
#endif

#endif // DRAG_RACE_MODE_H