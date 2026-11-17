/* Stage6 independent power lock. No legacy motor ISR/estimator entry. */
#include "device.h"
#include "board_hal.h"
#include "wildfire_lock.h"
volatile WILDFIRE_Status gWildfire={WILDFIRE_BUILD_ID};
void Wildfire_forceOutputsLocked(void)
{
 EPWM_forceTripZoneEvent(BOARD_U_EPWM_BASE,EPWM_TZ_FORCE_EVENT_OST);
 EPWM_forceTripZoneEvent(BOARD_V_EPWM_BASE,EPWM_TZ_FORCE_EVENT_OST);
 EPWM_forceTripZoneEvent(BOARD_W_EPWM_BASE,EPWM_TZ_FORCE_EVENT_OST);
}
bool Wildfire_controlQualified(void){return false;}
