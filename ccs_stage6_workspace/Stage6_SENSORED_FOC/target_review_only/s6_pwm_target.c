/* REVIEW-ONLY OBJECT. Excluded from every loadable locked configuration.
   Real three-phase shadow/global-load commit. No gate enable, GPIO remux,
   trip clear or protection-threshold modification is provided here. */
#include "device.h"
#include "board_hal.h"
#include "s6_pwm_target.h"
#include <string.h>
static const uint32_t bases[3]={EPWM1_BASE,EPWM2_BASE,EPWM6_BASE};
static int stop(S6_TargetPWM *t,uint32_t fault,uint32_t seq){
 unsigned j;for(j=0;j<3;j++)EPWM_forceTripZoneEvent(bases[j],EPWM_TZ_FORCE_EVENT_OST);
 S6_pwmFault(&t->pwm,fault,seq);return 0;
}
int S6T_configure(S6_TargetPWM *t,const S6_PWMConfig *c){
 unsigned j;memset(t,0,sizeof(*t));
 if(!S6_pwmInit(&t->pwm,c,S6_PWM_TARGET) || !c->window_reviewed || !c->polarity_reviewed || !c->sample_at_zero)
  return stop(t,S6_PWM_CONFIG,0UL);
 /* This configuration is only safe before an independent arming procedure,
    with all three OST asserted and TBCLKSYNC disabled by the caller. */
 if(HWREG(CPUSYS_BASE+SYSCTL_O_PCLKCR0)&SYSCTL_PCLKCR0_TBCLKSYNC)return stop(t,S6_PWM_CONFIG,0UL);
 for(j=0;j<3;j++){
  if(!(EPWM_getTripZoneFlagStatus(bases[j])&EPWM_TZ_FLAG_OST))return stop(t,S6_PWM_CONFIG,0UL);
  EPWM_setCounterCompareShadowLoadMode(bases[j],EPWM_COUNTER_COMPARE_A,EPWM_COMP_LOAD_FREEZE);
  EPWM_enableGlobalLoadRegisters(bases[j],EPWM_GL_REGISTER_CMPA_CMPAHR);
  EPWM_setGlobalLoadTrigger(bases[j],EPWM_GL_LOAD_PULSE_CNTR_ZERO);
  EPWM_setGlobalLoadEventPrescale(bases[j],1U);
  EPWM_enableGlobalLoadOneShotMode(bases[j]);EPWM_enableGlobalLoad(bases[j]);
  EPWM_setupEPWMLinks(bases[j],EPWM_LINK_WITH_EPWM_1,EPWM_LINK_GLDCTL2);
 }
 /* Paired acquisition/update design: ZERO S/H -> next ZERO shadow load.
    Not a standalone SOCA move. Board timing, duty bounds and settling must
    match the reviewed c, with ADC timing/FOC horizon kept in that contract. */
 EPWM_setADCTriggerSource(EPWM1_BASE,EPWM_SOC_A,EPWM_SOC_TBCTR_ZERO);
 t->configured=1U;return 1;
}
int S6T_observeLoad(S6_TargetPWM *t,uint32_t session){
 uint32_t now=CPUTimer_getTimerCount(CPUTIMER0_BASE);
 if(!t->configured || !t->pwm.pending)return 0;
 if(HWREGH(EPWM1_BASE+EPWM_O_GLDCTL2)&EPWM_GLDCTL2_OSHTLD)return 0;
 /* The flag observation is NOT a hardware-latched load timestamp. */
 if(now-t->pwm.committed.apply<0x80000000UL && now!=t->pwm.committed.apply)return stop(t,S6_PWM_ORDER,t->pwm.committed.sequence);
 if(!S6_pwmLoaded(&t->pwm,t->pwm.committed.sequence,session))return stop(t,S6_PWM_ORDER,t->pwm.committed.sequence);
 t->last_observed=now;return 1;
}
int S6T_submit(S6_TargetPWM *t,const float duty[3],uint32_t seq,uint32_t session,uint32_t release,uint16_t permit){
 unsigned j;uint32_t now;
 if(!t->configured || permit!=127U)return stop(t,S6_PWM_AUTH,seq);
 if(t->pwm.pending && !S6T_observeLoad(t,session))return stop(t,S6_PWM_ORDER,seq);
 for(j=0;j<3;j++)if(EPWM_getTripZoneFlagStatus(bases[j])&0x7FU)return stop(t,S6_PWM_BACKEND,seq);
 if(!S6_pwmPrepare(&t->pwm,duty,seq,session,release))return stop(t,t->pwm.first_error,seq);
 now=CPUTimer_getTimerCount(CPUTIMER0_BASE);
 if(!S6_pwmCheckCommit(&t->pwm,now,session,1U))return stop(t,t->pwm.first_error,seq);
 /* ISR single owner, INTM set; no independent phase load until master latch.
    If a deadline expires during the writes, all outputs trip before arming. */
 for(j=0;j<3;j++)EPWM_setCounterCompareValue(bases[j],EPWM_COUNTER_COMPARE_A,t->pwm.prepared.compare[j]);
 now=CPUTimer_getTimerCount(CPUTIMER0_BASE);
 if(!S6_pwmCommit(&t->pwm,now,session,1U))return stop(t,t->pwm.first_error,seq);
 EPWM_setGlobalLoadOneShotLatch(EPWM1_BASE); /* linked to all three GLDCTL2 */
 return 1;
}
