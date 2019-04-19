#ifdef __cplusplus
extern "C" {
#if 0
} // make emacs happy
#endif
#endif

#ifndef _OPTIMIZED_ROUTINES_ARM_H
#define _OPTIMIZED_ROUTINES_ARM_H

#include <string.h>
static inline void VMove_i16_ARM(const int16 *src, int16 *dst, uint32 N) {
    memcpy(dst, src, N*sizeof(int16));
}
static inline void VMove_i32_ARM(const int32 *src, int32 *dest, uint32 N ) {
    memcpy(dest, src, N*sizeof(int32));
}

//frmath.h
#include "limit_int_arm.h"
#define LimitInt32ToInt16(_x) LimitInt32ToInt16_ARM(_x)
#define LimitInt40ToInt32(_x) LimitInt40ToInt32_ARM(_x)

#endif
#ifdef __cplusplus
}
#endif

