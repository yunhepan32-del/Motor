#include "s6_control.h"
#include <string.h>
#include <math.h>
void S6_biasInit(S6_Bias *b){unsigned j;memset(b,0,sizeof(*b));for(j=0;j<3;j++)b->min[j]=4095U;}
static int biasRejected(S6_Bias *b,uint16_t reason,unsigned channel,uint16_t raw){
 if(!b->reject_reason){b->reject_reason=reason;b->reject_channel=(uint16_t)channel;b->reject_raw=raw;}
 return -1;
}
int S6_biasStep(S6_Bias *b,const S6_Config *c,const uint16_t raw[4]){
 unsigned j;
 if((c->evidence&(S6_E_INTERFACE|S6_E_ZERO_CURRENT))!=(S6_E_INTERFACE|S6_E_ZERO_CURRENT) ||
    b->valid || b->count>=c->bias_samples)return biasRejected(b,1U,0U,raw[0]);
 /* Validate the whole frame before any sum/min/max mutation. */
 for(j=0;j<3;j++){
  uint16_t lo=raw[j]<b->min[j]?raw[j]:b->min[j];
  uint16_t hi=raw[j]>b->max[j]?raw[j]:b->max[j];
  if(raw[j]<c->bias_min || raw[j]>c->bias_max)return biasRejected(b,2U,j,raw[j]);
  if(hi-lo>c->bias_span_max)return biasRejected(b,4U,j,raw[j]);
 }
 for(j=0;j<3;j++){
  if(raw[j]<b->min[j])b->min[j]=raw[j];
  if(raw[j]>b->max[j])b->max[j]=raw[j];
  b->sum[j]+=raw[j]; /* <=2048*4095=8386560: no u32 overflow. */
 }
 b->count++;
 if(b->count==c->bias_samples){
  for(j=0;j<3;j++)b->offset[j]=(float)b->sum[j]/(float)b->count;
  b->valid=1U;return 1;
 }
 return 0;
}
int S6_convert(S6_State *s,const S6_Input *x){
 unsigned j;const S6_Config *c=&s->config;
 if(!s->bias.valid || (c->evidence&(S6_E_INTERFACE|S6_E_CURRENT|S6_E_BUS))!=
    (S6_E_INTERFACE|S6_E_CURRENT|S6_E_BUS))return 0;
 for(j=0;j<4;j++)if(x->raw[j]<=1U || x->raw[j]>=4094U)return 0;
 for(j=0;j<3;j++){
  s->current[j]=((float)x->raw[j]-s->bias.offset[j])*c->current_A_per_count[j];
  if(!S6_finite(s->current[j]) || fabsf(s->current[j])>c->foc.current_limit_A)return 0;
 }
 s->bus=((float)x->raw[3]-c->bus_zero_count)*c->bus_V_per_count;
 if(!S6_finite(s->bus) || s->bus<c->foc.min_bus_V || s->bus>c->foc.max_bus_V)return 0;
 return 1;
}
