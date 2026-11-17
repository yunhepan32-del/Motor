#ifndef S6_CONTROL_H
#define S6_CONTROL_H
#include "sensored_foc.h"
#include <stddef.h>
#define S6_BUILD 0x26111702UL
#define S6_PERIOD 6000UL
#define S6_DIAG_STEPS 4096UL
#define S6_BIAS_SAMPLES 2048U
/* Evidence belongs to an offline-reviewed hardware profile, never WATCH flags. */
#define S6_E_CURRENT 1U
#define S6_E_BUS 2U
#define S6_E_ENCODER 4U
#define S6_E_ZERO 8U
#define S6_E_PWM 16U
#define S6_E_PROTECTION 32U
#define S6_E_REALTIME 64U
#define S6_E_INTERFACE 128U
#define S6_E_ZERO_CURRENT 256U
#define S6_E_GAINS 512U
#define S6_E_OFFSET 1024U
#define S6_F_ADC_MISSING 1UL
#define S6_F_ADC_OVERFLOW 2UL
#define S6_F_INTERVAL 4UL
#define S6_F_LOCK 8UL
#define S6_F_QEP 16UL
#define S6_F_INPUT 32UL
#define S6_F_ALGORITHM 64UL
#define S6_F_BUDGET 128UL
#define S6_F_DEADLINE 256UL
#define S6_F_BIAS 512UL
#define S6_F_CONFIG 1024UL
#define S6_F_WAIT 2048UL
#define S6_F_COUNTER 4096UL
#define S6_F_EXECUTOR 8192UL
#define S6_B_INTERFACE 1U
#define S6_B_CURRENT 2U
#define S6_B_OFFSET 4U
#define S6_B_BUS 8U
#define S6_B_ENCODER 16U
#define S6_B_ZERO 32U
#define S6_B_GAINS 64U
#define S6_B_PROTECTION 128U
#define S6_B_PWM 256U
#define S6_B_REALTIME 512U
enum {S6_LOCKED=1,S6_RAW=2,S6_CALIBRATING=3,S6_CURRENT_SHADOW=4,
      S6_SPEED_SHADOW=5,S6_STOPPED=6,S6_FAULT=7};
enum {S6_CMD_DIAG=1,S6_CMD_BIAS=2,S6_CMD_CURRENT=3,S6_CMD_SPEED=4,S6_CMD_STOP=5,S6_CMD_FEEDBACK=6};
typedef struct {
 SF_Config foc;
 uint16_t evidence,bias_samples,bias_span_max,bias_min,bias_max;
 uint16_t speed_divider,reserved;
 float current_A_per_count[3],bus_V_per_count,bus_zero_count;
 float current_zero_count[3]; /* reviewed calibration, not nominal 2048 */
 float speed_kp_A_per_rad_s,speed_ki_A_per_rad,speed_iq_limit_A;
 float speed_slew_rad_s2,iq_slew_A_s;
 float maximum_speed_rad_s;
 uint16_t sd_healthy_level,profile_version;
} S6_Config;
typedef struct {
 uint16_t count,valid,min[3],max[3];
 uint32_t sum[3];
 float offset[3];
 uint16_t reject_reason,reject_channel,reject_raw,reserved;
 uint32_t reject_sequence;
} S6_Bias;
typedef struct {
 float integral,iq,target,reference,feedback;
 int32_t count_sum;
 uint32_t ticks_sum;
 uint16_t count,primed;
} S6_Speed;
typedef struct {
 uint32_t sequence,entry,sample_tick,qep_tick,release;
 uint32_t position;
 uint16_t raw[4],flags[3],overflow[3],pre_overflow[3];
 uint16_t qep_status,qep_flags,tz[3],gpio_mask,sd,trip,latch;
} S6_Input;
typedef struct {
 S6_Config config;SF_State foc;S6_Bias bias;S6_Speed speed;
 uint32_t faults,first_fault,first_sequence,processed,foc_calls,foc_success;
 uint32_t previous_entry,previous_sequence;
 uint16_t state,mode,blockers,primed,called,conversion_valid,pwm_staged;
 uint16_t compare[3];
 float current[3],bus,id_ref,iq_ref,speed_ref;
} S6_State;
int S6_finite(float x);
void S6_profileLoad(S6_Config *c);
int S6_configValid(const S6_Config *c);
void S6_biasInit(S6_Bias *b);
int S6_biasStep(S6_Bias *b,const S6_Config *c,const uint16_t raw[4]);
int S6_convert(S6_State *s,const S6_Input *x);
int S6_speedStep(S6_Speed *s,const S6_Config *c,int32_t delta,uint32_t ticks,float request,float old_iq);
int S6_speedConfigValid(const S6_Config *c);
void S6_init(S6_State *s,const S6_Config *c);
uint16_t S6_blockers(const S6_State *s);
int S6_begin(S6_State *s,uint16_t command);
int S6_step(S6_State *s,const S6_Input *x);
/* Future reviewed backend / host simulation only. Running checks require no
   active trips, unlike the always-OST locked acquisition predicate. */
int S6_stepRunning(S6_State *s,const S6_Input *x);
void S6_fault(S6_State *s,uint32_t bits,uint32_t sequence);
int S6_preparePWM(S6_State *s);
int S6_setReferences(S6_State *s,float id_A,float iq_A,float speed_rad_s);
/* Permanent compile-time authority: no certificate or command enables power. */
int S6_powerAllowed(void);
#endif
