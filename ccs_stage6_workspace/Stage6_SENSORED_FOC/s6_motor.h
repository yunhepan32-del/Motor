#ifndef S6_MOTOR_H
#define S6_MOTOR_H
#include "s6_control.h"
typedef struct {
 uint32_t build,counts_per_rev;
 uint16_t reviewed,pole_pairs;
 int16_t direction;
 float Rs,Ld,Lq,flux,J,friction,rated_current,max_speed;
 float current_bandwidth_Hz,speed_bandwidth_Hz,damping;
 float speed_slew_rad_s2,iq_slew_A_s;
} S6_Motor;
void S6_motorLoad(S6_Motor *m);
int S6_motorValid(const S6_Motor *m);
int S6_motorApply(const S6_Motor *m,S6_Config *c);
#endif
