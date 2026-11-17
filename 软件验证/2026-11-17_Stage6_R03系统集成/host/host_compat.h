#ifndef OFFLINE_HOST_COMPAT_H
#define OFFLINE_HOST_COMPAT_H
/* Host mathematical substitutes only. NaN propagation is explicit; no claim
 * of target intrinsic NaN/rounding equivalence. No board or HAL emulation. */
#include <math.h>
static inline float offline_sat(float x,float hi,float lo) {
    return x > hi ? hi : (x < lo ? lo : x);
}
#define __fsat offline_sat
#define __sin sinf
#define __cos cosf
#define __sqrt sqrtf
#define __atan2 atan2f
#define __sinpuf32(x) sinf((x)*6.2831853071795864769f)
#define __cospuf32(x) cosf((x)*6.2831853071795864769f)
#define __atan2puf32(y,x) (atan2f((y),(x))*0.15915494309189533577f)
#endif
