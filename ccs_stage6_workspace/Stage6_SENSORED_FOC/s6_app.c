#include "s6_app.h"
#include "s6_port.h"
#include <string.h>
#pragma DATA_SECTION(gStage6,"encoder_data")
volatile S6_Export gStage6;
#pragma DATA_SECTION(gS6,"foc_data")
S6_State gS6;
#pragma DATA_SECTION(gSystem,"foc_data")
S6_System gSystem;
#pragma DATA_SECTION(gPWM,"foc_data")
S6_PWM gPWM;
#pragma DATA_SECTION(input,"foc_data")
static S6_Input input;
volatile uint16_t gStage6Command;
static uint16_t accepted,recordCount,rejected;
static volatile uint16_t active,done,exitPending,stopRequested;
static volatile uint32_t sequence,lastEntry;
static uint32_t startTick,maxC,maxWrapper,maxUpdate,updateCount;
static uint32_t minInterval,maxInterval,intervalCount,overflowCount,deadlineCount;
static uint32_t algorithmTicks,cTicks,stagedAt,stepCount,maxStep;
static uint16_t postOVF[3];
#define H gStage6.data.header
static void put32(uint16_t p,uint32_t v){H[p]=(uint16_t)v;H[p+1U]=(uint16_t)(v>>16);}
#pragma FUNC_CANNOT_INLINE(S6_readyStop)
void S6_readyStop(void){__asm(" NOP");}
#pragma FUNC_CANNOT_INLINE(S6_doneStop)
void S6_doneStop(void){__asm(" NOP");}
static void snapshot(volatile S6_Record *r){
 unsigned j;memset((void*)r,0,sizeof(*r));
 r->data.sequence=input.sequence;r->data.entry=input.entry;r->data.sample=input.sample_tick;
 r->data.qep=input.qep_tick;r->data.release=input.release;r->data.position=input.position;
 r->data.algorithm_ticks=algorithmTicks;r->data.c_ticks=cTicks;
 for(j=0;j<4;j++)r->data.raw[j]=input.raw[j];
 for(j=0;j<3;j++){
  r->data.flags[j]=input.flags[j];r->data.overflow[j]=input.overflow[j];
  r->data.pre_overflow[j]=input.pre_overflow[j];r->data.post_overflow[j]=postOVF[j];
  r->data.tz[j]=input.tz[j];r->data.compare[j]=gS6.compare[j];
 }
 r->data.qep_status=input.qep_status;r->data.qep_flags=input.qep_flags;
 r->data.gpio=input.gpio_mask;r->data.sd=input.sd;r->data.trip=input.trip;r->data.latch=input.latch;
 r->data.state=gS6.state;r->data.blockers=S6_blockers(&gS6);r->data.called=gS6.called;
 r->data.conversion_valid=gS6.conversion_valid;r->data.pwm_staged=gS6.pwm_staged;
 if(gS6.conversion_valid){for(j=0;j<3;j++)r->data.current[j]=gS6.current[j];r->data.bus=gS6.bus;}
 r->data.iq_reference=gS6.iq_ref;r->data.speed_reference=gS6.speed_ref;
 if(gS6.called)r->data.output=gS6.foc.output;
}
static void publish(void){
 unsigned j;
 if(gS6.faults && gSystem.state!=S6_SYS_FAULT)S6_systemFault(&gSystem,&gS6,gS6.first_fault,gS6.first_sequence);
 H[6]=gS6.faults?3U:(gS6.state==S6_LOCKED?4U:2U);H[7]=gS6.state;H[8]=gS6.mode;
 H[9]=accepted;H[10]=1U;H[11]=recordCount;H[12]=rejected;H[13]=S6_blockers(&gS6);
 H[14]=0U;H[15]=1U;
 put32(16U,gS6.processed);put32(18U,gS6.foc_calls);put32(20U,gS6.foc_success);
 put32(22U,gS6.faults);put32(24U,gS6.first_sequence);put32(26U,maxC);put32(28U,maxWrapper);
 put32(30U,maxUpdate);put32(32U,minInterval);put32(34U,maxInterval);put32(36U,intervalCount);
 put32(38U,overflowCount);put32(40U,deadlineCount);put32(42U,startTick);put32(44U,S6P_time());
 H[46]=gSystem.state;H[47]=S6_systemQualifications(&gSystem,&gS6,sequence);
 H[55]=gSystem.denied;put32(58U,gPWM.commits);
 H[67]=gPWM.backend;put32(94U,gPWM.first_error);
 H[48]=gS6.bias.valid;H[49]=gS6.bias.count;H[50]=gS6.config.evidence;
 H[52]=gS6.bias.reject_reason;H[53]=gS6.bias.reject_channel;H[54]=gS6.bias.reject_raw;put32(56U,gS6.bias.reject_sequence);
 S6_statsFinish(&gStage6.data.stats);
 put32(68U,updateCount);put32(70U,gS6.first_fault);
 for(j=0U;j<3U;j++){
  union {float f;uint32_t u;} bits;bits.f=gS6.bias.offset[j];put32((uint16_t)(72U+2U*j),bits.u);
  H[78U+j]=gS6.bias.min[j];H[81U+j]=gS6.bias.max[j];put32((uint16_t)(84U+2U*j),gS6.bias.sum[j]);
 }
 put32(90U,stepCount);put32(92U,maxStep);
 if(gS6.processed)snapshot(&gStage6.data.last);
}
void S6_appInit(void){
 S6_Config c;unsigned j;uint16_t registers[32];
 memset((void*)&gStage6,0,sizeof(gStage6));S6_profileLoad(&c);S6_init(&gS6,&c);S6_statsInit(&gStage6.data.stats,&c);S6_systemInit(&gSystem,S6_PWM_LOCKED);
 {S6_PWMConfig pc;S6_pwmDefault(&pc);S6_pwmInit(&gPWM,&pc,S6_PWM_LOCKED);}
 put32(0U,0x53364631UL);put32(2U,S6_BUILD);H[4]=3U;H[5]=4096U;H[7]=gS6.state;H[13]=S6_blockers(&gS6);H[15]=1U;
 put32(24U,0xFFFFFFFFUL);put32(60U,120000000UL);put32(62U,S6_PERIOD);H[64]=128U;H[65]=16U;
 H[46]=gSystem.state;H[47]=S6_systemQualifications(&gSystem,&gS6,0U);H[67]=S6_PWM_LOCKED;
 H[66]=0U; /* no physical actuation compiled */
 S6P_init();S6P_registers(registers);for(j=0;j<32;j++)H[96U+j]=registers[j];
 accepted=active=done=recordCount=rejected=exitPending=stopRequested=0U;
 sequence=lastEntry=maxC=maxWrapper=maxUpdate=updateCount=0UL;
 minInterval=0xFFFFFFFFUL;maxInterval=intervalCount=overflowCount=deadlineCount=0UL;
 algorithmTicks=cTicks=stagedAt=stepCount=maxStep=0UL;gStage6Command=0U;
 memset(&input,0,sizeof(input));memset(postOVF,0,sizeof(postOVF));
}
void S6_onISR(uint32_t entry){
 uint32_t before,dt;unsigned j;uint16_t index;
 if(!active){S6P_stop();S6P_interruptAck();return;}
 if(exitPending){S6_fault(&gS6,S6_F_COUNTER,sequence+1UL);S6P_stop();S6P_interruptAck();active=0U;done=1U;return;}
 if(sequence==0xFFFFFFFFUL){S6_fault(&gS6,S6_F_COUNTER,sequence);S6P_stop();S6P_interruptAck();active=0U;done=1U;return;}
 sequence++;S6P_read(&input);input.sequence=sequence;input.entry=entry;
 if(sequence>1UL){dt=lastEntry-entry;intervalCount++;if(dt<minInterval)minInterval=dt;if(dt>maxInterval)maxInterval=dt;}
 lastEntry=entry;S6P_ack(&input);S6P_finishInput(&input);
 /* Single ISR owner consumes the foreground stop mailbox before control. */
 if(stopRequested){
  stopRequested=0U;
  if(!S6_systemEvent(&gSystem,&gS6,S6_EVENT_STOP,sequence))S6_systemFault(&gSystem,&gS6,S6_F_CONFIG,sequence);
 }
 before=S6P_time();S6_systemStep(&gSystem,&gS6,&input);algorithmTicks=before-S6P_time();stepCount++;if(algorithmTicks>maxStep)maxStep=algorithmTicks;
 if(gS6.mode==S6_CMD_FEEDBACK && !gS6.faults){
  before=S6P_time();
  if(!S6_statsStep(&gStage6.data.stats,&gS6.config,&input))S6_fault(&gS6,S6_F_COUNTER,sequence);
  dt=before-S6P_time();
  if(dt>((uint32_t)gStage6.data.stats.data.header[34]|((uint32_t)gStage6.data.stats.data.header[35]<<16))){
   gStage6.data.stats.data.header[34]=(uint16_t)dt;gStage6.data.stats.data.header[35]=(uint16_t)(dt>>16);
  }
 }
 stagedAt=0UL;
 if(gS6.pwm_staged){
  /* Locked backend publishes a coherent three-phase RAM transaction. It
     never claims a hardware shadow load occurred. Next frame consumes the
     prior RAM transaction, independently of physical ePWM load evidence. */
  if(gPWM.pending && !S6_pwmLoaded(&gPWM,gPWM.committed.sequence,gSystem.session))S6_fault(&gS6,S6_F_EXECUTOR,sequence);
  if(!S6_pwmPrepare(&gPWM,gS6.foc.output.duty_preview,sequence,gSystem.session,input.release) ||
     !S6_pwmCommit(&gPWM,S6P_time(),gSystem.session,0U))S6_fault(&gS6,S6_F_EXECUTOR,sequence);
  if(!gS6.faults && S6P_stagePWM(gPWM.committed.compare,0U)!=0U)S6_fault(&gS6,S6_F_LOCK,sequence);
  stagedAt=S6P_time();dt=input.release-stagedAt;updateCount++;
  if(dt>maxUpdate)maxUpdate=dt;
  if(dt>=S6_PERIOD){deadlineCount++;S6_fault(&gS6,S6_F_DEADLINE,sequence);}
 }
 S6P_overflow(postOVF);for(j=0;j<3;j++)if(postOVF[j]&1U){S6_fault(&gS6,S6_F_ADC_OVERFLOW,sequence);}
 if(sequence%256UL==0UL && recordCount<16U){
  index=recordCount++;snapshot(&gStage6.data.snapshot[index]);
 }
 /* A second OVF read covers bounded snapshot work. No OVF clear/retry. */
 S6P_overflow(postOVF);for(j=0;j<3;j++)if(postOVF[j]&1U)S6_fault(&gS6,S6_F_ADC_OVERFLOW,sequence);
 for(j=0;j<3;j++)if((input.overflow[j]|input.pre_overflow[j]|postOVF[j])&1U){overflowCount++;break;}
 S6P_interruptAck();cTicks=entry-S6P_time();if(cTicks>maxC)maxC=cTicks;
 if(cTicks>=S6_PERIOD)S6_fault(&gS6,S6_F_BUDGET,sequence);
 if(gS6.mode==S6_CMD_FEEDBACK){
  unsigned off=gS6.faults?40U:(sequence%256UL==0UL?38U:36U);
  uint32_t old=(uint32_t)gStage6.data.stats.data.header[off]|((uint32_t)gStage6.data.stats.data.header[off+1U]<<16);
  if(cTicks>old){gStage6.data.stats.data.header[off]=(uint16_t)cTicks;gStage6.data.stats.data.header[off+1U]=(uint16_t)(cTicks>>16);}
  if(sequence==4096UL){gStage6.data.stats.data.header[42]=(uint16_t)cTicks;gStage6.data.stats.data.header[43]=(uint16_t)(cTicks>>16);}
 }
 if(recordCount && sequence%256UL==0UL && sequence<=S6_DIAG_STEPS){
  gStage6.data.snapshot[recordCount-1U].data.c_ticks=cTicks;
  for(j=0;j<3;j++)gStage6.data.snapshot[recordCount-1U].data.post_overflow[j]=postOVF[j];
 }
 if(gS6.first_fault && !H[51] && gS6.processed && input.sequence==gS6.first_sequence){snapshot(&gStage6.data.first);H[51]=1U;}
 if(gS6.faults || ((gS6.mode==S6_CMD_DIAG || gS6.mode==S6_CMD_FEEDBACK) && sequence>=S6_DIAG_STEPS) || gS6.state==S6_STOPPED){
  if(!gS6.faults)gS6.state=S6_STOPPED;
  active=0U;done=1U;S6P_stop();
 }
 exitPending=1U;
}
void S6_onExit(uint32_t tick){
 uint32_t elapsed;
 if(!exitPending)return;
 elapsed=input.entry-tick;if(elapsed>maxWrapper)maxWrapper=elapsed;
 if(elapsed>=S6_PERIOD){S6_fault(&gS6,S6_F_BUDGET,sequence);active=0U;done=1U;S6P_stop();}
 /* This is a wrapper tail timestamp, before epilogue/IRET. No foreground
    per-tick handshake. First-fault capture is terminal only. */
 if(gS6.first_fault && !H[51] && gS6.processed && input.sequence==gS6.first_sequence){snapshot(&gStage6.data.first);H[51]=1U;}
 exitPending=0U;
}
void S6_poll(void){
 uint16_t cmd=gStage6Command;
 S6P_backgroundLock();
 if(cmd){
  gStage6Command=0U;
  if(cmd==S6_CMD_STOP && active){
   if(gSystem.state==S6_SYS_SHADOW || gSystem.state==S6_SYS_CURRENT || gSystem.state==S6_SYS_SPEED){
    stopRequested=1U; /* no foreground writes to live PI/state floats */
   }else{S6P_stop();active=0U;done=1U;S6_begin(&gS6,cmd);}
  }
  else if(cmd==S6_CMD_STOP || accepted || active || H[10] || !S6_systemBegin(&gSystem,&gS6,cmd)){if(rejected<65535U)rejected++;H[12]=rejected;done=1U;}
  else{
   accepted=1U;H[6]=1U;H[8]=cmd;H[9]=1U;
   if(S6P_preflight()){S6_fault(&gS6,S6_F_ADC_OVERFLOW,0U);done=1U;}
   else{startTick=S6P_time();active=1U;S6P_start();}
  }
 }
 if(active && (sequence?lastEntry:startTick)-S6P_time()>120000000UL){S6P_stop();active=0U;done=1U;S6_fault(&gS6,S6_F_WAIT,sequence);}
 if(done && !H[10]){publish();S6_doneStop();}
}
