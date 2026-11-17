#ifndef BOARD_HAL_H
#define BOARD_HAL_H

#include "driverlib.h"

/* Connector candidates from connection plan v0.1, not electrical validation. */
#define BOARD_UH_GPIO 0U
#define BOARD_UL_GPIO 1U
#define BOARD_VH_GPIO 2U
#define BOARD_VL_GPIO 3U
#define BOARD_WH_GPIO 10U
#define BOARD_WL_GPIO 11U
#define BOARD_SD_A_GPIO 8U

#define BOARD_U_EPWM_BASE EPWM1_BASE
#define BOARD_V_EPWM_BASE EPWM2_BASE
#define BOARD_W_EPWM_BASE EPWM6_BASE

#define BOARD_IU_ADC_BASE ADCC_BASE
#define BOARD_IU_ADC_CH ADC_CH_ADCIN9
#define BOARD_IV_ADC_BASE ADCB_BASE
#define BOARD_IV_ADC_CH ADC_CH_ADCIN2
#define BOARD_IW_ADC_BASE ADCA_BASE
#define BOARD_IW_ADC_CH ADC_CH_ADCIN4
#define BOARD_VBUS_ADC_BASE ADCB_BASE
#define BOARD_VBUS_ADC_CH ADC_CH_ADCIN9

#define BOARD_IU_CMPSS_BASE CMPSS1_BASE
#define BOARD_IV_CMPSS_BASE CMPSS3_BASE
#define BOARD_IW_CMPSS_BASE CMPSS2_BASE
#define BOARD_ENCODER_EQEP_BASE EQEP1_BASE

/* Internal timing candidates only; no ePWM pin is connected in this build. */
#define BOARD_PWM_PERIOD_TICKS 3000U
#define BOARD_PWM_DEADTIME_TICKS 60U
#define BOARD_ADC_ACQPS 19U

/* Shadow-control conversion placeholders.  These are software-test units,
   not calibrated Wildfire current-sensor or bus-voltage coefficients. */
#define BOARD_ADC_MID_CODE 2048.0f
#define BOARD_SHADOW_CURRENT_FULL_SCALE_A 1.0f
#define BOARD_SHADOW_VBUS_FULL_SCALE_V 24.0f

void Board_initStage1Locked(void);

#endif
