#ifndef GLOBALVARIABLES_H
#define GLOBALVARIABLES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Threshold values for engine warnings
#define MAX_RPM 9000
#define MIN_LAMBDA 0.85
#define MAX_LAMBDA 1.15
#define MAX_FUEL_DUTY 90
#define MAX_MAP 300
#define MAX_SPEED 220
#define MIN_VIRTUAL_FUEL 15
#define MIN_OIL_PRESSURE 8
#define MIN_FUEL_PRESSURE 35
#define MAX_COOLANT_TEMP 110
#define MAX_INTAKE_TEMP 80
#define MIN_BATTERY_VOLTAGE 12.4

#define STATIC_ODOMETER_BASE 176338.0  

// --- Connection States ---
extern uint32_t lastPacketTime;
extern bool connectionLost;
extern bool dataReceivedFlag;

// ✅ OPTIMIZED MAXXECU STRUCT (Packed by Size)
typedef struct {
    // 4-BYTE VARIABLES
    float throttlePos, MAP, Afr, lambda, lambdaA, lambdaB;
    float ignitionAngle, fuelPulseWidth, fuelDuty;
    float undrivenSpeed, drivenSpeed, wheelSlip, targetSlip;
    float tractionCtrl, lambdaCorrA, lambdaCorrB, firmwareVersion;
    float batteryVoltage, baroPressure, intakeAirTemp, coolantTemp;
    float totalFuelTrim, ethanolConcentration, totalIgnitionComp;
    float userAnalogInput1, userAnalogInput2, userAnalogInput3, userAnalogInput4;
    float userChannel1, userChannel2, userChannel3, userChannel4, userChannel5, userChannel6;
    float userChannel7, userChannel8, userChannel9, userChannel10, userChannel11, userChannel12;
    float boostSolenoidDuty, oilPressure, oilTemp, wastegatePressure, coolantPressure;
    float boostTarget, virtualFuelTank, transmissionTemp, differentialTemp;
    float accelerationForward, accelerationRight, accelerationUp, lambdaTarget;
    float knockLevel, warningCH, tripmeter, fuelLevelPercent;
    float gpsLat, gpsLon;
    int rpm; 

    // 2-BYTE VARIABLES
    int16_t id, ignitionCut, fuelCut, vehicleSpeed, egt1, egtDifference;
    int16_t cpuTemp, errorCodeCount, syncLostCount, gear, fuelPressure;
    int16_t revLimitRPM, SPARE3, ecuErrorCodes, adjustedVE1, Torque_Nm;
    int16_t HorsePowerSC10, tachoRPM, radFan;

    // 1-BYTE VARIABLES
    uint8_t isHeartbeat, activeBoostTable, activeTuneSelector, shiftcutActive;
    uint8_t revLimitActive, antiLagActive, launchControlActive;
    uint8_t tractionPowerLimiterActive, throttleBlipActive, acIdleUpActive;
    uint8_t knockDetected, brakePedalActive, clutchPedalActive, speedLimitActive;
    uint8_t gpLimiterActive, userCutActive, ecuLogging, nitrousActive;
    uint8_t leftTurnSignal, rightTurnSignal;
    int8_t userPWMTable1, weatherTemp;
    uint8_t gpsSpeedLimit, weatherHumidity, parkingBrakeActive, oilPressureWarning, fuelLowWarning;
    bool wifiConnected, gpsLocked, tomtomOnline;

    // ARRAYS
    char weatherCond[8]; 
    char displayName[32];
} Maxxecu;

// Global Data Instance
extern Maxxecu receivedData;
static_assert(sizeof(Maxxecu) == 348, "Size Mismatch!");

#ifdef __cplusplus
}
#endif

#endif // GLOBALVARIABLES_H