#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*-----------------------------------------------------------
 (C) Copyright 2010 Signal Essence LLC; All Rights Reserved 

Module Name  - dcremove.h

Author: Hugh McLaughlin

History
   04-21-09       hjm    created

Description

Machine/Compiler: ANSI C
-------------------------------------------------------------*/

#ifndef __dcremove_h
#define __dcremove_h

#include "se_types.h"

typedef struct {
    float32 SampleRatekHz;
    float32 FCutHz;
    float32 XPrevState;
    float32 State;
} DCRmv_t;

/* Function Prototypes */
void  InitDcRemovalFilter( DCRmv_t *dp, float fcut_Hz, float sampleRate_kHz);
float32 DcRemovalSetCutoffHz(DCRmv_t *dp, float32 fcut_Hz);
void DcRemovalFilter( DCRmv_t *dp, const int16 *inptr, int16 *outptr, int32 N );


#endif // ifndef
#ifdef __cplusplus
}
#endif

