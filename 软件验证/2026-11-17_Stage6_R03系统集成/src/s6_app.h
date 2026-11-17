#ifndef S6_APP_H
#define S6_APP_H
#include "s6_control.h"
#include "s6_stats.h"
#include "s6_system.h"
typedef union {
 uint16_t words[128];
 struct {
  uint32_t sequence,entry,sample,qep,release,position,algorithm_ticks,c_ticks;
  uint16_t raw[4],flags[3],overflow[3],pre_overflow[3],post_overflow[3];
  uint16_t tz[3],qep_status,qep_flags,gpio,sd,trip,latch;
  uint16_t state,blockers,called,conversion_valid,pwm_staged,compare[3];
  uint16_t reserved[3];
  float current[3],bus,iq_reference,speed_reference;
  SF_Output output;
 } data;
} S6_Record;
typedef union {
 uint16_t words[4096];
 struct {uint16_t header[128];S6_Record first,last,snapshot[16];S6_Stats stats;uint16_t spare[384];} data;
} S6_Export;
typedef char S6_RECORD_FITS[(sizeof(((S6_Record*)0)->data)<=128U*sizeof(uint16_t))?1:-1];
typedef char S6_RECORD_SIZE[(sizeof(S6_Record)==128U*sizeof(uint16_t))?1:-1];
typedef char S6_EXPORT_SIZE[(sizeof(S6_Export)==4096U*sizeof(uint16_t))?1:-1];
typedef char S6_RECORD_OUTPUT_OFFSET[(offsetof(S6_Record,data.output)==64U*sizeof(uint16_t))?1:-1];
#define S6_OFFSET_ASSERT(tag,type,member,word) typedef char tag[(offsetof(type,member)==(word)*sizeof(uint16_t))?1:-1]
S6_OFFSET_ASSERT(S6_RAW_OFFSET,S6_Record,data.raw,16U);
S6_OFFSET_ASSERT(S6_FLAGS_OFFSET,S6_Record,data.flags,20U);
S6_OFFSET_ASSERT(S6_STATE_OFFSET,S6_Record,data.state,41U);
S6_OFFSET_ASSERT(S6_CURRENT_OFFSET,S6_Record,data.current,52U);
S6_OFFSET_ASSERT(S6_BUS_OFFSET,S6_Record,data.bus,58U);
S6_OFFSET_ASSERT(S6_FIRST_OFFSET,S6_Export,data.first,128U);
S6_OFFSET_ASSERT(S6_LAST_OFFSET,S6_Export,data.last,256U);
S6_OFFSET_ASSERT(S6_SNAPSHOTS_OFFSET,S6_Export,data.snapshot,384U);
S6_OFFSET_ASSERT(S6_STATS_OFFSET,S6_Export,data.stats,2432U);
S6_OFFSET_ASSERT(S6_SEG_OFFSET,S6_Stats,data.segment,128U);
S6_OFFSET_ASSERT(S6_WINDOW_OFFSET,S6_Stats,data.raw_window,640U);
S6_OFFSET_ASSERT(S6_CAL_OFFSET,S6_Stats,data.calibration,1184U);
S6_OFFSET_ASSERT(S6_CANDIDATE_OFFSET,S6_CalCandidate,data.candidate,16U);
S6_OFFSET_ASSERT(S6_CAL_REJECT_OFFSET,S6_CalCandidate,data.reject,22U);
typedef char S6_STATCELL_SIZE[(sizeof(S6_StatCell)==16U*sizeof(uint16_t))?1:-1];
#undef S6_OFFSET_ASSERT
extern volatile S6_Export gStage6;
extern volatile uint16_t gStage6Command;
extern S6_State gS6;
extern S6_System gSystem;
extern S6_PWM gPWM;
void S6_appInit(void);void S6_poll(void);void S6_onISR(uint32_t entry);
void S6_onExit(uint32_t exit_tick);
void S6_readyStop(void);void S6_doneStop(void);
#endif
