#ifdef __cplusplus
extern "C" {
#endif

#include "se_types.h"

#ifndef _LIMIT_INT_ARM_H
#define _LIMIT_INT_ARM_H

#include "se_types.h"

static inline int16 LimitInt32ToInt16_ARM( int32 x )   
{
	int16 x16;
	asm("    ssat  %[x16], #16, %[x]"
	    : [x16]"=r" (x16)
	    : [x]  "r"  (x)
	    :);
	return x16;
}


typedef union {
    int64 i64;
    int32 i32[2];
} int32_int64_union_t;

static inline int32 LimitInt64ToInt32_ARM( int64  x )   
{
    int32_int64_union_t y;
    y.i64 = x;
    y.i64 <<= 1;
    if( y.i32[1] > 0)
        return SE_MAX_INT32;
    if (y.i32[1] < -1)
        return SE_MIN_INT32;
    return (int32)x;
}
#endif
#ifdef __cplusplus
}
#endif

