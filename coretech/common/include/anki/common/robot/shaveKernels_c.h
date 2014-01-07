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

#define LOCAL_SHAVE_BUFFER_SIZE 50000

#include "anki/common/robot/config.h"

// Access to the local buffers for each of the valid Shaves
#ifdef USE_SHAVE_0
extern char shave0_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#endif

#ifdef USE_SHAVE_1
extern char shave1_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#endif

#ifdef USE_SHAVE_2
extern char shave2_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#endif

#ifdef USE_SHAVE_3
extern char shave3_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#endif

#ifdef USE_SHAVE_4
extern char shave4_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#endif

#ifdef USE_SHAVE_5
extern char shave5_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#endif

#ifdef USE_SHAVE_6
extern char shave6_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#endif

#ifdef USE_SHAVE_7
extern char shave7_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#endif

// Run the shave kernel functionName on a given shave number
#define START_SHAVE(shaveNumber, functionName, ...) swcStartShaveCC(shaveNumber, (u32)&shave ## shaveNumber ## _ ## functionName, __VA_ARGS__)

#pragma mark --- Natural C declarations ---

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

#pragma mark --- Per-Shave C declarations ---
// TODO: Does this have to be done by hand?

#ifdef __cplusplus
extern "C" {
#endif
#ifdef USE_SHAVE_0
  void shave0_addVectors_s32x4(
    const s32 * restrict pIn1,
    const s32 * restrict pIn2,
    s32 * restrict pOut,
    const s32 numElements
    );
#endif

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_SHAVE_KERNELS_H_