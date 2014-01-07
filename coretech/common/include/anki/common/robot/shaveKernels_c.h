/**
File: shaveKernels_c.h
Author: Peter Barnum
Created: 2013

C header for the SHAVE common kernels

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_SHAVE_KERNELS_H_
#define _ANKICORETECHEMBEDDED_COMMON_SHAVE_KERNELS_H_

#include "anki/common/robot/config.h"

#ifdef __cplusplus
extern "C" {
#endif

  void addVectors_s32x4(
    const s32 * restrict pIn1,
    const s32 * restrict pIn2,
    s32 * restrict pOut,
    const s32 numElements
    );

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_SHAVE_KERNELS_H_