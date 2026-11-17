#include "s6_control.h"
#include <limits.h>
static float clamp(float x,float lo,float hi){return x<lo?lo:(x>hi?hi:x);}
int S6_speedConfigValid(const S6_Config *c){
 return c->speed_divider>=1U && c->speed_divider<=100U &&
  c->speed_kp_A_per_rad_s>=0.0f && c->speed_kp_A_per_rad_s<=10.0f &&
  c->speed_ki_A_per_rad>=0.0f && c->speed_ki_A_per_rad<=100.0f &&
  c->speed_iq_limit_A>0.0f && c->speed_iq_limit_A<=c->foc.current_limit_A &&
  c->speed_slew_rad_s2>0.0f && c->speed_slew_rad_s2<=10000.0f &&
  c->iq_slew_A_s>0.0f && c->iq_slew_A_s<=10000.0f;
}
int S6_speedStep(S6_Speed *s,const S6_Config *c,int32_t delta,uint32_t ticks,float request,float old_iq){
 float dt,error,candidate,out,step;
 if(!S6_finite(request) || !S6_finite(old_iq) || !(c->speed_kp_A_per_rad_s>=0.0f && c->speed_kp_A_per_rad_s<=10.0f) ||
    !(c->speed_ki_A_per_rad>=0.0f && c->speed_ki_A_per_rad<=100.0f) ||
    !(c->speed_iq_limit_A>0.0f && c->speed_iq_limit_A<=c->foc.current_limit_A) ||
    !(c->speed_slew_rad_s2>0.0f && c->speed_slew_rad_s2<=10000.0f) ||
    !(c->iq_slew_A_s>0.0f && c->iq_slew_A_s<=10000.0f) ||
    ticks==0U || ticks>6600UL || s->count>=c->speed_divider ||
    (delta>0 && s->count_sum>INT32_MAX-delta) ||
    (delta<0 && s->count_sum<INT32_MIN-delta) || UINT32_MAX-s->ticks_sum<ticks)return 0;
 s->count_sum+=delta;s->ticks_sum+=ticks;s->count++;
 if(s->count<c->speed_divider)return 1;
 dt=(float)s->ticks_sum/120000000.0f;
 s->feedback=(float)s->count_sum*(6.2831853071795864769f/(float)c->foc.encoder.counts_per_rev)/dt;
 step=c->speed_slew_rad_s2*dt;s->reference+=clamp(request-s->reference,-step,step);
 error=s->reference-s->feedback;
 if(!s->primed){s->integral=clamp(old_iq-c->speed_kp_A_per_rad_s*error,-c->speed_iq_limit_A,c->speed_iq_limit_A);s->iq=old_iq;s->primed=1U;}
 candidate=s->integral+c->speed_ki_A_per_rad*dt*error;
 out=c->speed_kp_A_per_rad_s*error+candidate;
 if((out>c->speed_iq_limit_A && error>0.0f) || (out< -c->speed_iq_limit_A && error<0.0f))candidate=s->integral;
 s->integral=clamp(candidate,-c->speed_iq_limit_A,c->speed_iq_limit_A);
 s->target=clamp(c->speed_kp_A_per_rad_s*error+s->integral,-c->speed_iq_limit_A,c->speed_iq_limit_A);
 step=c->iq_slew_A_s*dt;s->iq+=clamp(s->target-s->iq,-step,step);
 s->count=0U;s->count_sum=0;s->ticks_sum=0U;
 return S6_finite(s->iq)&&S6_finite(s->feedback);
}
