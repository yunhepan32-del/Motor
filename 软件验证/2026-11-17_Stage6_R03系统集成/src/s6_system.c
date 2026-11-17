#include "s6_system.h"
#include <string.h>
#include <math.h>
#include <limits.h>
static void move(S6_System *s,uint16_t state){s->state=state;if(s->transitions<UINT32_MAX)s->transitions++;}
void S6_systemInit(S6_System *s,uint16_t backend){
 memset(s,0,sizeof(*s));s->build=S6_BUILD;s->session=S6_BUILD;s->backend=backend;
 s->state=S6_SYS_INIT;move(s,S6_SYS_DISABLED);
}
uint16_t S6_systemQualifications(const S6_System *s,const S6_State *c,uint32_t seq){
 uint16_t q=0U,b=S6_blockers(c),e=c->config.evidence;
 if(!(b&(S6_B_INTERFACE|S6_B_CURRENT|S6_B_OFFSET|S6_B_BUS)))q|=S6_Q_FEEDBACK;
 if(!(b&S6_B_ENCODER))q|=S6_Q_ENCODER;
 if(!(b&S6_B_GAINS) && S6_configValid(&c->config))q|=S6_Q_CONTROL;
 if(e&S6_E_PWM)q|=S6_Q_PWM;
 if(e&S6_E_PROTECTION)q|=S6_Q_PROTECTION;
 if(e&S6_E_REALTIME)q|=S6_Q_REALTIME;
 if(s->authority_session==s->session && seq && seq<=s->authority_until &&
    s->authority_until-seq<=40000UL && s->authority_modes)q|=S6_Q_OPERATOR;
 return q;
}
void S6_systemFault(S6_System *s,S6_State *c,uint32_t bits,uint32_t seq){
 if(!s->first_fault){s->first_fault=bits;s->first_sequence=seq;}
 S6_fault(c,bits,seq);move(s,S6_SYS_FAULT);s->authority_modes=0U;s->power_active=0U;
}
static int deny(S6_System *s){if(s->denied<65535U)s->denied++;return 0;}
int S6_systemBegin(S6_System *s,S6_State *c,uint16_t command){
 if(command==S6_CMD_STOP)return S6_begin(c,command);
 if(s->state!=S6_SYS_DISABLED || s->first_fault || !S6_begin(c,command))return deny(s);
 if(c->state==S6_CURRENT_SHADOW || c->state==S6_SPEED_SHADOW)move(s,S6_SYS_SHADOW);
 else if(c->state==S6_CALIBRATING)move(s,S6_SYS_CALIBRATION);
 return 1;
}
int S6_systemReferences(S6_System *s,const S6_State *c,float id,float iq,float speed){
 if(c->faults || !S6_finite(id) || !S6_finite(iq) || !S6_finite(speed) ||
   fabsf(id)>c->config.foc.current_limit_A || fabsf(iq)>c->config.foc.current_limit_A ||
   id*id+iq*iq>c->config.foc.current_limit_A*c->config.foc.current_limit_A ||
   fabsf(speed)>c->config.maximum_speed_rad_s || (c->state==S6_SPEED_SHADOW && id!=0.0f) || s->state==S6_SYS_STOPPING || s->state==S6_SYS_FAULT)return 0;
 s->requested_id=id;s->requested_iq=iq;s->requested_speed=speed;s->references_pending=1U;return 1;
}
int S6_systemEvent(S6_System *s,S6_State *c,uint16_t event,uint32_t seq){
 uint16_t q=S6_systemQualifications(s,c,seq);s->qualifications=q;
 if(s->state==S6_SYS_FAULT || c->faults)return deny(s);
 if(event==S6_EVENT_SELFTEST && s->state==S6_SYS_DISABLED){
  move(s,S6_SYS_SELFTEST);
  if(!S6_configValid(&c->config)){S6_systemFault(s,c,S6_F_CONFIG,seq);return 0;}
  if(c->state==S6_STOPPED){
   /* Normal-stop rearm only; no fault fields, trip registers or calibration
      certificates are cleared. Reprime timestamps after a deliberate gap. */
   memset(&c->foc,0,sizeof(c->foc));memset(&c->speed,0,sizeof(c->speed));
   c->primed=c->called=c->pwm_staged=c->conversion_valid=0U;c->state=S6_LOCKED;
   c->id_ref=c->iq_ref=c->speed_ref=0.0f;s->references_pending=0U;
  }
  move(s,(q&7U)==7U?S6_SYS_READY:S6_SYS_DISABLED);return 1;
 }
 if(event==S6_EVENT_CALIBRATE && s->state==S6_SYS_DISABLED){
  if(!S6_begin(c,S6_CMD_BIAS))return deny(s);
  move(s,S6_SYS_CALIBRATION);return 1;
 }
 if(event==S6_EVENT_SHADOW && (s->state==S6_SYS_READY || s->state==S6_SYS_DISABLED)){
  if(!S6_begin(c,S6_CMD_CURRENT))return deny(s);
  move(s,S6_SYS_SHADOW);return 1;
 }
 if(event==S6_EVENT_POWER_PREPARE && s->state==S6_SYS_READY){
  if(s->backend==S6_PWM_LOCKED || q!=127U)return deny(s);
  move(s,S6_SYS_POWER_READY);return 1;
 }
 if((event==S6_EVENT_CURRENT || event==S6_EVENT_SPEED) && s->state==S6_SYS_POWER_READY){
  uint16_t bit=event==S6_EVENT_CURRENT?1U:2U;
  if(q!=127U || !(s->authority_modes&bit) || (event==S6_EVENT_SPEED && !S6_speedConfigValid(&c->config)) || !(c->config.evidence&S6_E_ZERO) ||
     c->config.foc.encoder.zero_origin_epoch!=s->session)return deny(s);
  if(!S6_begin(c,event==S6_EVENT_SPEED?S6_CMD_SPEED:S6_CMD_CURRENT))return deny(s);
  s->power_active=1U;
  move(s,event==S6_EVENT_SPEED?S6_SYS_SPEED:S6_SYS_CURRENT);return 1;
 }
 if(event==S6_EVENT_ALIGN && s->state==S6_SYS_POWER_READY){
  if(q!=127U || !(s->authority_modes&4U) || !S6_alignBegin(&s->alignment,c))return deny(s);
  s->power_active=1U;move(s,S6_SYS_ZERO_ALIGN);return 1;
 }
 if(event==S6_EVENT_STOP && (s->state==S6_SYS_SHADOW || s->state==S6_SYS_CURRENT || s->state==S6_SYS_SPEED)){
  s->stopping_id=c->id_ref;s->stopping_iq=c->iq_ref;s->stop_frames=0U;
  s->reserved=(uint16_t)(s->state==S6_SYS_SPEED?2U:1U);
  s->references_pending=0U;move(s,S6_SYS_STOPPING);return 1;
 }
 return deny(s);
}
int S6_systemSwitch(S6_System *s,S6_State *c,uint16_t speed){
 if(c->faults || (s->state!=S6_SYS_SHADOW && s->state!=S6_SYS_CURRENT && s->state!=S6_SYS_SPEED))return deny(s);
 if(speed && (c->id_ref!=0.0f || (s->references_pending && s->requested_id!=0.0f) || !(c->config.evidence&S6_E_ZERO) || !S6_speedConfigValid(&c->config)))return deny(s);
 if(s->state!=S6_SYS_SHADOW && !(s->authority_modes&(speed?2U:1U)))return deny(s);
 /* Leave the current PI untouched; seed speed PI at next slow update using
    current iq, with a reference starting from measured speed. */
 memset(&c->speed,0,sizeof(c->speed));c->speed.iq=c->iq_ref;
 c->speed.reference=c->foc.output.omega_mech;
 c->state=speed?S6_SPEED_SHADOW:S6_CURRENT_SHADOW;
 if(s->state!=S6_SYS_SHADOW)move(s,speed?S6_SYS_SPEED:S6_SYS_CURRENT);
 return 1;
}
int S6_systemStep(S6_System *s,S6_State *c,const S6_Input *x){
 uint16_t powered=s->power_active;
 if(c->faults){S6_systemFault(s,c,c->first_fault,x->sequence);return 0;}
 if(s->state==S6_SYS_FAULT)return 0;
 if(s->build!=S6_BUILD || s->session!=S6_BUILD || s->state<S6_SYS_DISABLED || s->state>S6_SYS_STOPPING){
  S6_systemFault(s,c,S6_F_CONFIG,x->sequence);return 0;
 }
 if(powered && (s->backend==S6_PWM_LOCKED || S6_systemQualifications(s,c,x->sequence)!=127U)){
  S6_systemFault(s,c,S6_F_LOCK,x->sequence);return 0;
 }
 if(powered && (s->state!=S6_SYS_ZERO_ALIGN && s->state!=S6_SYS_CURRENT &&
    s->state!=S6_SYS_SPEED && s->state!=S6_SYS_STOPPING)){
  S6_systemFault(s,c,S6_F_LOCK,x->sequence);return 0;
 }
 if(powered && !(s->authority_modes&(s->state==S6_SYS_ZERO_ALIGN?4U:
    (s->state==S6_SYS_STOPPING?s->reserved:(s->state==S6_SYS_SPEED?2U:1U))))){
  S6_systemFault(s,c,S6_F_LOCK,x->sequence);return 0;
 }
 if(powered && s->state!=S6_SYS_ZERO_ALIGN && (!(c->config.evidence&S6_E_ZERO) || c->config.foc.encoder.zero_origin_epoch!=s->session)){
  S6_systemFault(s,c,S6_F_CONFIG,x->sequence);return 0;
 }
 if(s->state==S6_SYS_ZERO_ALIGN){
  if(!powered || !S6_alignStep(&s->alignment,c,x)){
   S6_systemFault(s,c,c->first_fault?c->first_fault:S6_F_LOCK,x->sequence);return 0;
  }
  if(s->alignment.candidate_valid){move(s,S6_SYS_DISABLED);s->power_active=0U;s->authority_modes=0U;}
  return 1;
 }
 if((s->state==S6_SYS_CURRENT || s->state==S6_SYS_SPEED) && !powered){
  S6_systemFault(s,c,S6_F_LOCK,x->sequence);return 0;
 }
 if(s->references_pending && s->state!=S6_SYS_STOPPING){
  float delta=c->config.iq_slew_A_s*0.00005f,d;
  if(!(delta>0.0f && delta<=0.5f)){S6_systemFault(s,c,S6_F_CONFIG,x->sequence);return 0;}
  d=s->requested_id-c->id_ref;c->id_ref+=d>delta?delta:(d< -delta?-delta:d);
  if(c->state!=S6_SPEED_SHADOW){d=s->requested_iq-c->iq_ref;c->iq_ref+=d>delta?delta:(d< -delta?-delta:d);}
  c->speed_ref=s->requested_speed;
 }
 if(s->state==S6_SYS_STOPPING){
  float delta=c->config.iq_slew_A_s*0.00005f;
  if(!(delta>0.0f && delta<=0.5f) || s->stop_frames++>=40000UL){S6_systemFault(s,c,S6_F_CONFIG,x->sequence);return 0;}
  s->stopping_iq+=s->stopping_iq>delta?-delta:(s->stopping_iq< -delta?delta:-s->stopping_iq);
  s->stopping_id+=s->stopping_id>delta?-delta:(s->stopping_id< -delta?delta:-s->stopping_id);
  c->state=S6_CURRENT_SHADOW;c->id_ref=s->stopping_id;c->iq_ref=s->stopping_iq;
 }
 if(!(powered?S6_stepRunning(c,x):S6_step(c,x))){
  if(c->faults)S6_systemFault(s,c,c->first_fault,x->sequence);
  return 0;
 }
 if(s->state==S6_SYS_CALIBRATION && c->state==S6_STOPPED)move(s,S6_SYS_DISABLED);
 if(s->state==S6_SYS_STOPPING && !s->stopping_id && !s->stopping_iq){
  c->state=S6_STOPPED;c->pwm_staged=0U;move(s,S6_SYS_DISABLED);s->authority_modes=0U;s->power_active=0U;
 }
 return 1;
}
