#include "encoder_feedback.h"
#include <limits.h>
#include <string.h>

#define EF_PI 3.14159265358979323846f
#define EF_TWO_PI 6.28318530717958647692f

static int fail(EF_State *s, uint16_t fault)
{
    s->output.fault_latch |= fault;
    s->output.sample_valid = 0U;
    s->output.angle_qualified = 0U;
    s->output.speed_qualified = 0U;
    s->output.power_permitted = 0U;
    return 0;
}

int EF_init(EF_State *s, const EF_Config *c, uint32_t origin_raw,
            uint32_t origin_timer, uint32_t previous_sequence,
            uint32_t origin_epoch)
{
    EF_Config copy;
    if(s == 0) return 0;
    if(c != 0) copy = *c;
    memset(s, 0, sizeof(*s));
    if(c == 0) return fail(s, EF_FAULT_CONFIG);
    c = &copy;
    /* Range checks also reject NaN and infinity without target math intrinsics. */
    if(c->counts_per_rev < 4U || c->counts_per_rev > 16777216UL ||
       c->timer_hz == 0U || c->pole_pairs == 0U ||
       c->max_interval_ticks == 0U || c->max_interval_ticks > 0x7FFFFFFFUL ||
       (c->direction != 1 && c->direction != -1) ||
       (c->certificates & (uint16_t)~0x001FU) != 0U ||
       !(c->electrical_offset_rad >= -EF_PI &&
         c->electrical_offset_rad <= EF_PI) ||
       ((c->certificates & EF_CERT_ZERO) != 0U &&
        (c->zero_origin_raw != origin_raw || c->zero_origin_epoch == 0U ||
         c->zero_origin_epoch != origin_epoch)))
        return fail(s, EF_FAULT_CONFIG);
    s->version = EF_VERSION;
    s->config = *c;
    s->origin_raw = origin_raw;
    s->previous_raw = origin_raw;
    s->previous_timer = origin_timer;
    s->previous_sequence = previous_sequence;
    return 1;
}

int EF_step(EF_State *s, const EF_Sample *x)
{
    uint32_t d, dt, mech, elec;
    int32_t delta;
    int64_t total, rem;
    float angle, speed;
    uint16_t angle_mask, speed_mask;
    if(s == 0) return 0;
    if(s->version != EF_VERSION) return fail(s, EF_FAULT_CONFIG);
    if(s->output.fault_latch != 0U) return 0;
    if(x == 0) return fail(s, EF_FAULT_SEQUENCE);
    if((uint32_t)(x->sequence - s->previous_sequence) != 1U)
        return fail(s, EF_FAULT_SEQUENCE);
    dt = s->previous_timer - x->timer_down;
    if(dt == 0U || dt > s->config.max_interval_ticks)
        return fail(s, EF_FAULT_TIME);
    /* Same PCE/PHE error bits as the frozen recorder; QDC/PCU are not faults. */
    if((x->qep_flags & 0x0006U) != 0U || (x->qep_status & 0x0001U) != 0U)
        return fail(s, EF_FAULT_QEP);
    d = x->raw_position - s->previous_raw;
    if(d == 0x80000000UL) return fail(s, EF_FAULT_POSITION);
    if(d <= 0x7FFFFFFFUL) delta = (int32_t)d;
    else delta = -1L - (int32_t)(0xFFFFFFFFUL-d);
    if(s->config.direction < 0) delta = -delta;
    total = s->output.relative_counts;
    if((delta > 0 && total > INT64_MAX - (int64_t)delta) ||
       (delta < 0 && total < INT64_MIN - (int64_t)delta))
        return fail(s, EF_FAULT_POSITION);
    total += delta;
    rem = total % (int64_t)s->config.counts_per_rev;
    if(rem < 0) rem += s->config.counts_per_rev;
    mech = (uint32_t)rem;
    /* Integer modulo before float conversion avoids loss near 2^32 rollover. */
    elec = (uint32_t)(((uint64_t)mech * s->config.pole_pairs) %
                     s->config.counts_per_rev);
    s->output.mechanical_relative_rad = EF_TWO_PI *
        ((float)mech / (float)s->config.counts_per_rev);
    if(s->output.mechanical_relative_rad >= EF_TWO_PI)
        s->output.mechanical_relative_rad = 0.0f;
    angle = EF_TWO_PI * ((float)elec / (float)s->config.counts_per_rev)
            - s->config.electrical_offset_rad;
    if(angle >= EF_PI) angle -= EF_TWO_PI;
    if(angle < -EF_PI) angle += EF_TWO_PI;
    speed = (float)delta * ((float)s->config.timer_hz / (float)dt);
    speed *= EF_TWO_PI / (float)s->config.counts_per_rev;
    angle_mask = EF_CERT_COUNTS | EF_CERT_DIRECTION | EF_CERT_POLE_PAIRS |
                 EF_CERT_ZERO;
    speed_mask = EF_CERT_COUNTS | EF_CERT_DIRECTION | EF_CERT_CLOCK |
                 EF_CERT_POLE_PAIRS;
    s->output.relative_counts = total;
    s->output.delta_counts = delta;
    s->output.interval_ticks = dt;
    s->output.mechanical_phase_counts = mech;
    s->output.electrical_phase_counts = elec;
    s->output.electrical_preview_rad = angle;
    s->output.mechanical_speed_rad_s = speed;
    s->output.electrical_speed_rad_s = speed * s->config.pole_pairs;
    s->output.sample_valid = 1U;
    s->output.angle_qualified =
        (uint16_t)((s->config.certificates & angle_mask) == angle_mask);
    s->output.speed_qualified =
        (uint16_t)((s->config.certificates & speed_mask) == speed_mask);
    s->output.first_index_seen |= (uint16_t)((x->qep_status & 0x0002U) != 0U);
    s->output.power_permitted = 0U;
    s->previous_raw = x->raw_position;
    s->previous_timer = x->timer_down;
    s->previous_sequence = x->sequence;
    return 1;
}
