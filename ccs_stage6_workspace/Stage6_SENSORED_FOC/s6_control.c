#include "s6_control.h"
#include <math.h>
#include <string.h>
void S6_fault(S6_State *s,uint32_t bits,uint32_t sequence){
 if(!s->first_fault){s->first_fault=bits;s->first_sequence=sequence;}
 s->faults|=bits;s->state=S6_FAULT;s->pwm_staged=0U;
}
uint16_t S6_blockers(const S6_State *s){
 uint16_t b=0U,e=s->config.evidence;
 if(!(e&S6_E_INTERFACE))b|=S6_B_INTERFACE;
 if(!(e&S6_E_CURRENT))b|=S6_B_CURRENT;
 if(!s->bias.valid)b|=S6_B_OFFSET;
 if(!(e&S6_E_BUS))b|=S6_B_BUS;
 if(!(e&S6_E_ENCODER))b|=S6_B_ENCODER;
 if(!(e&S6_E_ZERO))b|=S6_B_ZERO;
 if(!(e&S6_E_GAINS))b|=S6_B_GAINS;
 if(!(e&S6_E_PROTECTION))b|=S6_B_PROTECTION;
 if(!(e&S6_E_PWM))b|=S6_B_PWM;
 if(!(e&S6_E_REALTIME))b|=S6_B_REALTIME;
 return b;
}
void S6_init(S6_State *s,const S6_Config *c){
 unsigned j;
 memset(s,0,sizeof(*s));s->config=*c;S6_biasInit(&s->bias);s->state=S6_LOCKED;
 s->first_sequence=0xFFFFFFFFUL;
 if(!S6_configValid(c))S6_fault(s,S6_F_CONFIG,0U);
 if(!s->faults && c->evidence&S6_E_OFFSET){
  for(j=0;j<3;j++)s->bias.offset[j]=c->current_zero_count[j];
  s->bias.valid=1U;
 }
 s->blockers=S6_blockers(s);
}
int S6_begin(S6_State *s,uint16_t command){
 uint16_t compute=S6_B_INTERFACE|S6_B_CURRENT|S6_B_OFFSET|S6_B_BUS|S6_B_ENCODER|S6_B_GAINS;
 if(command==S6_CMD_STOP){if(s->state!=S6_FAULT)s->state=S6_STOPPED;return 1;}
 if(s->faults || s->state!=S6_LOCKED)return 0;
 s->blockers=S6_blockers(s);
 if(command==S6_CMD_FEEDBACK)s->state=S6_RAW;
 else if(command==S6_CMD_DIAG)s->state=(s->blockers&compute)?S6_RAW:S6_CURRENT_SHADOW;
 else if(command==S6_CMD_BIAS){
  if((s->config.evidence&(S6_E_INTERFACE|S6_E_ZERO_CURRENT))!=(S6_E_INTERFACE|S6_E_ZERO_CURRENT))return 0;
  s->state=S6_CALIBRATING;
 }else if(command==S6_CMD_CURRENT || command==S6_CMD_SPEED){
  if(s->blockers&compute)return 0;
  if(command==S6_CMD_SPEED && (!(s->config.evidence&S6_E_ZERO) ||
     !(s->config.speed_iq_limit_A>0.0f)))return 0;
  s->state=command==S6_CMD_CURRENT?S6_CURRENT_SHADOW:S6_SPEED_SHADOW;
 }else return 0;
 s->mode=command;return 1;
}
int S6_setReferences(S6_State *s,float id,float iq,float speed){
 if(s->faults || !S6_finite(id) || !S6_finite(iq) || !S6_finite(speed) ||
    fabsf(id)>s->config.foc.current_limit_A || fabsf(iq)>s->config.foc.current_limit_A ||
    fabsf(speed)>s->config.maximum_speed_rad_s)return 0;
 s->id_ref=id;s->iq_ref=iq;s->speed_ref=speed;return 1;
}
int S6_preparePWM(S6_State *s){
 unsigned j;if(!s->foc.output.valid || !s->conversion_valid || s->faults)return 0;
 for(j=0;j<3;j++){
  float duty=s->foc.output.duty_preview[j];
  if(!S6_finite(duty) || duty<0.0f || duty>1.0f)return 0;
  /* Existing AQ: A high on up-CMPA, low on down-CMPA => 1-CMPA/PRD.
     This is RAM staging only; board polarity still requires physical evidence. */
  s->compare[j]=(uint16_t)((1.0f-duty)*3000.0f+0.5f);
 }
 s->pwm_staged=1U;return 1;
}
static int stepDomain(S6_State *s,const S6_Input *x,uint16_t running){
 unsigned j;SF_Frame frame;uint32_t dt;uint16_t compute;
 s->called=0U;s->conversion_valid=0U;s->pwm_staged=0U;
 if(s->state<S6_RAW || s->state>S6_SPEED_SHADOW || s->faults)return 0;
 if(s->processed==0xFFFFFFFFUL){S6_fault(s,S6_F_COUNTER,x->sequence);return 0;}
 s->processed++;
 for(j=0;j<3;j++){
  if((x->flags[j]&1U)==0U)S6_fault(s,S6_F_ADC_MISSING,x->sequence);
  if((x->overflow[j]|x->pre_overflow[j])&1U)S6_fault(s,S6_F_ADC_OVERFLOW,x->sequence);
  if(running?(x->tz[j]&0x7FU)!=0U:!(x->tz[j]&4U))S6_fault(s,S6_F_LOCK,x->sequence);
 }
 if(running){
  if(!(s->config.evidence&S6_E_PROTECTION) || x->trip || x->latch || x->sd!=s->config.sd_healthy_level)
   S6_fault(s,S6_F_LOCK,x->sequence);
 }else if(x->gpio_mask)S6_fault(s,S6_F_LOCK,x->sequence);
 if((x->qep_flags&6U) || (x->qep_status&1U))S6_fault(s,S6_F_QEP,x->sequence);
 if(s->primed && (s->previous_entry-x->entry<5400UL || s->previous_entry-x->entry>6600UL ||
    (uint32_t)(x->sequence-s->previous_sequence)!=1UL))S6_fault(s,S6_F_INTERVAL,x->sequence);
 s->previous_entry=x->entry;s->previous_sequence=x->sequence;
 if(s->faults)return 0;
 if(s->state==S6_CALIBRATING){
  int b=S6_biasStep(&s->bias,&s->config,x->raw);
  if(b<0){s->bias.reject_sequence=x->sequence;S6_fault(s,S6_F_BIAS,x->sequence);}
  else if(b>0)s->state=S6_STOPPED;
  s->primed=1U;s->blockers=S6_blockers(s);return !s->faults;
 }
 /* Disconnected/unqualified ADC remains raw counts. No nominal midpoint/gain
    substitution and no conversion of floating pins to physical amperes. */
 compute=(uint16_t)(s->state==S6_CURRENT_SHADOW || s->state==S6_SPEED_SHADOW);
 if(!compute){s->primed=1U;return 1;}
 if(!S6_convert(s,x)){S6_fault(s,S6_F_INPUT,x->sequence);return 0;}
 s->conversion_valid=1U;
 if(!s->primed){
  SF_Config c=s->config.foc;
  uint32_t skew=x->sample_tick-x->qep_tick;
  if(skew>c.max_skew_ticks && (uint32_t)(0UL-skew)>c.max_skew_ticks){
   S6_fault(s,S6_F_INPUT,x->sequence);return 0;
  }
  /* A certified zero stays tied to its recorded origin/session. Never rebind
     a stale magnetic zero to the first observed count. EF_init rejects it. */
  if(!SF_init(&s->foc,&c,x->position,x->sample_tick,x->sequence,S6_BUILD))S6_fault(s,S6_F_CONFIG,x->sequence);
  else s->foc.encoder.previous_timer=x->qep_tick;
  /* SF_init has one common timestamp argument. The port has separate ADC
     and QEP read times: preserve ADC origin, seed EF with its actual time. */
  s->primed=1U;return !s->faults;
 }
 memset(&frame,0,sizeof(frame));frame.adc_sequence=x->sequence;frame.adc_timer_down=x->sample_tick;
 frame.apply_timer_down=x->sample_tick-S6_PERIOD; /* next period, not next zero */
 frame.encoder.sequence=x->sequence;frame.encoder.raw_position=x->position;
 frame.encoder.timer_down=x->qep_tick;frame.encoder.qep_status=x->qep_status;frame.encoder.qep_flags=x->qep_flags;
 for(j=0;j<3;j++)frame.current_A[j]=s->current[j];
 frame.vbus_V=s->bus;frame.id_ref_A=s->id_ref;frame.iq_ref_A=s->iq_ref;
 frame.current_calibrated=1U;frame.bus_calibrated=1U;
 s->called=1U;s->foc_calls++;
 if(!SF_step(&s->foc,&frame)){S6_fault(s,S6_F_ALGORITHM,x->sequence);return 0;}
 s->foc_success++;
 if(fabsf(s->foc.output.omega_mech)>s->config.maximum_speed_rad_s){S6_fault(s,S6_F_INPUT,x->sequence);return 0;}
 if(s->state==S6_SPEED_SHADOW){
  dt=s->foc.encoder.output.interval_ticks;
  if(!S6_speedStep(&s->speed,&s->config,s->foc.encoder.output.delta_counts,dt,s->speed_ref,s->iq_ref)){
   S6_fault(s,S6_F_CONFIG,x->sequence);return 0;
  }
  s->iq_ref=s->speed.iq; /* explicit one-current-tick outer-loop interface delay */
 }
 if(!S6_preparePWM(s)){S6_fault(s,S6_F_INPUT,x->sequence);return 0;}
 return 1;
}
int S6_step(S6_State *s,const S6_Input *x){return stepDomain(s,x,0U);}
int S6_stepRunning(S6_State *s,const S6_Input *x){return stepDomain(s,x,1U);}
