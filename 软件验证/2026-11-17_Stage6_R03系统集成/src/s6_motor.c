#include "s6_motor.h"
#include "s6_motor_profile.h"
#include <string.h>
#include <math.h>
void S6_motorLoad(S6_Motor *m){
 memset(m,0,sizeof(*m));m->build=S6_MOTOR_BUILD;m->reviewed=S6_MOTOR_REVIEWED;
 m->Rs=S6_MOTOR_RS;m->Ld=S6_MOTOR_LD;m->Lq=S6_MOTOR_LQ;m->flux=S6_MOTOR_FLUX;
 m->J=S6_MOTOR_J;m->friction=S6_MOTOR_B;m->rated_current=S6_MOTOR_CURRENT;
 m->max_speed=S6_MOTOR_SPEED;m->current_bandwidth_Hz=S6_MOTOR_CURRENT_HZ;
 m->speed_bandwidth_Hz=S6_MOTOR_SPEED_HZ;m->damping=S6_MOTOR_DAMPING;
 m->speed_slew_rad_s2=S6_MOTOR_SPEED_SLEW;m->iq_slew_A_s=S6_MOTOR_IQ_SLEW;
 m->counts_per_rev=S6_MOTOR_CPR;m->pole_pairs=S6_MOTOR_POLES;m->direction=S6_MOTOR_DIRECTION;
}
int S6_motorValid(const S6_Motor *m){
 return m->build==S6_BUILD && m->reviewed==1U && m->Rs>0.0f && m->Rs<=100.0f &&
  m->Ld>0.000001f && m->Ld<1.0f && m->Lq==m->Ld &&
  m->flux>0.0f && m->flux<10.0f && m->J>0.0f && m->J<10.0f && m->friction>=0.0f && m->friction<10.0f &&
  m->rated_current>0.0f && m->rated_current<=2.5f && m->max_speed>0.0f && m->max_speed<=1000.0f &&
  m->current_bandwidth_Hz>0.0f && m->current_bandwidth_Hz<=1000.0f &&
  m->speed_bandwidth_Hz>0.0f && m->speed_bandwidth_Hz<=50.0f &&
  m->speed_bandwidth_Hz<=0.1f*m->current_bandwidth_Hz && m->damping>=0.5f && m->damping<=2.0f &&
  m->speed_slew_rad_s2>0.0f && m->speed_slew_rad_s2<=10000.0f && m->iq_slew_A_s>0.0f && m->iq_slew_A_s<=10000.0f &&
  m->counts_per_rev>=4UL && m->counts_per_rev<=16777216UL && m->pole_pairs>0U && m->pole_pairs<=100U &&
  (m->direction==1 || m->direction==-1);
}
int S6_motorApply(const S6_Motor *m,S6_Config *c){
 float wi,ws,kt,kp,ki,sp,si;
 if(!S6_motorValid(m) || c->speed_divider!=20U)return 0;
 wi=6.28318530718f*m->current_bandwidth_Hz;ws=6.28318530718f*m->speed_bandwidth_Hz;
 kt=1.5f*(float)m->pole_pairs*m->flux;
 kp=m->Ld*wi;ki=m->Rs/m->Ld*0.00005f;
 sp=2.0f*m->damping*ws*m->J/kt;si=ws*ws*m->J/kt;
 if(!(kp>0.0f && kp<=100.0f && ki>0.0f && ki<=1.0f && sp<=10.0f && si<=100.0f))return 0;
 c->foc.kp_V_per_A=kp;c->foc.ki_series=ki;
 c->speed_kp_A_per_rad_s=sp;c->speed_ki_A_per_rad=si;
 if(m->rated_current<c->foc.current_limit_A)c->foc.current_limit_A=m->rated_current;
 c->speed_iq_limit_A=c->foc.current_limit_A;c->maximum_speed_rad_s=m->max_speed;
 c->speed_slew_rad_s2=m->speed_slew_rad_s2;c->iq_slew_A_s=m->iq_slew_A_s;
 c->foc.encoder.counts_per_rev=m->counts_per_rev;c->foc.encoder.pole_pairs=m->pole_pairs;c->foc.encoder.direction=m->direction;
 /* This module grants only control-model evidence, never sensor/power/zero. */
 c->evidence|=S6_E_GAINS;return 1;
}
