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

#define DECLARE_SHAVE_EMULATE_FUNCTION(functionName, parameters) void emulate_ ## functionName parameters ;

// 1. Access to the local buffers for each of the valid Shaves
// 2. If the shave is being used, add a function declaration macro
#ifdef USE_SHAVE_0
extern char shave0_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#define DECLARE_SHAVE_0_FUNCTION(functionName, parameters) void shave0_ ## functionName parameters ;
#else
#define DECLARE_SHAVE_0_FUNCTION(functionName, parameters)
#endif

#ifdef USE_SHAVE_1
extern char shave1_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#define DECLARE_SHAVE_1_FUNCTION(functionName, parameters) void shave1_ ## functionName parameters ;
#else
#define DECLARE_SHAVE_1_FUNCTION(functionName, parameters)
#endif

#ifdef USE_SHAVE_2
extern char shave2_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#define DECLARE_SHAVE_2_FUNCTION(functionName, parameters) void shave2_ ## functionName parameters ;
#else
#define DECLARE_SHAVE_2_FUNCTION(functionName, parameters)
#endif

#ifdef USE_SHAVE_3
extern char shave3_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#define DECLARE_SHAVE_3_FUNCTION(functionName, parameters) void shave3_ ## functionName parameters ;
#else
#define DECLARE_SHAVE_3_FUNCTION(functionName, parameters)
#endif

#ifdef USE_SHAVE_4
extern char shave4_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#define DECLARE_SHAVE_4_FUNCTION(functionName, parameters) void shave4_ ## functionName parameters ;
#else
#define DECLARE_SHAVE_4_FUNCTION(functionName, parameters)
#endif

#ifdef USE_SHAVE_5
extern char shave5_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#define DECLARE_SHAVE_5_FUNCTION(functionName, parameters) void shave5_ ## functionName parameters ;
#else
#define DECLARE_SHAVE_5_FUNCTION(functionName, parameters)
#endif

#ifdef USE_SHAVE_6
extern char shave6_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#define DECLARE_SHAVE_6_FUNCTION(functionName, parameters) void shave6_ ## functionName parameters ;
#else
#define DECLARE_SHAVE_6_FUNCTION(functionName, parameters)
#endif

#ifdef USE_SHAVE_7
extern char shave7_localBuffer[LOCAL_SHAVE_BUFFER_SIZE];
#define DECLARE_SHAVE_7_FUNCTION(functionName, parameters) void shave7_ ## functionName parameters ;
#else
#define DECLARE_SHAVE_7_FUNCTION(functionName, parameters)
#endif

#define DECLARE_SHAVE_FUNCTION(functionName, parameters) \
  DECLARE_SHAVE_EMULATE_FUNCTION(functionName, parameters) \
  DECLARE_SHAVE_0_FUNCTION(functionName, parameters) \
  DECLARE_SHAVE_1_FUNCTION(functionName, parameters) \
  DECLARE_SHAVE_2_FUNCTION(functionName, parameters) \
  DECLARE_SHAVE_3_FUNCTION(functionName, parameters) \
  DECLARE_SHAVE_4_FUNCTION(functionName, parameters) \
  DECLARE_SHAVE_5_FUNCTION(functionName, parameters) \
  DECLARE_SHAVE_6_FUNCTION(functionName, parameters) \
  DECLARE_SHAVE_7_FUNCTION(functionName, parameters)

// Run the shave kernel functionName on a given shave number
#define START_SHAVE(shaveNumber, functionName) swcStartShave(shaveNumber, (u32)&shave ## shaveNumber ## _ ## functionName)
#define START_SHAVE_WITH_ARGUMENTS(shaveNumber, functionName, formatString, ...) swcStartShaveCC(shaveNumber, (u32)&shave ## shaveNumber ## _ ## functionName, formatString, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif
  DECLARE_SHAVE_FUNCTION(AddVectors_s32x4,
  (
    const s32 * restrict pIn1,
    const s32 * restrict pIn2,
    s32 * restrict pOut,
    const s32 numElements
    ));

  DECLARE_SHAVE_FUNCTION(PrintTest,
  ());
#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_SHAVE_KERNELS_H_