#ifndef S6_ALIGNMENT_H
#define S6_ALIGNMENT_H
#include "s6_control.h"
typedef struct {
 float current_A,tolerance_A;
 uint32_t ramp_frames,hold_frames,max_hold_motion;
 uint16_t reviewed,reserved;
} S6_AlignConfig;
typedef struct {
 S6_AlignConfig config;
 uint32_t origin,hold_origin,frames,candidate_raw;
 uint16_t primed,candidate_valid;
} S6_Alignment;
int S6_alignBegin(S6_Alignment *a,S6_State *c);
int S6_alignStep(S6_Alignment *a,S6_State *c,const S6_Input *x);
#endif
