#include "s6_control.h"
#include <string.h>
#include "s6_calibration_profile.h"
#include "s6_motor.h"
#include <float.h>
int S6_finite(float x){return x<=FLT_MAX && x>=-FLT_MAX;}
void S6_profileLoad(S6_Config *c){
 S6_Motor motor;
 memset(c,0,sizeof(*c));
 /* Source references: Wildfire Encoder example 1000 PPR x4, four pole pairs.
    Direction, magnetic zero, current gains/bias and bus affine transfer UNKNOWN.
    Reference constants do not grant any evidence bit. */
 c->foc.encoder.counts_per_rev=4000UL;c->foc.encoder.pole_pairs=4U;
 c->foc.encoder.direction=1;c->foc.encoder.timer_hz=120000000UL;
 c->foc.encoder.max_interval_ticks=6600UL;
 c->foc.period_ticks=S6_PERIOD;c->foc.max_jitter_ticks=600UL;
 c->foc.max_skew_ticks=1200UL;c->foc.max_horizon_ticks=6000UL;
 c->foc.voltage_fraction=0.577350269f;
 /* Bounds are conservative SOFTWARE DEVELOPMENT limits, not measured ratings
    or verified protection settings. Kp/Ki remain unset and block FOC. */
 c->foc.min_bus_V=1.0f;c->foc.max_bus_V=60.0f;c->foc.current_limit_A=2.5f;
 c->bias_samples=S6_BIAS_SAMPLES;c->bias_span_max=32U;
 c->bias_min=1536U;c->bias_max=2560U;
 c->speed_divider=20U;
 c->maximum_speed_rad_s=1000.0f; /* SOFTWARE ceiling, not a motor rating */
 c->sd_healthy_level=0U;c->profile_version=3U;
 c->evidence=S6_PROFILE_EVIDENCE;
 c->current_A_per_count[0]=S6_PROFILE_IU_GAIN;c->current_zero_count[0]=S6_PROFILE_IU_ZERO;
 c->current_A_per_count[1]=S6_PROFILE_IV_GAIN;c->current_zero_count[1]=S6_PROFILE_IV_ZERO;
 c->current_A_per_count[2]=S6_PROFILE_IW_GAIN;c->current_zero_count[2]=S6_PROFILE_IW_ZERO;
 c->bus_V_per_count=S6_PROFILE_VBUS_GAIN;c->bus_zero_count=S6_PROFILE_VBUS_ZERO;
 /* Use the narrowest reviewed phase range as a conservative common bound. */
 if(c->evidence&S6_E_CURRENT){
  if(S6_PROFILE_IU_RANGE<c->foc.current_limit_A)c->foc.current_limit_A=S6_PROFILE_IU_RANGE;
  if(S6_PROFILE_IV_RANGE<c->foc.current_limit_A)c->foc.current_limit_A=S6_PROFILE_IV_RANGE;
  if(S6_PROFILE_IW_RANGE<c->foc.current_limit_A)c->foc.current_limit_A=S6_PROFILE_IW_RANGE;
 }
 if((c->evidence&S6_E_BUS) && S6_PROFILE_VBUS_RANGE<c->foc.max_bus_V)c->foc.max_bus_V=S6_PROFILE_VBUS_RANGE;
 S6_motorLoad(&motor);
 if(motor.reviewed && !S6_motorApply(&motor,c))c->profile_version=0U;
}
int S6_configValid(const S6_Config *c){
 unsigned j;if(c==0 || (c->evidence&~2047U)!=0U)return 0;
 if(c->foc.period_ticks!=S6_PERIOD || c->foc.encoder.timer_hz!=120000000UL ||
    c->bias_samples<16U || c->bias_samples>S6_BIAS_SAMPLES ||
    c->bias_min>=c->bias_max || c->bias_max>=4095U || c->bias_span_max>128U ||
    c->speed_divider<1U || c->speed_divider>100U || c->sd_healthy_level>1U ||
    c->profile_version!=3U || !(c->maximum_speed_rad_s>0.0f && c->maximum_speed_rad_s<=1000.0f))return 0;
 if(c->evidence&S6_E_CURRENT)for(j=0U;j<3U;j++)
  if(!S6_finite(c->current_A_per_count[j]) ||
     !(c->current_A_per_count[j]>-1.0f && c->current_A_per_count[j]<1.0f) ||
     c->current_A_per_count[j]==0.0f)return 0;
 if((c->evidence&S6_E_BUS) && (!(c->bus_V_per_count>0.0f && c->bus_V_per_count<1.0f) ||
    !(c->bus_zero_count>=-4096.0f && c->bus_zero_count<=4096.0f)))return 0;
 if(c->evidence&S6_E_OFFSET)for(j=0;j<3;j++)
  if(!(c->current_zero_count[j]>=c->bias_min && c->current_zero_count[j]<=c->bias_max))return 0;
 if((c->evidence&S6_E_ENCODER) && ((c->foc.encoder.certificates&23U)!=23U ||
    c->foc.encoder.counts_per_rev<4UL || !c->foc.encoder.pole_pairs ||
    (c->foc.encoder.direction!=1 && c->foc.encoder.direction!=-1)))return 0;
 if((c->evidence&S6_E_ZERO) && ((c->foc.encoder.certificates&EF_CERT_ZERO)==0U ||
    c->foc.encoder.zero_origin_epoch!=S6_BUILD))return 0;
 if((c->evidence&S6_E_GAINS) && (!(c->foc.kp_V_per_A>0.0f && c->foc.kp_V_per_A<=100.0f) ||
    !(c->foc.ki_series>=0.0f && c->foc.ki_series<=1.0f)))return 0;
 if((c->evidence&S6_E_CURRENT) && !(c->foc.current_limit_A>0.0f && c->foc.current_limit_A<=100.0f))return 0;
 if((c->evidence&S6_E_BUS) && !(c->foc.min_bus_V>=1.0f && c->foc.max_bus_V>=c->foc.min_bus_V && c->foc.max_bus_V<=1000.0f))return 0;
 return 1;
}
int S6_powerAllowed(void){return 0;}
