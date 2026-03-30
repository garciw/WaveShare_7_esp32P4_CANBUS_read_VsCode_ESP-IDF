#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "lvgl.h"
#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

// **Diagnostics Functions**
// This checks for active fault codes from the MaxxECU
void checkECUErrors();

#ifdef __cplusplus
}
#endif

#endif // DIAGNOSTICS_H