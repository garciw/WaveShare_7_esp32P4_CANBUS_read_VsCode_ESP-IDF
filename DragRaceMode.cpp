#include "DragRaceMode.h"
#include "lvgl.h"
#include "ui.h"
#include "globalVariables.h"
#include "SpeedUnitControl.h"
#include <stdio.h>    // Replaces cstdio
#include <math.h>     // Replaces cmath
#include "esp_timer.h" // 🛑 ESP-IDF Timer

// ==========================================
// GLOBALS & STATE
// ==========================================
static bool raceActive = false;
static uint32_t startTime = 0;           // 🛑 ESP-IDF change
static uint32_t lastIntegrationTime = 0; // 🛑 ESP-IDF change
static float distanceMeters = 0;         // Always track in METERS internally
static float previousSpeedMPS = 0;       // Store previous speed for trapezoidal math
static bool waitingForLaunch = false;

// Stores results
struct DragRaceSession {
    float sixtyFtTime;
    float zeroToHundredTime;
    float quarterMileTime;
    int16_t maxSpeed;
};

DragRaceSession raceSessions[2]; 
int currentSession = 0;

// ==========================================
// START LOGIC
// ==========================================
void startDragRace() {
    
    // 1. ARMING: If stopped, get ready
    if (receivedData.vehicleSpeed == 0) {
        waitingForLaunch = true; 
        // Optional: You could set a "READY" label here if you have one
    }

    // 2. LAUNCH DETECTION
    if (waitingForLaunch && receivedData.vehicleSpeed > 0) {
        
        // Anti-False Start: If RPM is high but speed is barely moving (Wheelspin/Burnout?)
        if (receivedData.rpm > 5000 && receivedData.vehicleSpeed < 2) {  
             return; 
        }

        // 3. GO!
        startTime = esp_timer_get_time() / 1000; // 🛑 ESP-IDF change
        lastIntegrationTime = startTime;
        distanceMeters = 0;
        previousSpeedMPS = 0; // Reset physics
        raceActive = true;
        waitingForLaunch = false; 

        // Clear current session data
        raceSessions[currentSession] = {0, 0, 0, 0};
        
        // Clear UI labels immediately
        if(ui_Label60ftRun1) lv_label_set_text(ui_Label60ftRun1, "--.--s");
        if(ui_LabelZeroToOnehundredRun1) lv_label_set_text(ui_LabelZeroToOnehundredRun1, "--.--s");
        if(ui_LabelQuarterMileRun1) lv_label_set_text(ui_LabelQuarterMileRun1, "--.--s");
    }
}

// ==========================================
// STOP LOGIC
// ==========================================
void stopDragRace() {
    raceActive = false;
    currentSession = (currentSession + 1) % 2; // Cycle sessions
}

// ==========================================
// PHYSICS & UI LOOP (High Accuracy Version)
// ==========================================
void updateDragRaceUI() {
    
    // 1. If not racing, do nothing (Saves CPU)
    if (!raceActive) return;

    uint32_t currentMillis = esp_timer_get_time() / 1000; // 🛑 ESP-IDF change
    uint32_t dt = currentMillis - lastIntegrationTime;    // Delta time
    
    // Safety check: Avoid divide by zero or huge jumps
    if (dt == 0 || dt > 500) {
        lastIntegrationTime = currentMillis;
        return;
    }

    // --------------------------------------------------
    // A. PHYSICS ENGINE (Trapezoidal Integration)
    // --------------------------------------------------
    // We assume receivedData.vehicleSpeed is ALWAYS KPH from ECU.
    
    float currentSpeedKPH = (float)receivedData.vehicleSpeed;
    float currentSpeedMPS = currentSpeedKPH / 3.6; // Convert KPH to Meters per Second
    
    // TRAPEZOIDAL MATH:
    // Instead of assuming constant speed, we take the average of Start and End speed.
    // This handles acceleration curves much better.
    float avgSpeedMPS = (previousSpeedMPS + currentSpeedMPS) / 2.0;
    
    // Calculate distance traveled in this specific time step
    float distanceStep = avgSpeedMPS * (dt / 1000.0); 
    
    // Update totals
    float prevDistanceMeters = distanceMeters; // Save for interpolation
    distanceMeters += distanceStep; 
    
    uint32_t elapsedTimeTotal = currentMillis - startTime;

    // --------------------------------------------------
    // B. RECORD SPLITS (With Linear Interpolation)
    // --------------------------------------------------
    
    // 1. 60 Foot (18.288 Meters)
    if (raceSessions[currentSession].sixtyFtTime == 0 && distanceMeters >= 18.288) {
        
        // INTERPOLATION: Find the exact fraction of the step where we crossed 18.288m
        float fraction = (18.288 - prevDistanceMeters) / (distanceMeters - prevDistanceMeters);
        
        // Calculate exact time: (Total Time - dt) + (dt * fraction)
        float exactTimeMs = (elapsedTimeTotal - dt) + (fraction * dt);
        
        raceSessions[currentSession].sixtyFtTime = exactTimeMs / 1000.0;
        
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2fs", raceSessions[currentSession].sixtyFtTime);
        if(ui_Label60ftRun1) lv_label_set_text(ui_Label60ftRun1, buf);
    }

    // 2. 0-100 KPH (0-62 MPH)
    // We use KPH internally for the check (100.0 KPH standard)
    if (raceSessions[currentSession].zeroToHundredTime == 0 && currentSpeedKPH >= 100.0) {
        
        // INTERPOLATION: Find exact moment we crossed 100kph speed
        float prevSpeedKPH = previousSpeedMPS * 3.6;
        
        // Avoid divide by zero if speed didn't change
        float fraction = 0.0;
        if (currentSpeedKPH > prevSpeedKPH) {
            fraction = (100.0 - prevSpeedKPH) / (currentSpeedKPH - prevSpeedKPH);
        }
        
        float exactTimeMs = (elapsedTimeTotal - dt) + (fraction * dt);

        raceSessions[currentSession].zeroToHundredTime = exactTimeMs / 1000.0;
        
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2fs", raceSessions[currentSession].zeroToHundredTime);
        if(ui_LabelZeroToOnehundredRun1) lv_label_set_text(ui_LabelZeroToOnehundredRun1, buf);
    }

    // 3. Quarter Mile (402.34 Meters)
    if (raceSessions[currentSession].quarterMileTime == 0 && distanceMeters >= 402.34) {
        
        // INTERPOLATION: Find exact crossing of 402.34m
        float fraction = (402.34 - prevDistanceMeters) / (distanceMeters - prevDistanceMeters);
        float exactTimeMs = (elapsedTimeTotal - dt) + (fraction * dt);
        
        raceSessions[currentSession].quarterMileTime = exactTimeMs / 1000.0;
        
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2fs", raceSessions[currentSession].quarterMileTime);
        if(ui_LabelQuarterMileRun1) lv_label_set_text(ui_LabelQuarterMileRun1, buf);
        
        // RACE FINISHED
        stopDragRace();
    }

    // --------------------------------------------------
    // C. UPDATE DISPLAY (Speed & Max Speed)
    // --------------------------------------------------
    
    // 1. Determine Display Speed (MPH or KPH)
    int displaySpeed = (int)currentSpeedKPH;
    if (useMPH) {
        displaySpeed = (int)(currentSpeedKPH / 1.60934);
    }

    // 2. Update Max Speed (Only if higher)
    if (displaySpeed > raceSessions[currentSession].maxSpeed) {
        raceSessions[currentSession].maxSpeed = displaySpeed;
    }

    // 3. Update Labels
    static int lastDisplaySpeed = -1;
    if (displaySpeed != lastDisplaySpeed) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", displaySpeed);
        if(ui_LabelMaxSpeedRun1) lv_label_set_text(ui_LabelMaxSpeedRun1, buf);
        lastDisplaySpeed = displaySpeed;
    }
    
    // 4. Update Units
    static bool lastUnitState = !useMPH;
    if (useMPH != lastUnitState) {
        if(ui_speedunit3) lv_label_set_text(ui_speedunit3, useMPH ? "MPH" : "km/h");
        if(ui_speedunit8) lv_label_set_text(ui_speedunit8, useMPH ? "MPH" : "km/h");
        lastUnitState = useMPH;
    }

    // --------------------------------------------------
    // D. PREPARE FOR NEXT LOOP
    // --------------------------------------------------
    previousSpeedMPS = currentSpeedMPS;
    lastIntegrationTime = currentMillis;
}

// ==========================================
// RESET LOGIC
// ==========================================
void resetDragRaceData() {
    currentSession = 0;
    raceActive = false;
    distanceMeters = 0;
    waitingForLaunch = true; // Ready to go again immediately

    for (int i = 0; i < 2; i++) {
        raceSessions[i] = {0, 0, 0, 0};
    }

    // Reset UI
    if(ui_Label60ftRun1) lv_label_set_text(ui_Label60ftRun1, "0.00s");
    if(ui_LabelZeroToOnehundredRun1) lv_label_set_text(ui_LabelZeroToOnehundredRun1, "0.00s");
    if(ui_LabelQuarterMileRun1) lv_label_set_text(ui_LabelQuarterMileRun1, "0.00s");
    if(ui_LabelMaxSpeedRun1) lv_label_set_text(ui_LabelMaxSpeedRun1, "0");
}