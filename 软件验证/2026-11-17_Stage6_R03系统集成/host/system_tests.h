/* Included by host/test_stage6.c; pure CPU plant and mock backend only. */
#include "s6_motor.h"
static S6_Motor hostMotor(void){
 S6_Motor m;memset(&m,0,sizeof(m));m.build=S6_BUILD;m.reviewed=1U;m.counts_per_rev=4000U;m.pole_pairs=4U;m.direction=1;
 m.Rs=.59f;m.Ld=m.Lq=.00066f;m.flux=.03f;m.J=.00002f;m.friction=.00001f;
 m.rated_current=1.0f;m.max_speed=100.0f;m.current_bandwidth_Hz=500.0f;m.speed_bandwidth_Hz=10.0f;m.damping=.707f;m.speed_slew_rad_s2=100.0f;m.iq_slew_A_s=10.0f;return m;
}
static void certifiedConfig(S6_Config *c){
 config(c);c->evidence|=S6_E_ZERO|S6_E_PWM|S6_E_PROTECTION|S6_E_REALTIME;
 c->foc.encoder.certificates=31U;c->foc.encoder.zero_origin_epoch=S6_BUILD;c->foc.encoder.zero_origin_raw=1U;
}
static void readySystem(S6_System *s,S6_State *core,S6_Config *c,unsigned backend){
 S6_init(core,c);S6_systemInit(s,(uint16_t)backend);assert(S6_systemEvent(s,core,S6_EVENT_SELFTEST,1U));assert(s->state==S6_SYS_READY);
}
static void authorize(S6_System *s){s->authority_session=s->session;s->authority_until=30000U;s->authority_modes=3U;}
static void pwmTests(void){
 S6_PWM p,keep;S6_PWMConfig c;float d[3]={.2f,.5f,.8f};unsigned j;
 S6_pwmDefault(&c);assert(S6_pwmInit(&p,&c,S6_PWM_LOCKED));
 assert(S6_pwmPrepare(&p,d,2U,S6_BUILD,100000U));assert(!p.prepared.window_valid);
 assert(S6_pwmCommit(&p,99900U,S6_BUILD,0U));assert(p.locked&&p.commits==1&&!p.loads);
 assert(p.committed.compare[0]==2400&&p.committed.compare[1]==1500&&p.committed.compare[2]==600);
 keep=p;assert(!S6_pwmPrepare(&p,d,3U,S6_BUILD,94000U));assert(!memcmp(&keep.committed,&p.committed,sizeof(p.committed)));
 assert(!S6_pwmLoaded(&p,2U,S6_BUILD)); /* fault latch cannot be cleared by ack */
 c.sample_at_zero=c.polarity_reviewed=c.window_reviewed=1U;c.blank=30U;c.settle=120U;
 assert(S6_pwmInit(&p,&c,S6_PWM_SIMULATED));assert(S6_pwmPrepare(&p,d,2U,S6_BUILD,100000U));assert(p.prepared.window_valid);
 assert(!S6_pwmCommit(&p,99900U,S6_BUILD,0U)&&p.first_error==S6_PWM_AUTH&&!p.commits);
 assert(S6_pwmInit(&p,&c,S6_PWM_SIMULATED));assert(S6_pwmPrepare(&p,d,2U,S6_BUILD,100000U));
 assert(!S6_pwmCommit(&p,94120U,S6_BUILD,1U)&&p.first_error==S6_PWM_LATE);
 assert(S6_pwmInit(&p,&c,S6_PWM_SIMULATED));assert(S6_pwmPrepare(&p,d,2U,S6_BUILD,100000U));assert(S6_pwmCommit(&p,99900U,S6_BUILD,1U));
 assert(S6_pwmLoaded(&p,2U,S6_BUILD)&&p.loads==1U);assert(S6_pwmPrepare(&p,d,3U,S6_BUILD,94000U));
 assert(S6_pwmCommit(&p,93900U,S6_BUILD,1U));assert(S6_pwmLoaded(&p,3U,S6_BUILD));
 assert(!S6_pwmPrepare(&p,d,5U,S6_BUILD,88000U)&&p.first_error==S6_PWM_ORDER);
 for(j=0;j<4;j++){
  assert(S6_pwmInit(&p,&c,S6_PWM_SIMULATED));d[0]=j==0?NAN:j==1?INFINITY:j==2?0.0f:1.0f;
  assert(!S6_pwmPrepare(&p,d,1U,S6_BUILD,100000U)&&p.first_error==S6_PWM_DUTY);
 }
 d[0]=.2f;c.sample_at_zero=0U;assert(S6_pwmInit(&p,&c,S6_PWM_SIMULATED));assert(S6_pwmPrepare(&p,d,1U,S6_BUILD,100000U));
 assert(!S6_pwmCommit(&p,99900U,S6_BUILD,1U)&&p.first_error==S6_PWM_WINDOW);
 c.sample_at_zero=1U;c.settle=3000U;assert(S6_pwmInit(&p,&c,S6_PWM_SIMULATED));assert(S6_pwmPrepare(&p,d,1U,S6_BUILD,100000U));
 assert(!S6_pwmCommit(&p,99900U,S6_BUILD,1U));
 c.settle=120U;assert(S6_pwmInit(&p,&c,S6_PWM_SIMULATED));assert(S6_pwmPrepare(&p,d,1U,S6_BUILD,100000U));assert(!S6_pwmCommit(&p,99900U,S6_BUILD+1U,1U));
}
static void lifecycleTests(void){
 S6_Config c;S6_State core;S6_System s;S6_Input x;S6_Motor m=hostMotor();unsigned k;uint32_t fault;
 certifiedConfig(&c);assert(S6_motorApply(&m,&c));assert(c.foc.kp_V_per_A>2.07f&&c.foc.kp_V_per_A<2.08f);
 m.Lq=.0007f;assert(!S6_motorApply(&m,&c));m=hostMotor();m.Rs=NAN;assert(!S6_motorValid(&m));m=hostMotor();m.build--;assert(!S6_motorValid(&m));
 readySystem(&s,&core,&c,S6_PWM_LOCKED);authorize(&s);assert(!S6_systemEvent(&s,&core,S6_EVENT_POWER_PREPARE,1U)&&s.state==S6_SYS_READY);assert(!S6_powerAllowed());
 readySystem(&s,&core,&c,S6_PWM_SIMULATED);assert(!S6_systemEvent(&s,&core,S6_EVENT_POWER_PREPARE,1U));authorize(&s);
 assert(S6_systemEvent(&s,&core,S6_EVENT_POWER_PREPARE,1U));assert(S6_systemEvent(&s,&core,S6_EVENT_CURRENT,1U));
 x=sensor(1);x.tz[0]=x.tz[1]=x.tz[2]=0U;assert(S6_systemStep(&s,&core,&x));
 assert(S6_systemReferences(&s,&core,0,.2f,5.0f));
 for(k=2;k<=128;k++){x=sensor(k);x.position=1U;x.tz[0]=x.tz[1]=x.tz[2]=0U;assert(S6_systemStep(&s,&core,&x));}
 assert(core.foc_success==127U&&core.iq_ref<=.2f);assert(S6_systemSwitch(&s,&core,1U)&&s.state==S6_SYS_SPEED);
 x=sensor(129);x.position=1U;x.tz[0]=x.tz[1]=x.tz[2]=0U;assert(S6_systemStep(&s,&core,&x));
 s.authority_modes=2U; /* speed-only lease also authorizes its own stop */
 assert(!S6_systemReferences(&s,&core,.1f,.1f,5.0f));
 assert(S6_systemEvent(&s,&core,S6_EVENT_STOP,130U));
 for(k=130;k<1000&&s.state==S6_SYS_STOPPING;k++){x=sensor(k);x.position=1U;x.tz[0]=x.tz[1]=x.tz[2]=0U;assert(S6_systemStep(&s,&core,&x));}
 assert(s.state==S6_SYS_DISABLED&&core.state==S6_STOPPED&&!core.pwm_staged&&!core.iq_ref);
 assert(S6_systemEvent(&s,&core,S6_EVENT_SELFTEST,k)&&s.state==S6_SYS_READY&&core.state==S6_LOCKED);
 assert(S6_systemEvent(&s,&core,S6_EVENT_SHADOW,k));
 for(k=0;k<9;k++){
  certifiedConfig(&c);readySystem(&s,&core,&c,S6_PWM_SIMULATED);authorize(&s);assert(S6_systemEvent(&s,&core,S6_EVENT_POWER_PREPARE,1U));assert(S6_systemEvent(&s,&core,S6_EVENT_CURRENT,1U));
  x=sensor(1);x.tz[0]=x.tz[1]=x.tz[2]=0U;
  if(k==0)x.tz[1]=4U;
  if(k==1)x.sd=1U;
  if(k==2)x.trip=1U;
  if(k==3)x.latch=1U;
  if(k==4)s.authority_until=0U;
  if(k==5)core.config.evidence&=(uint16_t)~S6_E_REALTIME;
  if(k==6)x.flags[0]=0U;
  if(k==7)x.overflow[0]=1U;
  if(k==8)x.qep_flags=2U;
  assert(!S6_systemStep(&s,&core,&x)&&s.state==S6_SYS_FAULT);fault=s.first_fault;
  assert(!S6_systemEvent(&s,&core,S6_EVENT_SELFTEST,2U)&&s.first_fault==fault);
 }
 certifiedConfig(&c);readySystem(&s,&core,&c,S6_PWM_LOCKED);s.state=S6_SYS_CURRENT;x=sensor(1);assert(!S6_systemStep(&s,&core,&x));
 certifiedConfig(&c);readySystem(&s,&core,&c,S6_PWM_SIMULATED);s.session++;x=sensor(1);assert(!S6_systemStep(&s,&core,&x));
}
static void plantTests(void){
 S6_Config c;S6_State core;S6_System s;S6_Motor m=hostMotor();S6_PWM pwm;S6_PWMConfig pc;S6_Input x;
 double id=0,iq=0,omega=0,theta=0,phase,ca,sa,ia,ib,ic;unsigned k,j;FILE *f;
 certifiedConfig(&c);assert(S6_motorApply(&m,&c));readySystem(&s,&core,&c,S6_PWM_SIMULATED);authorize(&s);
 assert(S6_systemEvent(&s,&core,S6_EVENT_POWER_PREPARE,1U));assert(S6_systemEvent(&s,&core,S6_EVENT_CURRENT,1U));
 S6_pwmDefault(&pc);pc.sample_at_zero=pc.polarity_reviewed=pc.window_reviewed=1U;pc.blank=30;pc.settle=120;
 assert(S6_pwmInit(&pwm,&pc,S6_PWM_SIMULATED));
 f=fopen("HOST_ONLY_system_plant.csv","w");assert(f);fprintf(f,"sequence,state,id,iq,omega,iq_ref,commits,physical_power\n");
 for(k=1;k<=24000;k++){
  x=sensor(k);x.tz[0]=x.tz[1]=x.tz[2]=0U;
  phase=4*theta;ca=cos(phase);sa=sin(phase);ia=ca*id-sa*iq;
  ib=-.5*ia+.866025403784*(sa*id+ca*iq);ic=-ia-ib;
  {double currents[3];currents[0]=ia;currents[1]=ib;currents[2]=ic;
   for(j=0;j<3;j++){long raw=(long)floor(2048+currents[j]*1000+.5);assert(raw>1&&raw<4094);x.raw[j]=(uint16_t)raw;}}
  x.position=(uint32_t)(1L+(int32_t)floor(theta*4000/6.28318530718+.5));
  if(k==2)assert(S6_systemReferences(&s,&core,0,.08f,10.0f));
  if(k==1000)assert(S6_systemSwitch(&s,&core,1U));
  if(k==15000){assert(S6_systemSwitch(&s,&core,0U));assert(S6_systemReferences(&s,&core,0,.02f,0));}
  if(k==17000)assert(S6_systemEvent(&s,&core,S6_EVENT_STOP,k));
  if(s.state==S6_SYS_DISABLED)break;
  assert(S6_systemStep(&s,&core,&x));
  if(core.pwm_staged){
   if(pwm.pending)assert(S6_pwmLoaded(&pwm,pwm.committed.sequence,S6_BUILD));
   assert(S6_pwmPrepare(&pwm,core.foc.output.duty_preview,k,S6_BUILD,x.release));
   assert(S6_pwmCommit(&pwm,x.release-100U,S6_BUILD,1U));
  }
  if(core.called){
   /* Deliberately simple HOST_ONLY isotropic dq plant; truth supplies sensors
      and plant dynamics, never qualification or an observer angle correction. */
   double vd=core.foc.output.vd,vq=core.foc.output.vq;
   id+=.00005*(vd-m.Rs*id+4*omega*m.Lq*iq)/m.Ld;
   iq+=.00005*(vq-m.Rs*iq-4*omega*(m.Ld*id+m.flux))/m.Lq;
   omega+=.00005*(1.5*4*m.flux*iq-.0003*omega)/m.J;theta+=.00005*omega;
  }
  assert(isfinite(id)&&isfinite(iq)&&isfinite(omega)&&fabs(iq)<1.0&&fabs(omega)<100);
  if(k%20==0)fprintf(f,"%u,%u,%.9g,%.9g,%.9g,%.9g,%lu,0\n",k,s.state,id,iq,omega,core.iq_ref,(unsigned long)pwm.commits);
  if(k==14000)assert(fabs(omega-10)<2.0);
 }
 assert(fclose(f)==0);assert(s.state==S6_SYS_DISABLED&&k>17000&&pwm.commits>16000&&!core.faults);
}
static void alignmentTests(void){
 S6_System s;S6_State core;S6_Config c;S6_Input x;unsigned k;
 certifiedConfig(&c);c.evidence&=(uint16_t)~S6_E_ZERO;c.foc.encoder.certificates&=(uint16_t)~EF_CERT_ZERO;
 readySystem(&s,&core,&c,S6_PWM_SIMULATED);authorize(&s);s.authority_modes|=4U;
 assert(S6_systemEvent(&s,&core,S6_EVENT_POWER_PREPARE,1U));
 assert(!S6_systemEvent(&s,&core,S6_EVENT_CURRENT,1U)); /* no circular ZERO requirement for alignment */
 s.alignment.config.current_A=.1f;s.alignment.config.tolerance_A=.01f;
 s.alignment.config.ramp_frames=128;s.alignment.config.hold_frames=128;s.alignment.config.max_hold_motion=2;s.alignment.config.reviewed=1;
 assert(S6_systemEvent(&s,&core,S6_EVENT_ALIGN,1U));assert(s.state==S6_SYS_ZERO_ALIGN);
 for(k=1;k<=256;k++){
  x=sensor(k);x.position=1U;x.tz[0]=x.tz[1]=x.tz[2]=0U;
  x.raw[0]=2148U;x.raw[1]=x.raw[2]=1998U;
  assert(S6_systemStep(&s,&core,&x));
 }
 assert(s.state==S6_SYS_DISABLED&&s.alignment.candidate_valid&&s.alignment.candidate_raw==1U&&!(core.config.evidence&S6_E_ZERO));
 assert(!s.power_active&&!core.pwm_staged&&!S6_powerAllowed());
}
static void extraSystemNegatives(void){
 S6_Config c;S6_State core;S6_System s;S6_Input x;unsigned k;
 certifiedConfig(&c);readySystem(&s,&core,&c,S6_PWM_LOCKED);assert(S6_systemEvent(&s,&core,S6_EVENT_SHADOW,1U));
 core.config.speed_ki_A_per_rad=NAN;assert(!S6_systemSwitch(&s,&core,1U));
 assert(!S6_systemReferences(&s,&core,2.0f,2.0f,0.0f));
 for(k=0;k<5;k++){
  certifiedConfig(&c);readySystem(&s,&core,&c,S6_PWM_SIMULATED);authorize(&s);assert(S6_systemEvent(&s,&core,S6_EVENT_POWER_PREPARE,1U));assert(S6_systemEvent(&s,&core,S6_EVENT_CURRENT,1U));
  x=sensor(1);x.position=1U;x.tz[0]=x.tz[1]=x.tz[2]=0U;assert(S6_systemStep(&s,&core,&x));
  x=sensor(2);x.position=1U;x.tz[0]=x.tz[1]=x.tz[2]=0U;
  if(k==0)x.raw[3]=4094U;
  if(k==1)x.raw[0]=4095U;
  if(k==2)x.entry-=700U;
  if(k==3)core.config.foc.encoder.zero_origin_epoch--;
  if(k==4)s.authority_modes=2U;
  assert(!S6_systemStep(&s,&core,&x)&&s.state==S6_SYS_FAULT&&!s.power_active);
 }
}
static void systemTests(void){pwmTests();lifecycleTests();plantTests();alignmentTests();extraSystemNegatives();}
