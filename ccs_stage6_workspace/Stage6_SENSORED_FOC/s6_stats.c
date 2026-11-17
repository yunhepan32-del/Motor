#include "s6_stats.h"
#include <limits.h>
#include <string.h>
static void put(volatile uint16_t *h,unsigned i,uint32_t v){h[i]=(uint16_t)v;h[i+1U]=(uint16_t)(v>>16);}
static uint32_t get(volatile uint16_t *h,unsigned i){return (uint32_t)h[i]|((uint32_t)h[i+1U]<<16);}
static void rejected(volatile S6_CalCandidate *b,uint32_t reason,uint32_t seq,unsigned ch,uint16_t raw){
 if(!b->data.first_sequence){b->data.first_sequence=seq;b->data.first_channel=(uint16_t)ch;b->data.first_value=raw;}
 b->data.reject|=reason;
}
void S6_statsInit(volatile S6_Stats *s,const S6_Config *c){
 unsigned i,j;memset((void*)s,0,sizeof(*s));s->data.header[0]=2U;s->data.header[1]=512U;
 s->data.header[2]=8U;s->data.header[3]=128U;s->data.header[4]=32U;
 for(j=0;j<4;j++){
  s->data.total[j].min=4095U;
  for(i=0;i<8;i++)s->data.segment[i][j].min=4095U;
 }
 for(j=0;j<3;j++)s->data.calibration.data.min[j]=4095U;
 s->data.calibration.data.eligible=(uint16_t)((c->evidence&(S6_E_INTERFACE|S6_E_ZERO_CURRENT))==(S6_E_INTERFACE|S6_E_ZERO_CURRENT));
 if(!(c->evidence&S6_E_INTERFACE))s->data.calibration.data.reject|=S6_CAL_INTERFACE;
 if(!(c->evidence&S6_E_ZERO_CURRENT))s->data.calibration.data.reject|=S6_CAL_ZERO;
}
static int cellValid(volatile S6_StatCell *c,uint16_t raw,uint16_t jump,uint16_t square){
 int32_t delta=(int32_t)raw-2048L;uint32_t sq=(uint32_t)(delta*delta);
 return !(c->count==UINT32_MAX || UINT32_MAX-c->sum<raw ||
    (square && UINT32_MAX-c->square_center<sq) ||
    (jump && c->jumps==65535U) || ((raw<=1U || raw>=4094U) && c->saturated==65535U));
}
static void cellAdd(volatile S6_StatCell *c,uint16_t raw,uint16_t jump,uint16_t square){
 int32_t delta=(int32_t)raw-2048L;
 c->count++;c->sum+=raw;if(square)c->square_center+=(uint32_t)(delta*delta);
 if(raw<c->min)c->min=raw;
 if(raw>c->max)c->max=raw;
 if(raw<=1U || raw>=4094U)c->saturated++;
 if(jump)c->jumps++;
}
int S6_statsStep(volatile S6_Stats *s,const S6_Config *c,const S6_Input *x){
 uint32_t n=get(s->data.header,8U);unsigned seg,j,slot,k;uint16_t raw,jump,reason,previous;
 volatile S6_CalCandidate *b=&s->data.calibration;
 if(s->data.header[5] || n>=4096UL || x->sequence!=n+1UL)return 0;
 seg=(unsigned)(n>>9U);slot=n<64UL?(unsigned)n:64U+(unsigned)(n&63UL);
 /* Preflight all channels: a rejected frame cannot partially mutate coverage. */
 for(j=0;j<4;j++){
  raw=x->raw[j];if(raw>4095U)return 0;
  previous=s->data.header[24U+j];jump=(uint16_t)(n && (uint16_t)(raw>previous?raw-previous:previous-raw)>32U);
  if(!cellValid(&s->data.total[j],raw,jump,0U) || !cellValid(&s->data.segment[seg][j],raw,jump,1U) ||
     (n<2048UL && j<3U && UINT32_MAX-b->data.sum[j]<raw)){
   rejected(b,S6_STAT_OVERFLOW,x->sequence,j,raw);return 0;
  }
 }
 for(j=0;j<4;j++){
  raw=x->raw[j];
  previous=s->data.header[24U+j];jump=(uint16_t)(n && (uint16_t)(raw>previous?raw-previous:previous-raw)>32U);
  reason=(uint16_t)((raw<=1U || raw>=4094U)?1U:0U);if(jump)reason|=2U;
  if(reason && !s->data.anomaly[0]){
   s->data.anomaly[0]=1U;s->data.anomaly[1]=reason;put(s->data.anomaly,2U,x->sequence);
   s->data.anomaly[4]=(uint16_t)j;s->data.anomaly[5]=raw;s->data.anomaly[6]=previous;
   for(k=0U;k<4U;k++)s->data.anomaly[8U+k]=x->raw[k];
  }
  cellAdd(&s->data.total[j],raw,jump,0U);cellAdd(&s->data.segment[seg][j],raw,jump,1U);
  s->data.header[24U+j]=raw;s->data.raw_window[slot][j]=raw;
  if(n<2048UL && j<3U){
   b->data.sum[j]+=raw;if(raw<b->data.min[j])b->data.min[j]=raw;if(raw>b->data.max[j])b->data.max[j]=raw;
   if(raw<c->bias_min || raw>c->bias_max)rejected(b,S6_CAL_RANGE,x->sequence,j,raw);
   if(b->data.max[j]-b->data.min[j]>c->bias_span_max)rejected(b,S6_CAL_SPAN,x->sequence,j,raw);
   if(raw<=1U || raw>=4094U)rejected(b,S6_CAL_SAT,x->sequence,j,raw);
  }
 }
 if(n<2048UL)b->data.count++;
 /* Sticky QFLG records observations, not event multiplicities. */
 s->data.header[28]|=x->qep_flags;s->data.header[29]|=x->qep_status;
 if(n && ((x->qep_status^s->data.header[30])&0x20U))s->data.header[31]++;
 s->data.header[32]|=(uint16_t)(x->qep_flags&~s->data.header[33]);s->data.header[33]=x->qep_flags;
 s->data.header[30]=x->qep_status;
 put(s->data.header,8U,n+1UL);s->data.header[10]=(uint16_t)((n+1UL)&63UL);
 return 1;
}
void S6_statsFinish(volatile S6_Stats *s){
 unsigned j;volatile S6_CalCandidate *b=&s->data.calibration;
 if(s->data.header[5])return;
 if(b->data.count==2048UL){b->data.complete=1U;for(j=0;j<3;j++)b->data.candidate[j]=(float)b->data.sum[j]/2048.0f;}
 else b->data.reject|=S6_CAL_INCOMPLETE;
 /* Candidate never imported into operational bias or hardware profile. */
 b->data.approved=0U;s->data.header[5]=1U;
}
