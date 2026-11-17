#ifndef WILDFIRE_LOCK_H
#define WILDFIRE_LOCK_H
#include <stdint.h>
#include <stdbool.h>
#if !defined(WILDFIRE_LOCKED)
#error Stage6 requires permanent hardware output locking
#endif
#define WILDFIRE_BUILD_ID 0x26111702UL
/* Diagnostic identity only. No variable is an unlock command. */
typedef struct {
    uint32_t buildId;
    uint32_t blockedPWM, blockedClear, blockedConfig, blockedISR;
    uint32_t adcOverflowMask;
    uint16_t initialized, controlQualified, calibrationVerified, motorVerified;
    uint16_t phaseVoltageAvailable, sourceBoardSelectorIsLegacy;
    uint16_t adcFlags[3], adcOverflows[3];
    float shadowModulation[3];
} WILDFIRE_Status;
extern volatile WILDFIRE_Status gWildfire;
extern volatile uint16_t gTripEventMask, gProtectionLatch;
void Wildfire_initBoardLocked(void);
void Board_pollProtection(void);
void Wildfire_forceOutputsLocked(void);
bool Wildfire_controlQualified(void);
#endif
