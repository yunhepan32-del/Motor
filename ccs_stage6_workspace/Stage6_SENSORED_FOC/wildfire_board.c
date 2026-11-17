/* Stage6 locked board adapter: CMPA shadow load PERIOD. Other GPIO, ADC,
   SOCA and protection configuration retained; no hardware test implied. */
#include "device.h"
#include "board_hal.h"
#include "wildfire_lock.h"
volatile uint16_t gTripEventMask=0U,gProtectionLatch=0U;
static void lockTrip(uint32_t base)
{
    EPWM_setTripZoneAction(base, EPWM_TZ_ACTION_EVENT_TZA,
                           EPWM_TZ_ACTION_LOW);
    EPWM_setTripZoneAction(base, EPWM_TZ_ACTION_EVENT_TZB,
                           EPWM_TZ_ACTION_LOW);
    EPWM_forceTripZoneEvent(base, EPWM_TZ_FORCE_EVENT_OST);
}

static void configurePWM(uint32_t base)
{
    EPWM_setClockPrescaler(base, EPWM_CLOCK_DIVIDER_1,
                           EPWM_HSCLOCK_DIVIDER_1);
    EPWM_setTimeBasePeriod(base, BOARD_PWM_PERIOD_TICKS);
    EPWM_setTimeBaseCounter(base, 0U);
    EPWM_setCounterCompareValue(base, EPWM_COUNTER_COMPARE_A,
                                BOARD_PWM_PERIOD_TICKS / 2U);
    EPWM_setCounterCompareShadowLoadMode(base, EPWM_COUNTER_COMPARE_A,
                                          EPWM_COMP_LOAD_ON_CNTR_PERIOD);
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setRisingEdgeDeadBandDelayInput(base, EPWM_DB_INPUT_EPWMA);
    EPWM_setFallingEdgeDeadBandDelayInput(base, EPWM_DB_INPUT_EPWMA);
    EPWM_setDeadBandDelayPolarity(base, EPWM_DB_FED,
                                  EPWM_DB_POLARITY_ACTIVE_LOW);
    EPWM_setRisingEdgeDelayCount(base, BOARD_PWM_DEADTIME_TICKS);
    EPWM_setFallingEdgeDelayCount(base, BOARD_PWM_DEADTIME_TICKS);
    EPWM_setDeadBandDelayMode(base, EPWM_DB_RED, true);
    EPWM_setDeadBandDelayMode(base, EPWM_DB_FED, true);
    EPWM_setTimeBaseCounterMode(base, EPWM_COUNTER_MODE_STOP_FREEZE);
    lockTrip(base);
}

static void configurePWMSync(void)
{
    EPWM_disablePhaseShiftLoad(BOARD_U_EPWM_BASE);
    EPWM_enableSyncOutPulseSource(BOARD_U_EPWM_BASE,
                                  EPWM_SYNC_OUT_PULSE_ON_CNTR_ZERO);
    EPWM_setSyncInPulseSource(BOARD_V_EPWM_BASE,
                              EPWM_SYNC_IN_PULSE_SRC_SYNCOUT_EPWM1);
    EPWM_setSyncInPulseSource(BOARD_W_EPWM_BASE,
                              EPWM_SYNC_IN_PULSE_SRC_SYNCOUT_EPWM1);
    EPWM_setPhaseShift(BOARD_V_EPWM_BASE, 0U);
    EPWM_setPhaseShift(BOARD_W_EPWM_BASE, 0U);
    EPWM_setCountModeAfterSync(BOARD_V_EPWM_BASE,
                               EPWM_COUNT_MODE_UP_AFTER_SYNC);
    EPWM_setCountModeAfterSync(BOARD_W_EPWM_BASE,
                               EPWM_COUNT_MODE_UP_AFTER_SYNC);
    EPWM_enablePhaseShiftLoad(BOARD_V_EPWM_BASE);
    EPWM_enablePhaseShiftLoad(BOARD_W_EPWM_BASE);
}

static void configureADCModule(uint32_t base)
{
    ADC_setPrescaler(base, ADC_CLK_DIV_4_0);
    ADC_setInterruptPulseMode(base, ADC_PULSE_END_OF_CONV);
    ADC_enableConverter(base);
}

static void configureADC(void)
{
    /* F28003x shares the reference; this also loads offset trim for all ADCs. */
    ADC_setVREF(ADCA_BASE, ADC_REFERENCE_INTERNAL, ADC_REFERENCE_3_3V);

    configureADCModule(ADCA_BASE);
    configureADCModule(ADCB_BASE);
    configureADCModule(ADCC_BASE);

    /* Same ePWM1 SOCA event; ADCB VBUS follows its current conversion. */
    ADC_setupSOC(BOARD_IU_ADC_BASE, ADC_SOC_NUMBER0,
                 ADC_TRIGGER_EPWM1_SOCA, BOARD_IU_ADC_CH, BOARD_ADC_ACQPS);
    ADC_setupSOC(BOARD_IV_ADC_BASE, ADC_SOC_NUMBER0,
                 ADC_TRIGGER_EPWM1_SOCA, BOARD_IV_ADC_CH, BOARD_ADC_ACQPS);
    ADC_setupSOC(BOARD_IW_ADC_BASE, ADC_SOC_NUMBER0,
                 ADC_TRIGGER_EPWM1_SOCA, BOARD_IW_ADC_CH, BOARD_ADC_ACQPS);
    ADC_setupSOC(BOARD_VBUS_ADC_BASE, ADC_SOC_NUMBER1,
                 ADC_TRIGGER_EPWM1_SOCA, BOARD_VBUS_ADC_CH, BOARD_ADC_ACQPS);

    EPWM_setADCTriggerSource(BOARD_U_EPWM_BASE, EPWM_SOC_A,
                             EPWM_SOC_TBCTR_PERIOD);
    EPWM_setADCTriggerEventPrescale(BOARD_U_EPWM_BASE, EPWM_SOC_A, 1U);
    EPWM_disableADCTrigger(BOARD_U_EPWM_BASE, EPWM_SOC_A);
}

static void configureCMPSSFailClosed(uint32_t base)
{
    /* Fail-closed DAC placeholders; forced OST remains the primary lock. */
    CMPSS_configHighComparator(base, CMPSS_INSRC_DAC);
    CMPSS_configLowComparator(base, CMPSS_INSRC_DAC | CMPSS_INV_INVERTED);
    CMPSS_configDAC(base, CMPSS_DACREF_VDDA |
                           CMPSS_DACVAL_SYSCLK | CMPSS_DACSRC_SHDW);
    CMPSS_setDACValueHigh(base, 0U);
    CMPSS_setDACValueLow(base, 4095U);
    CMPSS_configOutputsHigh(base, CMPSS_TRIP_ASYNC_COMP);
    CMPSS_configOutputsLow(base, CMPSS_TRIP_ASYNC_COMP);
    CMPSS_enableModule(base);
}

static void configureProtectionRoute(uint32_t epwmBase)
{
    EPWM_enableDigitalCompareTripCombinationInput(
        epwmBase, EPWM_DC_COMBINATIONAL_TRIPIN4, EPWM_DC_TYPE_DCAH);
    EPWM_setTripZoneDigitalCompareEventCondition(
        epwmBase, EPWM_TZ_DC_OUTPUT_A1, EPWM_TZ_EVENT_DCXH_HIGH);
    EPWM_setDigitalCompareEventSource(epwmBase, EPWM_DC_MODULE_A,
                                      EPWM_DC_EVENT_1,
                                      EPWM_DC_EVENT_SOURCE_ORIG_SIGNAL);
    EPWM_enableTripZoneSignals(epwmBase, EPWM_TZ_SIGNAL_DCAEVT1);
}

static void configureProtection(void)
{
    /* SPRSP61C Fig. 6-37, 100-pin PZ: HPMXSEL0/LPMXSEL0 selects
       A2/B6/C9 for CMPSS1 (IU), A4/B8 for CMPSS2 (IW),
       B2/C6 for CMPSS3 (IV). Thresholds remain lock-only placeholders. */
    ASysCtl_selectCMPHPMux(ASYSCTL_CMPHPMUX_SELECT_1, 0U);
    ASysCtl_selectCMPLPMux(ASYSCTL_CMPLPMUX_SELECT_1, 0U);
    ASysCtl_selectCMPHPMux(ASYSCTL_CMPHPMUX_SELECT_2, 0U);
    ASysCtl_selectCMPLPMux(ASYSCTL_CMPLPMUX_SELECT_2, 0U);
    ASysCtl_selectCMPHPMux(ASYSCTL_CMPHPMUX_SELECT_3, 0U);
    ASysCtl_selectCMPLPMux(ASYSCTL_CMPLPMUX_SELECT_3, 0U);
    configureCMPSSFailClosed(BOARD_IU_CMPSS_BASE);
    configureCMPSSFailClosed(BOARD_IV_CMPSS_BASE);
    configureCMPSSFailClosed(BOARD_IW_CMPSS_BASE);

    XBAR_setEPWMMuxConfig(XBAR_TRIP4,
                           XBAR_EPWM_MUX00_CMPSS1_CTRIPH_OR_L);
    XBAR_setEPWMMuxConfig(XBAR_TRIP4,
                           XBAR_EPWM_MUX02_CMPSS2_CTRIPH_OR_L);
    XBAR_setEPWMMuxConfig(XBAR_TRIP4,
                           XBAR_EPWM_MUX04_CMPSS3_CTRIPH_OR_L);
    XBAR_enableEPWMMux(XBAR_TRIP4, XBAR_MUX00 | XBAR_MUX02 | XBAR_MUX04);

    configureProtectionRoute(BOARD_U_EPWM_BASE);
    configureProtectionRoute(BOARD_V_EPWM_BASE);
    configureProtectionRoute(BOARD_W_EPWM_BASE);
}

static void configureEncoder(void)
{
    /* J12 routing still depends on the physical S5 switch position. */
    GPIO_setPinConfig(GPIO_40_EQEP1_A);
    GPIO_setPinConfig(GPIO_41_EQEP1_B);
    GPIO_setPinConfig(GPIO_59_EQEP1_INDEX);
    EQEP_setDecoderConfig(BOARD_ENCODER_EQEP_BASE,
                          EQEP_CONFIG_QUADRATURE |
                          EQEP_CONFIG_2X_RESOLUTION |
                          EQEP_CONFIG_NO_SWAP);
    EQEP_setPositionCounterConfig(BOARD_ENCODER_EQEP_BASE,
                                  EQEP_POSITION_RESET_MAX_POS,
                                  0xFFFFFFFFU);
    EQEP_enableModule(BOARD_ENCODER_EQEP_BASE);
}

void Board_initStage1Locked(void)
{
    static const uint32_t outputPins[6] = {0U, 1U, 2U, 3U, 10U, 11U};
    static const uint32_t gpioConfigs[6] = {
        GPIO_0_GPIO0, GPIO_1_GPIO1, GPIO_2_GPIO2,
        GPIO_3_GPIO3, GPIO_10_GPIO10, GPIO_11_GPIO11
    };
    uint16_t i;

    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
    configurePWM(BOARD_U_EPWM_BASE);
    configurePWM(BOARD_V_EPWM_BASE);
    configurePWM(BOARD_W_EPWM_BASE);
    configurePWMSync();
    configureADC();
    configureProtection();
    configureEncoder();

    /* The six physical pins stay GPIO outputs, never ePWM outputs. */
    for(i = 0U; i < 6U; i++)
    {
        GPIO_writePin(outputPins[i], 0U);
        GPIO_setPinConfig(gpioConfigs[i]);
        GPIO_setPadConfig(outputPins[i], GPIO_PIN_TYPE_STD);
        GPIO_setDirectionMode(outputPins[i], GPIO_DIR_MODE_OUT);
    }
    GPIO_setPinConfig(GPIO_8_GPIO8);
    GPIO_setPadConfig(BOARD_SD_A_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(BOARD_SD_A_GPIO, GPIO_DIR_MODE_IN);
}

static void configureCapture(void)
{
    /* EOC flags are polled; CPU ADC interrupts are intentionally not enabled. */
    ADC_setInterruptSource(ADCA_BASE, ADC_INT_NUMBER1, ADC_SOC_NUMBER0);
    ADC_setInterruptSource(ADCB_BASE, ADC_INT_NUMBER1, ADC_SOC_NUMBER1);
    ADC_setInterruptSource(ADCC_BASE, ADC_INT_NUMBER1, ADC_SOC_NUMBER0);
    ADC_enableInterrupt(ADCA_BASE, ADC_INT_NUMBER1);
    ADC_enableInterrupt(ADCB_BASE, ADC_INT_NUMBER1);
    ADC_enableInterrupt(ADCC_BASE, ADC_INT_NUMBER1);

    CPUTimer_setPeriod(CPUTIMER0_BASE, 0xFFFFFFFFUL);
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0U);
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    CPUTimer_startTimer(CPUTIMER0_BASE);
}

void Board_pollProtection(void)
{
    if((EPWM_getTripZoneFlagStatus(BOARD_U_EPWM_BASE) &
        EPWM_TZ_FLAG_DCAEVT1) != 0U)
    {
        gTripEventMask |= 1U;
    }
    if((EPWM_getTripZoneFlagStatus(BOARD_V_EPWM_BASE) &
        EPWM_TZ_FLAG_DCAEVT1) != 0U)
    {
        gTripEventMask |= 2U;
    }
    if((EPWM_getTripZoneFlagStatus(BOARD_W_EPWM_BASE) &
        EPWM_TZ_FLAG_DCAEVT1) != 0U)
    {
        gTripEventMask |= 4U;
    }
    if((gTripEventMask != 0U) && (gProtectionLatch == 0U))
    {
        gProtectionLatch = 1U;
        EPWM_forceTripZoneEvent(BOARD_U_EPWM_BASE, EPWM_TZ_FORCE_EVENT_OST);
        EPWM_forceTripZoneEvent(BOARD_V_EPWM_BASE, EPWM_TZ_FORCE_EVENT_OST);
        EPWM_forceTripZoneEvent(BOARD_W_EPWM_BASE, EPWM_TZ_FORCE_EVENT_OST);
    }
}



void Wildfire_initBoardLocked(void)
{
    Board_initStage1Locked();
    DEVICE_DELAY_US(5000);
    configureCapture();
}
