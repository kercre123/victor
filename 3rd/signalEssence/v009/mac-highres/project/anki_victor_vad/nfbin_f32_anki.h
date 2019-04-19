#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2017 Signal Essence customized for Anki; All Rights Reserved

  Module Name  - nfbin_i16.h

  Author: Hugh McLaughlin

  Description:

      Noise Floor Estimator.  

  History:    hjm - Hugh McLaughlin 

  12-15-17    hjm  created


  Machine/Compiler: ANSI C

--------------------------------------------------------------------------*/

#ifndef __nfbin_f32_h
#define __nfbin_f32_h

#define FD_MAX_GROUPED_BINS  65

#include "se_types.h"

// !!! note -- hjm 1-1-2012 -- nfBinStatePtr array takes up a lot of room for the 
// fdsearch cuz the required number of subbins is much less than FD_MAX_GROUPED_BINS
// perhaps the array should be allocated by the calling function which knows how
// many subbands/bins to request

typedef struct {
    float    KDown;
    float    KFloatUp;
    float    InitNf;
    float    MaxNf;
    float    MinNf;
    int      NumBins;
    float    NfBinState[FD_MAX_GROUPED_BINS]; 
} NfBin_f32_t;

void InitNoiseFloorPerBin_f32_anki(
        NfBin_f32_t *np,
        float downStepSize,
        float floatUpRate,
        float initNf,
        float minNf,
        float maxNf,
        int numBins );


void ComputeNoiseFloorPerBin_f32_anki( NfBin_f32_t *np,             // structure pointer to object
                                  float conf,                  // confidence in measuring a higher value
                                  float *xPwrPtr,              // signal power vector
                                  float *nfBinPtr );           // estimated noise floor

#endif
#ifdef __cplusplus
}
#endif

