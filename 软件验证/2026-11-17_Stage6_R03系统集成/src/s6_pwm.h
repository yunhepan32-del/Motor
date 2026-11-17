#ifndef S6_PWM_H
#define S6_PWM_H
#include <stdint.h>
enum {S6_PWM_LOCKED=0,S6_PWM_SIMULATED=1,S6_PWM_TARGET=2};
enum {S6_PWM_CONFIG=1,S6_PWM_DUTY=2,S6_PWM_ORDER=4,S6_PWM_LATE=8,
      S6_PWM_WINDOW=16,S6_PWM_AUTH=32,S6_PWM_BACKEND=64};
typedef struct {
 uint16_t period,dead,minimum_edge,blank,settle,acquire,guard;
 uint16_t sample_at_zero,polarity_reviewed,window_reviewed;
} S6_PWMConfig;
typedef struct {
 uint32_t sequence,session,release,apply;
 uint16_t compare[3],window_valid,valid;
} S6_PWMFrame;
typedef struct {
 S6_PWMConfig config;
 S6_PWMFrame prepared,committed,loaded;
 uint32_t commits,loads,first_error_sequence,first_error;
 uint16_t backend,pending,prepared_valid,locked;
} S6_PWM;
void S6_pwmDefault(S6_PWMConfig *c);
int S6_pwmInit(S6_PWM *p,const S6_PWMConfig *c,uint16_t backend);
int S6_pwmPrepare(S6_PWM *p,const float duty[3],uint32_t seq,uint32_t session,uint32_t release);
int S6_pwmCheckCommit(S6_PWM *p,uint32_t now,uint32_t session,uint16_t authorized);
int S6_pwmCommit(S6_PWM *p,uint32_t now,uint32_t session,uint16_t authorized);
int S6_pwmLoaded(S6_PWM *p,uint32_t seq,uint32_t session);
void S6_pwmFault(S6_PWM *p,uint32_t error,uint32_t seq);
#endif
