#include "sensored_foc.h"
#include <float.h>
#include <math.h>
#include <string.h>
static int finite_value(float x) {return x<=FLT_MAX && x>=-FLT_MAX;}
static int fail(SF_State *s,uint16_t error) {
    uint16_t latched=(uint16_t)(s->output.fault_latch|error);
    memset(&s->output,0,sizeof(s->output));s->output.fault_latch=latched;
    /* PI/encoder states remain available for fault investigation. No retry. */
    return 0;
}
static float wrap(float a) {
    /* Callers bound age/advance below pi; at most two revolutions here. */
    while(a>=MATH_PI)a-=MATH_TWO_PI;
    while(a< -MATH_PI)a+=MATH_TWO_PI;
    return a;
}
int SF_init(SF_State *s,const SF_Config *c,uint32_t raw,uint32_t timer,
            uint32_t sequence,uint32_t epoch) {
    SF_Config copy;
    if(s==0)return 0;
    if(c!=0)copy=*c;
    memset(s,0,sizeof(*s));
    if(c==0)return fail(s,SF_FAULT_CONFIG);
    c=&copy;
    if(c->period_ticks==0U || c->period_ticks>0x3FFFFFFFUL ||
       c->max_jitter_ticks>=c->period_ticks || c->max_skew_ticks>c->period_ticks ||
       c->max_horizon_ticks>c->period_ticks || c->encoder.timer_hz==0U ||
       !(c->kp_V_per_A>0.0f && c->kp_V_per_A<=100.0f) ||
       !(c->ki_series>=0.0f && c->ki_series<=1.0f) ||
       !(c->min_bus_V>=1.0f && c->max_bus_V>=c->min_bus_V && c->max_bus_V<=1000.0f) ||
       !(c->current_limit_A>0.0f && c->current_limit_A<=100.0f) ||
       /* <=1/sqrt(3) retains linear SVGEN range; do not inherit .66 overmod. */
       !(c->voltage_fraction>0.0f && c->voltage_fraction<=0.577350269f))
        return fail(s,SF_FAULT_CONFIG);
    s->config=*c;s->last_adc_timer=timer;s->last_sequence=sequence;
    if(!EF_init(&s->encoder,&c->encoder,raw,timer,sequence,epoch))
        return fail(s,SF_FAULT_ENCODER);
    s->clarke.numSensors=3U;s->clarke.alpha_sf=MATH_ONE_OVER_THREE;
    s->clarke.beta_sf=MATH_ONE_OVER_SQRT_THREE;
    PI_setGains(&s->d,c->kp_V_per_A,c->ki_series);
    PI_setGains(&s->q,c->kp_V_per_A,c->ki_series);
    SVGEN_setMode(&s->svgen,SVM_COM_C);s->version=SF_VERSION;
    return 1;
}
int SF_step(SF_State *s,const SF_Frame *x) {
    uint32_t period,horizon,skew_bits;
    int32_t skew;
    float theta,theta_apply,omega,skew_rad,advance,vmax,qmax,radius;
    unsigned j;
    MATH_Vec3 abc,pwm;MATH_Vec2 ab,dq,vdq,voltage,phasor;
    if(s==0)return 0;
    if(s->output.fault_latch!=0U)return 0;
    if(s->version!=SF_VERSION)return fail(s,SF_FAULT_CONFIG);
    if(x==0 || (uint32_t)(x->adc_sequence-s->last_sequence)!=1U ||
       x->encoder.sequence!=x->adc_sequence)return fail(s,SF_FAULT_FRAME);
    period=s->last_adc_timer-x->adc_timer_down;
    horizon=x->adc_timer_down-x->apply_timer_down;
    skew_bits=x->adc_timer_down-x->encoder.timer_down;
    if(skew_bits==0x80000000UL)return fail(s,SF_FAULT_TIME);
    skew=skew_bits<=0x7FFFFFFFUL?(int32_t)skew_bits:-1L-(int32_t)(0xFFFFFFFFUL-skew_bits);
    if(period<s->config.period_ticks-s->config.max_jitter_ticks ||
       period>s->config.period_ticks+s->config.max_jitter_ticks ||
       horizon>s->config.max_horizon_ticks ||
       skew>(int32_t)s->config.max_skew_ticks || skew<-(int32_t)s->config.max_skew_ticks)
        return fail(s,SF_FAULT_TIME);
    if(!finite_value(x->vbus_V) || x->vbus_V<s->config.min_bus_V || x->vbus_V>s->config.max_bus_V ||
       !finite_value(x->id_ref_A) || !finite_value(x->iq_ref_A) ||
       fabsf(x->id_ref_A)>s->config.current_limit_A || fabsf(x->iq_ref_A)>s->config.current_limit_A ||
       x->current_calibrated>1U || x->bus_calibrated>1U)return fail(s,SF_FAULT_INPUT);
    for(j=0U;j<3U;j++) {
        if(!finite_value(x->current_A[j]) || fabsf(x->current_A[j])>s->config.current_limit_A)
            return fail(s,SF_FAULT_INPUT);
        abc.value[j]=x->current_A[j];
    }
    if(!EF_step(&s->encoder,&x->encoder))return fail(s,SF_FAULT_ENCODER);
    omega=s->encoder.output.electrical_speed_rad_s;
    skew_rad=omega*((float)skew/(float)s->config.encoder.timer_hz);
    advance=omega*((float)horizon/(float)s->config.encoder.timer_hz);
    if(!finite_value(skew_rad) || !finite_value(advance) || fabsf(skew_rad)>MATH_PI || fabsf(advance)>MATH_PI)
        return fail(s,SF_FAULT_TIME);
    /* Encoder read time -> current sample time -> planned voltage application.
       Constant-speed extrapolation is an explicit assumption, not calibration. */
    theta=wrap(s->encoder.output.electrical_preview_rad-skew_rad);
    theta_apply=wrap(theta+advance);
    CLARKE_run_threeInput(&s->clarke,&abc,&ab);
    phasor.value[0]=cosf(theta);phasor.value[1]=sinf(theta);
    PARK_setPhasor(&s->park,&phasor);PARK_run(&s->park,&ab,&dq);
    vmax=s->config.voltage_fraction*x->vbus_V;
    PI_setMinMax(&s->d,-vmax,vmax);
    PI_run_series(&s->d,x->id_ref_A,dq.value[0],0.0f,&vdq.value[0]);
    radius=vmax*vmax-vdq.value[0]*vdq.value[0];
    if(radius<0.0f)radius=0.0f;
    qmax=sqrtf(radius);PI_setMinMax(&s->q,-qmax,qmax);
    PI_run(&s->q,x->iq_ref_A,dq.value[1],&vdq.value[1]);
    phasor.value[0]=cosf(theta_apply);phasor.value[1]=sinf(theta_apply);
    IPARK_setPhasor(&s->ipark,&phasor);IPARK_run(&s->ipark,&vdq,&voltage);
    SVGEN_setup(&s->svgen,1.0f/x->vbus_V);SVGEN_run(&s->svgen,&voltage,&pwm);
    if(!finite_value(voltage.value[0]) || !finite_value(voltage.value[1]))return fail(s,SF_FAULT_MATH);
    for(j=0U;j<3U;j++)if(!finite_value(pwm.value[j]) || fabsf(pwm.value[j])>0.50001f)
        return fail(s,SF_FAULT_MATH);
    s->output.theta_sample=theta;s->output.theta_apply=theta_apply;
    s->output.omega_mech=s->encoder.output.mechanical_speed_rad_s;s->output.omega_elec=omega;
    s->output.i_alpha=ab.value[0];s->output.i_beta=ab.value[1];s->output.id=dq.value[0];s->output.iq=dq.value[1];
    s->output.vd=vdq.value[0];s->output.vq=vdq.value[1];
    s->output.v_alpha=voltage.value[0];s->output.v_beta=voltage.value[1];
    for(j=0U;j<3U;j++) {
        s->output.modulation[j]=pwm.value[j];
        /* RAM mathematical convention only, NOT verified gate/compare polarity. */
        s->output.duty_preview[j]=0.5f+pwm.value[j];
    }
    s->output.ui_d=PI_getUi(&s->d);s->output.ui_q=PI_getUi(&s->q);
    s->output.vmax=vmax;s->output.qmax=qmax;
    s->output.current_sum=abc.value[0]+abc.value[1]+abc.value[2];s->output.advance_rad=advance;
    s->output.valid=1U;s->output.angle_qualified=s->encoder.output.angle_qualified;
    s->output.current_qualified=(uint16_t)(x->current_calibrated && x->bus_calibrated);
    s->output.power_permitted=0U;
    s->last_sequence=x->adc_sequence;s->last_adc_timer=x->adc_timer_down;
    return 1;
}
