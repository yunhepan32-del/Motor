#include "device.h"
#include "board_hal.h"
#include "wildfire_lock.h"
#include "s6_app.h"
#include "s6_port.h"
static uint16_t running=0U;
__interrupt void Stage6_adcB1ISR(void) {uint32_t entry=CPUTimer_getTimerCount(CPUTIMER0_BASE);S6_onISR(entry);S6_onExit(CPUTimer_getTimerCount(CPUTIMER0_BASE));}
uint32_t S6P_time(void) {return CPUTimer_getTimerCount(CPUTIMER0_BASE);}
uint32_t S6P_position(void) {return EQEP_getPosition(BOARD_ENCODER_EQEP_BASE);}
void S6P_init(void) {Interrupt_register(INT_ADCB1,&Stage6_adcB1ISR);}

static uint16_t S6P_sd(void) {return (uint16_t)GPIO_readPin(BOARD_SD_A_GPIO);}
void S6P_overflow(uint16_t v[3]) {
    v[0]=HWREGH(ADCA_BASE+ADC_O_INTOVF);v[1]=HWREGH(ADCB_BASE+ADC_O_INTOVF);
    v[2]=HWREGH(ADCC_BASE+ADC_O_INTOVF);
}
uint16_t S6P_preflight(void) {
    uint16_t values[6];
    /* Nothing is acknowledged here. Stale completion/overflow requires a new
       separately authorized investigation, never automatic clearing/retry. */
    values[0]=HWREGH(ADCA_BASE+ADC_O_INTFLG);values[1]=HWREGH(ADCB_BASE+ADC_O_INTFLG);
    values[2]=HWREGH(ADCC_BASE+ADC_O_INTFLG);
    S6P_overflow(&values[3]);
    return (uint16_t)((values[0]|values[1]|values[2]|values[3]|values[4]|values[5])&1U);
}
void S6P_start(void) {
    if(running==0U) {
        /* Acquisition only. Existing compare, SOCA source/prescale, clock
           dividers, GPIO mux, CMPSS/XBAR/TZ and OST are never changed. */
        EPWM_setTimeBaseCounter(BOARD_U_EPWM_BASE,0U);
        EPWM_setTimeBaseCounter(BOARD_V_EPWM_BASE,0U);
        EPWM_setTimeBaseCounter(BOARD_W_EPWM_BASE,0U);
        EPWM_setTimeBaseCounterMode(BOARD_U_EPWM_BASE,EPWM_COUNTER_MODE_UP_DOWN);
        EPWM_setTimeBaseCounterMode(BOARD_V_EPWM_BASE,EPWM_COUNTER_MODE_UP_DOWN);
        EPWM_setTimeBaseCounterMode(BOARD_W_EPWM_BASE,EPWM_COUNTER_MODE_UP_DOWN);
        running=1U;Interrupt_enable(INT_ADCB1);Interrupt_enableMaster();
        EPWM_enableADCTrigger(BOARD_U_EPWM_BASE,EPWM_SOC_A);
        SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
    } else Interrupt_enableMaster();
}
void S6P_stop(void) {
    EPWM_disableADCTrigger(BOARD_U_EPWM_BASE,EPWM_SOC_A);
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
    EPWM_setTimeBaseCounterMode(BOARD_U_EPWM_BASE,EPWM_COUNTER_MODE_STOP_FREEZE);
    EPWM_setTimeBaseCounterMode(BOARD_V_EPWM_BASE,EPWM_COUNTER_MODE_STOP_FREEZE);
    EPWM_setTimeBaseCounterMode(BOARD_W_EPWM_BASE,EPWM_COUNTER_MODE_STOP_FREEZE);
    Interrupt_disable(INT_ADCB1);running=0U;
}
/* No per-frame foreground handshake or interrupt-masking section. */

void S6P_interruptAck(void) {Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);}
void S6P_read(S6_Input *s) {
    s->entry=S6P_time();
    s->flags[0]=HWREGH(ADCA_BASE+ADC_O_INTFLG);
    s->flags[1]=HWREGH(ADCB_BASE+ADC_O_INTFLG);
    s->flags[2]=HWREGH(ADCC_BASE+ADC_O_INTFLG);
    S6P_overflow(s->overflow);
    
    s->raw[0]=ADC_readResult(ADCCRESULT_BASE,ADC_SOC_NUMBER0);
    s->raw[1]=ADC_readResult(ADCBRESULT_BASE,ADC_SOC_NUMBER0);
    s->raw[2]=ADC_readResult(ADCARESULT_BASE,ADC_SOC_NUMBER0);
    s->raw[3]=ADC_readResult(ADCBRESULT_BASE,ADC_SOC_NUMBER1);
    S6P_overflow(s->pre_overflow);
}
void S6P_feedback(S6_Input *s) {
    uint32_t phase_tick;uint16_t tbctr,tbsts;
    s->qep_tick=S6P_time();s->position=S6P_position();
    s->qep_status=EQEP_getStatus(BOARD_ENCODER_EQEP_BASE);
    s->qep_flags=EQEP_getInterruptStatus(BOARD_ENCODER_EQEP_BASE);
    /* TBSTS/TBCTR/Timer0 are sequential reads. This is an estimated SOCA
       event time, NOT a hardware-latched S/H aperture timestamp. */
    tbsts=HWREGH(BOARD_U_EPWM_BASE+EPWM_O_TBSTS);
    tbctr=EPWM_getTimeBaseCounterValue(BOARD_U_EPWM_BASE);phase_tick=S6P_time();
    s->release=phase_tick+(((tbsts&EPWM_TBSTS_CTRDIR)==0U)?
        3000UL-(uint32_t)tbctr:3000UL+(uint32_t)tbctr);
}
void S6P_ack(const S6_Input *s) {
    unsigned j;
    for(j=0;j<3;j++)if(!(s->flags[j]&1U) || ((s->overflow[j]|s->pre_overflow[j])&1U))return;
    ADC_clearInterruptStatus(ADCA_BASE,ADC_INT_NUMBER1);
    ADC_clearInterruptStatus(ADCB_BASE,ADC_INT_NUMBER1);
    ADC_clearInterruptStatus(ADCC_BASE,ADC_INT_NUMBER1);
}
void S6P_protection(uint16_t *trip,uint16_t *latch,uint16_t tz[3]) {
    Board_pollProtection();Wildfire_forceOutputsLocked();
    *trip=gTripEventMask;*latch=gProtectionLatch;
    tz[0]=EPWM_getTripZoneFlagStatus(BOARD_U_EPWM_BASE);
    tz[1]=EPWM_getTripZoneFlagStatus(BOARD_V_EPWM_BASE);
    tz[2]=EPWM_getTripZoneFlagStatus(BOARD_W_EPWM_BASE);
}
void S6P_registers(uint16_t v[32]) {
    uint32_t x;
    v[0]=HWREGH(ANALOGSUBSYS_BASE+ASYSCTL_O_ANAREFCTL);
    v[1]=HWREGH(ADCA_BASE+ADC_O_CTL1);v[2]=HWREGH(ADCB_BASE+ADC_O_CTL1);v[3]=HWREGH(ADCC_BASE+ADC_O_CTL1);
    v[4]=HWREGH(ADCA_BASE+ADC_O_CTL2);v[5]=HWREGH(ADCB_BASE+ADC_O_CTL2);v[6]=HWREGH(ADCC_BASE+ADC_O_CTL2);
    v[7]=HWREGH(ADCA_BASE+ADC_O_INTSEL1N2);v[8]=HWREGH(ADCB_BASE+ADC_O_INTSEL1N2);v[9]=HWREGH(ADCC_BASE+ADC_O_INTSEL1N2);
    x=HWREG(ADCA_BASE+ADC_O_SOC0CTL);v[10]=(uint16_t)x;v[11]=(uint16_t)(x>>16);
    x=HWREG(ADCB_BASE+ADC_O_SOC0CTL);v[12]=(uint16_t)x;v[13]=(uint16_t)(x>>16);
    x=HWREG(ADCB_BASE+ADC_O_SOC1CTL);v[14]=(uint16_t)x;v[15]=(uint16_t)(x>>16);
    x=HWREG(ADCC_BASE+ADC_O_SOC0CTL);v[16]=(uint16_t)x;v[17]=(uint16_t)(x>>16);
    v[18]=HWREGH(BOARD_U_EPWM_BASE+EPWM_O_TBCTL);v[19]=HWREGH(BOARD_U_EPWM_BASE+EPWM_O_TBPRD);
    v[20]=HWREGH(BOARD_U_EPWM_BASE+EPWM_O_ETSEL);v[21]=HWREGH(BOARD_U_EPWM_BASE+EPWM_O_ETPS);
    v[22]=HWREGH(CPUTIMER0_BASE+CPUTIMER_O_TCR);v[23]=HWREGH(CPUTIMER0_BASE+CPUTIMER_O_TPR);v[24]=HWREGH(CPUTIMER0_BASE+CPUTIMER_O_TPRH);
    v[25]=HWREGH(BOARD_ENCODER_EQEP_BASE+EQEP_O_QDECCTL);v[26]=HWREGH(BOARD_ENCODER_EQEP_BASE+EQEP_O_QEPCTL);
    v[27]=EPWM_getTripZoneFlagStatus(BOARD_U_EPWM_BASE);v[28]=EPWM_getTripZoneFlagStatus(BOARD_V_EPWM_BASE);v[29]=EPWM_getTripZoneFlagStatus(BOARD_W_EPWM_BASE);
    v[30]=(uint16_t)(GPIO_readPin(0U)|(GPIO_readPin(1U)<<1U)|(GPIO_readPin(2U)<<2U)|
        (GPIO_readPin(3U)<<3U)|(GPIO_readPin(10U)<<4U)|(GPIO_readPin(11U)<<5U));
    v[31]=S6P_sd();
}

void S6P_backgroundLock(void) {Board_pollProtection();Wildfire_forceOutputsLocked();}
void S6P_finishInput(S6_Input *s) {
    S6P_feedback(s);S6P_protection(&s->trip,&s->latch,s->tz);
    s->sample_tick=s->release;s->gpio_mask=(uint16_t)(GPIO_readPin(0U)|(GPIO_readPin(1U)<<1U)|(GPIO_readPin(2U)<<2U)|(GPIO_readPin(3U)<<3U)|(GPIO_readPin(10U)<<4U)|(GPIO_readPin(11U)<<5U));
    s->sd=S6P_sd();
}
uint16_t S6P_stagePWM(const uint16_t compare[3],uint16_t requested) {
    unsigned j;
    for(j=0;j<3;j++)if(compare[j]>BOARD_PWM_PERIOD_TICKS)return 1U;
    (void)requested;
    /* Intentional locked-build boundary: NO compare write, OST clear,
       GPIO remux or gate-enable implementation is compiled. */
    return 0U;
}
