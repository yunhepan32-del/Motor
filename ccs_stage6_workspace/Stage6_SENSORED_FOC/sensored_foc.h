#ifndef SENSORED_FOC_H
#define SENSORED_FOC_H
#include "encoder_feedback.h"
#include "libraries/transforms/clarke/clarke.h"
#include "libraries/transforms/park/park.h"
#include "libraries/transforms/ipark/ipark.h"
#include "libraries/transforms/svgen/svgen.h"
#include "libraries/control/pi/pi.h"

#define SF_VERSION 0x26111603UL
#define SF_FAULT_CONFIG 1U
#define SF_FAULT_FRAME 2U
#define SF_FAULT_TIME 4U
#define SF_FAULT_INPUT 8U
#define SF_FAULT_ENCODER 16U
#define SF_FAULT_MATH 32U
typedef struct {
    EF_Config encoder;
    uint32_t period_ticks,max_jitter_ticks,max_skew_ticks,max_horizon_ticks;
    float kp_V_per_A,ki_series,min_bus_V,max_bus_V,current_limit_A;
    float voltage_fraction;
} SF_Config;
typedef struct {
    EF_Sample encoder;
    uint32_t adc_sequence,adc_timer_down,apply_timer_down;
    float current_A[3],vbus_V,id_ref_A,iq_ref_A;
    uint16_t current_calibrated,bus_calibrated;
} SF_Frame;
typedef struct {
    float theta_sample,theta_apply,omega_mech,omega_elec;
    float i_alpha,i_beta,id,iq,vd,vq,v_alpha,v_beta;
    float modulation[3],duty_preview[3],ui_d,ui_q,vmax,qmax,current_sum,advance_rad;
    uint16_t valid,angle_qualified,current_qualified,power_permitted,fault_latch;
} SF_Output;
typedef struct {
    uint32_t version,last_adc_timer,last_sequence;
    SF_Config config;
    EF_State encoder;
    CLARKE_Obj clarke;
    PARK_Obj park;
    IPARK_Obj ipark;
    SVGEN_Obj svgen;
    PI_Obj d,q;
    SF_Output output;
} SF_State;
int SF_init(SF_State*,const SF_Config*,uint32_t origin_raw,uint32_t timer,
            uint32_t origin_sequence,uint32_t origin_epoch);
/* Calculation only. No MMIO, PWM writes, ISR, retries or power-enable path. */
int SF_step(SF_State*,const SF_Frame*);
#endif
