#ifndef STAGE5_ENCODER_FEEDBACK_H
#define STAGE5_ENCODER_FEEDBACK_H
#include <stdint.h>

/* Pure calculation module. No DriverLib, MMIO, PWM, fault clear or enable API. */
#define EF_VERSION 0x26111601UL
#define EF_CERT_COUNTS     0x0001U
#define EF_CERT_DIRECTION  0x0002U
#define EF_CERT_POLE_PAIRS 0x0004U
#define EF_CERT_ZERO       0x0008U
#define EF_CERT_CLOCK      0x0010U
#define EF_FAULT_CONFIG    0x0001U
#define EF_FAULT_SEQUENCE  0x0002U
#define EF_FAULT_TIME      0x0004U
#define EF_FAULT_QEP       0x0008U
#define EF_FAULT_POSITION  0x0010U

typedef struct {
    uint32_t counts_per_rev;
    uint32_t timer_hz;
    uint32_t max_interval_ticks;
    uint32_t zero_origin_raw; /* Zero calibration is tied to this run origin. */
    uint32_t zero_origin_epoch; /* Caller session ID; stale zero records reject. */
    uint16_t pole_pairs;
    int16_t direction;       /* +1/-1 is a convention until certified. */
    uint16_t certificates;  /* Caller evidence, never inferred from motion/Z. */
    float electrical_offset_rad; /* [-pi, pi]; no fitted offset is generated. */
} EF_Config;

typedef struct {
    uint32_t sequence;
    uint32_t raw_position;
    uint32_t timer_down;
    uint16_t qep_status;
    uint16_t qep_flags;
} EF_Sample;

typedef struct {
    int64_t relative_counts;
    int32_t delta_counts;
    uint32_t interval_ticks;
    uint32_t mechanical_phase_counts;
    uint32_t electrical_phase_counts;
    float mechanical_relative_rad; /* [0,2pi), origin-relative, reference CPR */
    float electrical_preview_rad;  /* [-pi,pi), not a magnetic zero by default */
    float mechanical_speed_rad_s;  /* Window-average, not instantaneous speed */
    float electrical_speed_rad_s;
    uint16_t sample_valid;
    uint16_t angle_qualified;
    uint16_t speed_qualified;
    uint16_t first_index_seen; /* Sticky FIMF observation, NOT an index count */
    uint16_t fault_latch;
    uint16_t power_permitted; /* Always zero: math qualification grants no power */
} EF_Output;

typedef struct {
    uint32_t version;
    EF_Config config;
    EF_Output output;
    uint32_t origin_raw;
    uint32_t previous_raw;
    uint32_t previous_timer;
    uint32_t previous_sequence;
} EF_State;

/* New origin explicitly invalidates any zero record for a different origin. */
int EF_init(EF_State *state, const EF_Config *config, uint32_t origin_raw,
            uint32_t origin_timer, uint32_t previous_sequence,
            uint32_t origin_epoch);
/* First error latches; no retry/clear API. Explicit reinitialization is separate. */
int EF_step(EF_State *state, const EF_Sample *sample);
#endif
