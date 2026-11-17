#ifndef S6_PWM_TARGET_H
#define S6_PWM_TARGET_H
#include "s6_pwm.h"
typedef struct {S6_PWM pwm;uint16_t configured;uint32_t last_observed;} S6_TargetPWM;
int S6T_configure(S6_TargetPWM *t,const S6_PWMConfig *c);
int S6T_submit(S6_TargetPWM *t,const float duty[3],uint32_t seq,uint32_t session,uint32_t release,uint16_t permit);
int S6T_observeLoad(S6_TargetPWM *t,uint32_t session);
#endif
