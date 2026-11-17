#include "s6_pwm.h"
#include <string.h>
#include <limits.h>
void S6_pwmDefault(S6_PWMConfig *c){
 memset(c,0,sizeof(*c));c->period=3000U;c->dead=60U;c->minimum_edge=60U;
 c->acquire=19U;c->guard=120U; /* software reserve, not a measured deadline proof */
 /* Existing PERIOD SOCA and physical polarity remain unqualified. */
}
void S6_pwmFault(S6_PWM *p,uint32_t error,uint32_t seq){
 if(!p->first_error){p->first_error=error;p->first_error_sequence=seq;}
 p->locked=1U;p->prepared_valid=0U;
}
int S6_pwmInit(S6_PWM *p,const S6_PWMConfig *c,uint16_t backend){
 memset(p,0,sizeof(*p));p->config=*c;p->backend=backend;p->locked=1U;
 if(c->period!=3000U || c->dead!=60U || c->minimum_edge<c->dead ||
    c->minimum_edge>=1500U || c->guard<1U || c->guard>=6000U ||
    c->acquire!=19U || c->sample_at_zero>1U || backend>S6_PWM_TARGET){
  S6_pwmFault(p,S6_PWM_CONFIG,0U);return 0;
 }
 return 1;
}
int S6_pwmPrepare(S6_PWM *p,const float duty[3],uint32_t seq,uint32_t session,uint32_t release){
 unsigned j;uint16_t edge=3000U;S6_PWMFrame f;
 if(p->first_error)return 0;
 if(!seq || !session || p->pending || p->prepared_valid ||
    (p->commits && (p->committed.sequence==UINT32_MAX || seq!=p->committed.sequence+1UL || session!=p->committed.session))){
  S6_pwmFault(p,S6_PWM_ORDER,seq);return 0;
 }
 memset(&f,0,sizeof(f));f.sequence=seq;f.session=session;f.release=release;f.apply=release-6000UL;
 for(j=0;j<3;j++){
  float x=duty[j];if(!(x>=0.0f && x<=1.0f)){S6_pwmFault(p,S6_PWM_DUTY,seq);return 0;}
  f.compare[j]=(uint16_t)((1.0f-x)*3000.0f+0.5f);
  if(f.compare[j]<p->config.minimum_edge || f.compare[j]>3000U-p->config.minimum_edge){
   S6_pwmFault(p,S6_PWM_DUTY,seq);return 0;
  }
  if(f.compare[j]<edge)edge=f.compare[j];
 }
 /* A high around PERIOD, complementary low-side high around ZERO only if
    reviewed driver polarity agrees. A post-ZERO S/H must finish before edge. */
 f.window_valid=(uint16_t)(p->config.polarity_reviewed && p->config.window_reviewed &&
   p->config.sample_at_zero && (uint32_t)edge>(uint32_t)p->config.dead+p->config.blank+p->config.settle+p->config.acquire);
 f.valid=1U;p->prepared=f;p->prepared_valid=1U;return 1;
}
int S6_pwmCheckCommit(S6_PWM *p,uint32_t now,uint32_t session,uint16_t authorized){
 uint32_t age;
 if(p->first_error)return 0;
 if(!p->prepared_valid || p->pending || session!=p->prepared.session || p->commits==UINT32_MAX){
  S6_pwmFault(p,S6_PWM_ORDER,p->prepared.sequence);return 0;
 }
 age=p->prepared.release-now;
 if(age>=6000UL-p->config.guard){S6_pwmFault(p,S6_PWM_LATE,p->prepared.sequence);return 0;}
 if(p->backend!=S6_PWM_LOCKED && (!authorized || !p->prepared.window_valid)){
  S6_pwmFault(p,!authorized?S6_PWM_AUTH:S6_PWM_WINDOW,p->prepared.sequence);return 0;
 }
 return 1;
}
int S6_pwmCommit(S6_PWM *p,uint32_t now,uint32_t session,uint16_t authorized){
 if(!S6_pwmCheckCommit(p,now,session,authorized))return 0;
 /* One owner in the ISR. Publish only after all three compare values exist. */
 p->committed=p->prepared;p->prepared_valid=0U;p->commits++;p->pending=1U;
 p->locked=(uint16_t)(p->backend==S6_PWM_LOCKED);return 1;
}
int S6_pwmLoaded(S6_PWM *p,uint32_t seq,uint32_t session){
 if(p->first_error)return 0;
 if(!p->pending || seq!=p->committed.sequence || session!=p->committed.session || p->loads==UINT32_MAX){
  S6_pwmFault(p,S6_PWM_ORDER,seq);return 0;
 }
 p->loaded=p->committed;p->pending=0U;p->loads++;return 1;
}
