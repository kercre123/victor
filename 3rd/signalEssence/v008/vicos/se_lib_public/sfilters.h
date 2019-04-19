#ifdef __cplusplus
extern "C" {
#endif

/*
(C) Copyright 2009 Signal Essence LLC; All Rights Reserved

Module Name  - sfilters.h

Author: Hugh McLaughlin

History

   Dec07,09      hjm    created

Description

    simple/scalar filter functions

Machine/Compiler: ANSI C

-------------------------------------------------------------*/

#ifndef __sfilters_h
#define __sfilters_h

#include "se_types.h"

/* definitions */

/* data structures */

/* Function Prototypes */

void LeakyAverage_i16( int16 *yPtr, int16 x, int16 alpha );

void LeakyAverage_i32( int32 *yPtr, int32 x, int16 alpha );

void LeakyAverage_f32( float32 *yPtr, float32 x, float32 alpha );

void TrackUpLeakDown_i16( int16 *yPtr,     // state
                          int16 x,         // input value
                          int16 alpha );    // coefficient

void TrackDownLeakUp_i16( int16 *yPtr,     // state
                          int16 x,         // input value
                          int16 alpha );    // coefficient

void TrackUpLeakDown_i32( int32 *yPtr,     // state
                          int32 x,         // input value
                          int16 alpha );   // coefficient

void TrackUpLeakDown_f32( float32 *yPtr,     // state
                          float32 x,         // input value
                          float32 alpha );    // coefficient

void TrackDownLeakUp_f32( float32 *yPtr,     // state
                          float32 x,         // input value
                          float32 alpha );    // coefficient

float32 MinMax_f32( float32 input, float32 min, float32 max );
#endif

#ifdef __cplusplus
}
#endif

