#include "s6_alignment.h"
#include <math.h>
int S6_alignBegin(S6_Alignment *a,S6_State *c){
 if(a->config.reviewed!=1U || !(a->config.current_A>0.0f && a->config.current_A<=c->config.foc.current_limit_A) ||
    !(a->config.tolerance_A>0.0f && a->config.tolerance_A<a->config.current_A) ||
    a->config.ramp_frames<1UL || a->config.ramp_frames>20000UL ||
    a->config.hold_frames<128UL || a->config.hold_frames>20000UL || a->config.max_hold_motion>100UL ||
    !S6_begin(c,S6_CMD_CURRENT))return 0;
 a->primed=a->candidate_valid=0U;a->frames=0UL;
 /* Fixed stator alpha-axis current regulation, not rotor-angle feedback.
    Explicitly revoke any previous magnetic-zero qualification. */
 c->config.evidence&=(uint16_t)~S6_E_ZERO;
 c->config.foc.encoder.certificates&=(uint16_t)~EF_CERT_ZERO;
 c->config.foc.encoder.electrical_offset_rad=0.0f;
 c->id_ref=c->iq_ref=0.0f;return 1;
}
int S6_alignStep(S6_Alignment *a,S6_State *c,const S6_Input *x){
 S6_Input fixed=*x;uint32_t elapsed,delta;
 if(!a->primed){a->origin=x->position;a->primed=1U;}
 fixed.position=a->origin; /* Intentional fixed stator frame, no model truth. */
 if(a->frames>=a->config.ramp_frames+a->config.hold_frames){S6_fault(c,S6_F_CONFIG,x->sequence);return 0;}
 a->frames++;
 c->id_ref=a->frames<a->config.ramp_frames?
  a->config.current_A*((float)a->frames/(float)a->config.ramp_frames):a->config.current_A;
 c->iq_ref=0.0f;
 if(!S6_stepRunning(c,&fixed))return 0;
 if(a->frames==a->config.ramp_frames)a->hold_origin=x->position;
 if(a->frames>a->config.ramp_frames){
  delta=x->position-a->hold_origin;if(delta>0x7FFFFFFFUL)delta=0UL-delta;
  if(delta>a->config.max_hold_motion || (c->called &&
    (fabsf(c->foc.output.id-a->config.current_A)>a->config.tolerance_A || fabsf(c->foc.output.iq)>a->config.tolerance_A))){
   S6_fault(c,S6_F_INPUT,x->sequence);return 0;
  }
 }
 elapsed=a->config.ramp_frames+a->config.hold_frames;
 if(a->frames==elapsed){
  a->candidate_raw=x->position;a->candidate_valid=1U;
  /* Stop and export a candidate, never import an offset or clear a fault. */
  c->state=S6_STOPPED;c->pwm_staged=0U;
 }
 return 1;
}
