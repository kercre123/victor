#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*-----------------------------------------------------------
 (C) Copyright 2010 Signal Essence LLC; All Rights Reserved 

Module Name  - dcremove.h

Author: Hugh McLaughlin

History
   04-21-09       hjm    created
   09-30-15       ryu    forked from fixed point version

Description

Machine/Compiler: ANSI C
-------------------------------------------------------------*/

#ifndef __dcremove_f32_h
#define __dcremove_f32_h

#include "se_types.h"

typedef struct {
    float32 SampleRatekHz;
    float32 FCutHz;
    float32 XPrevState;
    float32 State;
} DCRmvF32_t;

/* Function Prototypes */
void  InitDcRemovalFilter_f32( DCRmvF32_t *dp, float fcut_Hz, float sampleRate_kHz);
float32 DcRemovalSetCutoffHz_f32(DCRmvF32_t *dp, float32 fcut_Hz);
void DcRemovalFilter_f32( DCRmvF32_t *dp, const float32 *inptr, float32 *outptr, int N );


#endif // ifndef
#ifdef __cplusplus
}
#endif

