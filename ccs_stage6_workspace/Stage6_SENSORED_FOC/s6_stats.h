#ifndef S6_STATS_H
#define S6_STATS_H
#include "s6_control.h"
/* Eight 512-sample segments. Exact centered square sum <=512*2048^2=2^31.
   No uint64, float, division or square root in the sampling update. */
typedef struct {
 uint32_t count,sum,square_center;
 uint16_t min,max,saturated,jumps,reserved[6];
} S6_StatCell;
typedef union {
 uint16_t words[96];
 struct {
  uint32_t count,sum[3];
  uint16_t min[3],max[3];
  uint16_t padding[2];
  float candidate[3];
  uint32_t reject,first_sequence;
  uint16_t first_channel,first_value,complete,eligible,approved,reserved;
 } data;
} S6_CalCandidate;
typedef union {
 uint16_t words[1280];
 struct {
  uint16_t header[64];
  S6_StatCell total[4];
  S6_StatCell segment[8][4];
  uint16_t raw_window[128][4]; /* first 64 + last 64 ring, never filtered */
  uint16_t anomaly[32];
  S6_CalCandidate calibration;
 } data;
} S6_Stats;
#define S6_CAL_INTERFACE 1UL
#define S6_CAL_ZERO 2UL
#define S6_CAL_RANGE 4UL
#define S6_CAL_SPAN 8UL
#define S6_CAL_SAT 16UL
#define S6_CAL_INCOMPLETE 32UL
#define S6_STAT_OVERFLOW 64UL
void S6_statsInit(volatile S6_Stats *s,const S6_Config *c);
int S6_statsStep(volatile S6_Stats *s,const S6_Config *c,const S6_Input *x);
void S6_statsFinish(volatile S6_Stats *s);
#endif
