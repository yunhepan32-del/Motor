#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "s6_app.h"
#include "s6_port.h"
/* Include the app for fault-injection access to private scheduling fields.
   Platform timestamps below are HOST_ONLY event models, never CPU cycles. */
#include "../src/s6_app.c"
static uint32_t clocktick,irqNo,acks;
static unsigned running,mode,overflowReads,physicalWrites;
uint32_t S6P_time(void){clocktick-=10U;return clocktick;}
void S6P_init(void){}
void S6P_start(void){running=1U;}
void S6P_stop(void){running=0U;}
uint16_t S6P_preflight(void){return mode==7U;}
void S6P_ack(const S6_Input *x){(void)x;acks++;}
void S6P_interruptAck(void){}
void S6P_backgroundLock(void){}
void S6P_registers(uint16_t x[32]){memset(x,0,32U*sizeof(*x));}
void S6P_finishInput(S6_Input *x){(void)x;}
uint16_t S6P_stagePWM(const uint16_t compare[3],uint16_t requested){
 unsigned j;assert(!requested);for(j=0;j<3;j++)assert(compare[j]<=3000U);if(mode==11U)clocktick-=6100U;return 0U;
}
void S6P_overflow(uint16_t x[3]){overflowReads++;x[0]=0U;x[1]=0U;x[2]=(uint16_t)(mode==6U&&overflowReads==2U);}
void S6P_read(S6_Input *x){
 unsigned j;memset(x,0,sizeof(*x));
 x->entry=clocktick;x->release=clocktick+120U;x->sample_tick=x->release;x->qep_tick=x->sample_tick-80U;
 x->position=irqNo/20U;
 for(j=0;j<3;j++){x->flags[j]=1U;x->tz[j]=4U;x->raw[j]=(uint16_t)((mode==9U||mode==11U)?2048U:80U+irqNo%173U);}
 x->raw[3]=(uint16_t)((mode==9U||mode==11U)?2400U:147U);
 if(mode==12U)for(j=0;j<3;j++)x->raw[j]=(uint16_t)(1998U+irqNo%101U);
 if(mode==13U){for(j=0;j<3;j++)x->raw[j]=2048U;if(irqNo==32U)x->raw[0]=4095U;}
 if(mode==1U)x->flags[2]=0U;
 if(mode==2U)x->overflow[1]=1U;
 if(mode==3U)x->tz[0]=0U;
 if(mode==4U)x->qep_flags=4U;
 if(mode==5U)clocktick-=6100U;
}
static void irq(void){
 clocktick=0xFFF00000UL-irqNo*6000UL;irqNo++;overflowReads=0U;
 S6_onISR(clocktick);if(mode==10U)clocktick-=6100U;S6_onExit(S6P_time());
}
static void setup(unsigned m){mode=m;clocktick=0xFFF10000UL;irqNo=acks=0U;physicalWrites=0U;running=0U;S6_appInit();}
static uint32_t u(unsigned off){return H[off]|((uint32_t)H[off+1U]<<16);}
static void dump(const char *name){FILE *f=fopen(name,"w");unsigned j;assert(f);fprintf(f,"1651 1 C000 0 1000\n");for(j=0;j<4096;j++)fprintf(f,"%04X\n",gStage6.words[j]);assert(fclose(f)==0);}
static void config(S6_Config *c){
 unsigned j;S6_profileLoad(c);c->evidence=S6_E_INTERFACE|S6_E_CURRENT|S6_E_BUS|S6_E_ENCODER|S6_E_GAINS|S6_E_OFFSET;
 for(j=0;j<3;j++){c->current_A_per_count[j]=0.001f;c->current_zero_count[j]=2048.0f;}
 c->bus_V_per_count=.01f;c->bus_zero_count=0.0f;
 c->foc.kp_V_per_A=1.0f;c->foc.ki_series=.01f;c->foc.encoder.certificates=23U;
 c->speed_kp_A_per_rad_s=.1f;c->speed_ki_A_per_rad=1.0f;c->speed_iq_limit_A=1.0f;
 c->speed_slew_rad_s2=100.0f;c->iq_slew_A_s=10.0f;
}
static S6_Input sensor(unsigned k){S6_Input x;unsigned j;memset(&x,0,sizeof(x));x.sequence=k;
 x.entry=0xFFF00000UL-k*6000UL;x.release=x.sample_tick=x.entry+120UL;x.qep_tick=x.sample_tick-80UL;x.position=k;
 for(j=0;j<3;j++){x.raw[j]=(uint16_t)(2048U+(k+j)%31U-15U);x.flags[j]=1U;x.tz[j]=4U;}x.raw[3]=2400U;return x;}
static void coreTests(void){
 S6_Config c;S6_State s,previous;SF_State reference;SF_Frame frame;S6_Input x;S6_Speed speed;unsigned j,k;FILE *f;
 config(&c);assert(S6_configValid(&c));S6_init(&s,&c);assert(S6_begin(&s,S6_CMD_CURRENT));assert(S6_setReferences(&s,.2f,.3f,0.0f));
 f=fopen("HOST_ONLY_real_feedback_control.csv","w");assert(f);fprintf(f,"step,theta,id,iq,vd,vq,compareU,compareV,compareW,fault,power\n");
 for(k=1;k<=4096;k++){x=sensor(k);assert(S6_step(&s,&x));assert(!S6_powerAllowed());
  if(k>1){assert(s.called&&s.foc.output.valid&&s.pwm_staged);fprintf(f,"%u,%.9g,%.9g,%.9g,%.9g,%.9g,%u,%u,%u,%lu,0\n",k,s.foc.output.theta_sample,s.foc.output.id,s.foc.output.iq,s.foc.output.vd,s.foc.output.vq,s.compare[0],s.compare[1],s.compare[2],(unsigned long)s.faults);}}
 assert(fclose(f)==0);assert(s.foc_success==4095U);
 /* Independent frame construction against unchanged SF over identical physical inputs. */
 S6_init(&s,&c);assert(S6_begin(&s,S6_CMD_CURRENT));assert(S6_setReferences(&s,.2f,.3f,0));
 x=sensor(1);assert(S6_step(&s,&x));assert(SF_init(&reference,&c.foc,x.position,x.qep_tick,1U,S6_BUILD));
 reference.last_adc_timer=x.sample_tick;
 for(k=2;k<=4096;k++){
  x=sensor(k);memset(&frame,0,sizeof(frame));frame.adc_sequence=k;frame.adc_timer_down=x.sample_tick;
  frame.apply_timer_down=x.sample_tick-6000U;frame.encoder.sequence=k;frame.encoder.raw_position=x.position;
  frame.encoder.timer_down=x.qep_tick;frame.vbus_V=24.0f;frame.id_ref_A=.2f;frame.iq_ref_A=.3f;
  frame.current_calibrated=frame.bus_calibrated=1U;
  for(j=0;j<3;j++)frame.current_A[j]=((float)x.raw[j]-2048.0f)*.001f;
  assert(SF_step(&reference,&frame));assert(S6_step(&s,&x));
  assert(s.foc.encoder.output.interval_ticks==6000U);
  assert(!memcmp(&reference.output,&s.foc.output,sizeof(reference.output)));
 }
 previous=s;x=sensor(4097);x.raw[0]=4095U;assert(!S6_step(&s,&x));assert(s.first_fault==S6_F_INPUT);
 x=sensor(4098);assert(!S6_step(&s,&x));assert(s.first_fault==S6_F_INPUT&&s.foc_success==previous.foc_success);
 assert(!S6_begin(&s,S6_CMD_CURRENT));assert(S6_begin(&s,S6_CMD_STOP)&&s.state==S6_FAULT);
 S6_profileLoad(&c);S6_init(&s,&c);assert(!S6_begin(&s,S6_CMD_BIAS));assert(!S6_begin(&s,S6_CMD_CURRENT));assert(!S6_begin(&s,S6_CMD_SPEED));assert(!S6_setReferences(&s,NAN,0,0));
 c.evidence=S6_E_INTERFACE|S6_E_ZERO_CURRENT;S6_init(&s,&c);assert(S6_begin(&s,S6_CMD_BIAS));
 for(k=1;k<=2048;k++){x=sensor(k);x.raw[0]=(uint16_t)(2047U+k%2U);x.raw[1]=x.raw[2]=2048U;assert(S6_step(&s,&x));}
 assert(s.bias.valid&&s.state==S6_STOPPED&&s.bias.offset[0]==2047.5f);
 S6_init(&s,&c);assert(S6_begin(&s,S6_CMD_BIAS));x=sensor(1);x.raw[0]=100U;assert(!S6_step(&s,&x)&&s.first_fault==S6_F_BIAS);
 S6_init(&s,&c);assert(S6_begin(&s,S6_CMD_BIAS));x=sensor(1);assert(S6_step(&s,&x));x=sensor(2);x.raw[1]=2100U;assert(!S6_step(&s,&x));assert(s.bias.count==1U&&s.bias.sum[0]==2034U&&s.bias.reject_reason==4U&&s.bias.reject_channel==1U&&s.bias.reject_sequence==2U);
 config(&c);c.evidence|=S6_E_ZERO;c.foc.encoder.certificates|=EF_CERT_ZERO;c.foc.encoder.zero_origin_epoch=S6_BUILD;c.foc.encoder.zero_origin_raw=1U;
 S6_init(&s,&c);assert(S6_begin(&s,S6_CMD_SPEED));assert(S6_setReferences(&s,0,.1f,2.0f));
 for(k=1;k<=2048;k++){x=sensor(k);assert(S6_step(&s,&x));assert(fabsf(s.iq_ref)<=1.00001f);}
 assert(s.speed.primed);
 c.foc.encoder.zero_origin_raw=0U;S6_init(&s,&c);assert(S6_begin(&s,S6_CMD_CURRENT));x=sensor(1);assert(!S6_step(&s,&x)); /* stale zero rejected, never rebound */
 config(&c);memset(&speed,0,sizeof(speed));
 for(k=0;k<10000;k++)assert(S6_speedStep(&speed,&c,0,6000U,100.0f,.2f));
 assert(speed.iq<=1.0f&&speed.integral<=1.0f);
 for(k=0;k<10000;k++)assert(S6_speedStep(&speed,&c,0,6000U,-100.0f,speed.iq));
 assert(speed.iq>=-1.0f&&speed.reference<0.0f);
 /* Affine bus offset and signed current gain must be used, never nominal fallback. */
 config(&c);c.bus_zero_count=2048.0f;c.bus_V_per_count=.05f;c.current_A_per_count[1]=-.001f;
 S6_init(&s,&c);x=sensor(1);x.raw[3]=2528U;x.raw[0]=2058U;x.raw[1]=2038U;
 assert(S6_convert(&s,&x));assert(fabsf(s.bus-24.0f)<1e-6f&&fabsf(s.current[1]-.01f)<1e-6f);
 config(&c);S6_init(&s,&c);assert(S6_begin(&s,S6_CMD_CURRENT));x=sensor(1);assert(S6_step(&s,&x));
 x=sensor(2);x.qep_tick=x.sample_tick-1300U;assert(!S6_step(&s,&x)&&s.first_fault==S6_F_ALGORITHM);
 config(&c);c.foc.encoder.direction=-1;S6_init(&s,&c);assert(S6_begin(&s,S6_CMD_CURRENT));
 for(k=1;k<=64;k++){x=sensor(k);assert(S6_step(&s,&x));}assert(s.foc.output.omega_mech<0);
 assert(!S6_setReferences(&s,3.0f,0,0));assert(!S6_setReferences(&s,0,0,INFINITY));
 speed.count_sum=INT32_MAX;assert(!S6_speedStep(&speed,&c,1,6000U,0,0));
 for(j=0;j<3;j++){config(&c);c.current_A_per_count[j]=NAN;assert(!S6_configValid(&c));}
 config(&c);c.evidence|=S6_E_PWM|S6_E_PROTECTION|S6_E_REALTIME;S6_init(&s,&c);assert(!S6_powerAllowed());
}
static void statisticTests(void){
 S6_Config c;S6_Stats st,frozen;S6_Input x;unsigned k,j;uint32_t sums[4]={0,0,0,0};
 config(&c);S6_statsInit(&st,&c);
 for(k=1;k<=4096;k++){
  x=sensor(k);for(j=0;j<4;j++){x.raw[j]=(uint16_t)((k*13U+j*37U)%4096U);sums[j]+=x.raw[j];}
  assert(S6_statsStep(&st,&c,&x));
 }
 S6_statsFinish(&st);assert(st.data.calibration.data.complete&&!st.data.calibration.data.approved);
 assert(st.data.calibration.data.reject&S6_CAL_SPAN);
 for(j=0;j<4;j++){assert(st.data.total[j].count==4096&&st.data.total[j].sum==sums[j]);assert(st.data.total[j].min==0&&st.data.total[j].max==4095);assert(st.data.total[j].saturated==4U);}
 frozen=st;x=sensor(4097);assert(!S6_statsStep(&st,&c,&x));S6_statsFinish(&st);assert(!memcmp(&st,&frozen,sizeof(st)));
 S6_statsInit(&st,&c);for(k=1;k<=512;k++){x=sensor(k);for(j=0;j<4;j++)x.raw[j]=0U;assert(S6_statsStep(&st,&c,&x));}
 assert(st.data.segment[0][0].square_center==2147483648UL);
 S6_statsInit(&st,&c);st.data.total[0].sum=UINT32_MAX;x=sensor(1);assert(!S6_statsStep(&st,&c,&x));assert(st.data.calibration.data.reject&S6_STAT_OVERFLOW);
 S6_statsInit(&st,&c);st.data.segment[0][0].square_center=UINT32_MAX;x=sensor(1);assert(!S6_statsStep(&st,&c,&x));
 S6_statsInit(&st,&c);x=sensor(1);x.raw[2]=4096U;assert(!S6_statsStep(&st,&c,&x));assert(!st.data.total[0].count&&!st.data.total[1].count);
 c.evidence|=S6_E_ZERO_CURRENT;S6_statsInit(&st,&c);
 for(k=1;k<=4096;k++){x=sensor(k);for(j=0;j<3;j++)x.raw[j]=2048U;assert(S6_statsStep(&st,&c,&x));}
 S6_statsFinish(&st);assert(st.data.calibration.data.reject==0&&st.data.calibration.data.complete&&st.data.calibration.data.eligible&&!st.data.calibration.data.approved);
 assert(st.data.calibration.data.candidate[0]==2048.0f);
}
#include "system_tests.h"
int main(void){
 unsigned j,m;uint32_t first;
 coreTests();statisticTests();systemTests();setup(0);dump("HOST_ONLY_init.dat");gStage6Command=1U;S6_poll();assert(running);
 /* No foreground calls at all during 4096 hardware-trigger model steps. */
 for(j=0;j<4096;j++)irq();
 S6_poll();assert(H[6]==2U&&u(16)==4096U&&u(18)==0U&&H[11]==16U&&u(22)==0U);
 assert(!physicalWrites&&!running);dump("HOST_ONLY_raw_batch.dat");gStage6Command=1U;S6_poll();assert(rejected==1U&&irqNo==4096U);
 for(m=1;m<=7;m++){setup(m);gStage6Command=1U;S6_poll();if(running)irq();S6_poll();assert(H[6]==3U&&u(22));first=gS6.first_fault;gStage6Command=1U;S6_poll();assert(gS6.first_fault==first);{char n[50];snprintf(n,sizeof(n),"HOST_ONLY_fault_%u.dat",m);dump(n);}}
 setup(0);gStage6Command=2U;S6_poll();assert(!running&&!accepted&&H[6]==4U&&rejected==1U);dump("HOST_ONLY_bias_blocked.dat");
 setup(0);gStage6Command=1U;S6_poll();clocktick-=120000010U;S6_poll();assert(gS6.first_fault==S6_F_WAIT);dump("HOST_ONLY_timeout.dat");
 setup(0);gStage6Command=1U;S6_poll();irq();irqNo++;irq();S6_poll();assert(gS6.first_fault==S6_F_INTERVAL);dump("HOST_ONLY_missing_irq.dat");
 setup(10);gStage6Command=1U;S6_poll();irq();S6_poll();assert(gS6.first_fault==S6_F_BUDGET);dump("HOST_ONLY_exit_timeout.dat");
 setup(9);{S6_Config c;config(&c);S6_init(&gS6,&c);}gStage6Command=1U;S6_poll();for(j=0;j<4096;j++)irq();
 S6_poll();assert(gS6.foc_success==4095U&&u(68)==4095U&&H[6]==2U);dump("HOST_ONLY_calibrated_control.dat");
 setup(9);{S6_Config c;config(&c);S6_init(&gS6,&c);}gStage6Command=3U;S6_poll();
 for(j=0;j<4352;j++)irq();
 assert(running&&sequence==4352U&&gStage6.data.snapshot[15].data.sequence==4096U);
 S6_poll();assert(running);gStage6Command=5U;S6_poll();while(running&&sequence<7000U)irq();S6_poll();assert(H[6]==2U&&u(16)>=4352U&&u(20)==u(16)-1U);
 dump("HOST_ONLY_continuous_control_stop.dat");
 setup(11);{S6_Config c;config(&c);S6_init(&gS6,&c);}gStage6Command=1U;S6_poll();irq();irq();S6_poll();assert(gS6.first_fault==S6_F_DEADLINE);dump("HOST_ONLY_stage_deadline.dat");
 setup(0);gStage6Command=1U;S6_poll();exitPending=1U;irq();S6_poll();assert(gS6.first_fault==S6_F_COUNTER&&!running&&H[10]);dump("HOST_ONLY_owner_collision.dat");
 setup(0);gStage6Command=1U;S6_poll();sequence=0xFFFFFFFFUL;irq();S6_poll();assert(gS6.first_fault==S6_F_COUNTER&&!running&&H[10]);dump("HOST_ONLY_counter_overflow.dat");
 setup(0);gStage6Command=1U;S6_poll();irq();gStage6Command=5U;S6_poll();assert(!running&&gS6.state==S6_STOPPED);dump("HOST_ONLY_operator_stop.dat");
 setup(0);gStage6Command=6U;S6_poll();for(j=0;j<4096;j++)irq();S6_poll();
 assert(H[6]==2U&&gStage6.data.stats.data.total[0].count==4096U&&!gStage6.data.stats.data.calibration.data.approved);
 assert(gStage6.data.stats.data.calibration.data.reject&S6_CAL_INTERFACE);dump("HOST_ONLY_feedback_low.dat");
 {S6_Stats keep=gStage6.data.stats;gStage6Command=6U;S6_poll();assert(!memcmp(&keep,(const void*)&gStage6.data.stats,sizeof(keep)));}
 setup(9);gStage6Command=6U;S6_poll();for(j=0;j<4096;j++)irq();S6_poll();assert(H[6]==2U);dump("HOST_ONLY_feedback_stable_unqualified.dat");
 setup(9);{S6_Config c;config(&c);c.evidence|=S6_E_ZERO_CURRENT;S6_init(&gS6,&c);S6_statsInit(&gStage6.data.stats,&c);}gStage6Command=6U;S6_poll();for(j=0;j<4096;j++)irq();S6_poll();assert(!gStage6.data.stats.data.calibration.data.reject&&!gS6.called);dump("HOST_ONLY_feedback_eligible.dat");
 setup(12);gStage6Command=6U;S6_poll();for(j=0;j<4096;j++)irq();S6_poll();assert(H[6]==2U&&(gStage6.data.stats.data.calibration.data.reject&S6_CAL_SPAN));dump("HOST_ONLY_feedback_noisy.dat");
 setup(13);gStage6Command=6U;S6_poll();for(j=0;j<4096;j++)irq();S6_poll();assert(H[6]==2U&&gStage6.data.stats.data.total[0].saturated==1U);dump("HOST_ONLY_feedback_saturated.dat");
 for(m=1;m<=7;m++){setup(m);gStage6Command=6U;S6_poll();if(running)irq();S6_poll();assert(H[6]==3U&&u(22));{char name[64];snprintf(name,sizeof(name),"HOST_ONLY_feedback_fault_%u.dat",m);dump(name);}}
 puts("HOST_ONLY PASS: real-feedback integration, raw batch without foreground handshake, calibration blockers, PI/reversal/slew/antiwindup, stale zero, ADC/lock/QEP/period/overflow/deadline/timeout/first-fault negatives; permanent power lock. Mock time is NOT target performance.");return 0;
}
