#ifndef LAUNCH_H
#define LAUNCH_H

#include "ui.h" 
#include "globalVariables.h" 

#ifdef __cplusplus
extern "C" {
#endif

// --- Launch Control Logic ---
// This handles the RPM-based visual feedback and boost status
void Update_Launch_Control(const Maxxecu &data); 

#ifdef __cplusplus
}
#endif

#endif // LAUNCH_H