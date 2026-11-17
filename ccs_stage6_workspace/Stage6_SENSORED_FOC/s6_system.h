#ifndef S6_SYSTEM_H
#define S6_SYSTEM_H
#include "s6_control.h"
#include "s6_pwm.h"
#include "s6_alignment.h"
enum {S6_SYS_INIT=1,S6_SYS_DISABLED,S6_SYS_SELFTEST,S6_SYS_CALIBRATION,
 S6_SYS_READY,S6_SYS_SHADOW,S6_SYS_POWER_READY,S6_SYS_ZERO_ALIGN,
 S6_SYS_CURRENT,S6_SYS_SPEED,S6_SYS_STOPPING,S6_SYS_FAULT};
enum {S6_Q_FEEDBACK=1,S6_Q_ENCODER=2,S6_Q_CONTROL=4,S6_Q_PWM=8,
 S6_Q_PROTECTION=16,S6_Q_REALTIME=32,S6_Q_OPERATOR=64};
enum {S6_EVENT_SELFTEST=1,S6_EVENT_CALIBRATE,S6_EVENT_SHADOW,
 S6_EVENT_POWER_PREPARE,S6_EVENT_ALIGN,S6_EVENT_CURRENT,S6_EVENT_SPEED,S6_EVENT_STOP};
typedef struct {
 uint32_t build,session,first_sequence,first_fault,transitions,stop_frames;
 uint32_t authority_session,authority_until;
 uint16_t state,qualifications,denied,backend,authority_modes,power_active;
 float stopping_id,stopping_iq;
 float requested_id,requested_iq,requested_speed;
 uint16_t references_pending,reserved;
 S6_Alignment alignment;
} S6_System;
void S6_systemInit(S6_System *s,uint16_t backend);
uint16_t S6_systemQualifications(const S6_System *s,const S6_State *core,uint32_t seq);
int S6_systemEvent(S6_System *s,S6_State *core,uint16_t event,uint32_t seq);
int S6_systemSwitch(S6_System *s,S6_State *core,uint16_t speed_mode);
void S6_systemFault(S6_System *s,S6_State *core,uint32_t bits,uint32_t seq);
int S6_systemStep(S6_System *s,S6_State *core,const S6_Input *x);
int S6_systemBegin(S6_System *s,S6_State *core,uint16_t command);
int S6_systemReferences(S6_System *s,const S6_State *core,float id,float iq,float speed);
/* No hardware command populates operator authority. Future reviewed frontend
   must bind mode+session+finite lifetime, independently of calibration profile. */
#endif
