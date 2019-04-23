#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2017, Signal Essence for Anki
  
  Module Name  - svad.h
  
  Description:
     This module implements a simple voice activity detector

**************************************************************************
*/

#ifndef __svad_h
#define __svad_h

#include "dcremove_f32.h"
#include "nfbin_f32_anki.h"
#include "svad_pub.h"

// redefine to 480 if 48 kHz, keep small for now
#define MAX_VAD_BLOCK_SIZE 160     

typedef struct 
{
    // configuration parameters:  the SVadConfig object contains
    // parameters which are likely to change
    SVadConfig_t            Config;

    // constants derived from config
    float InvBlockSize;

    // objects
    DCRmvF32_t DCRmvObj[2];
    NfBin_f32_t NoiseFloorObj;

    // variables
    float   PreemphState;
    float   BlockPower;
    float   AvePowerInBlock;
    float   NoiseFloor;
    float   FourBlockMovingAverage;
    float   AvePowerInBlock_delay1;
    float   AvePowerInBlock_delay2;
    float   AvePowerInBlock_delay3;
    int     Activity;
    float   PowerTrk;
    float   ActivityRatio;   // ratio between power merit and max(noiseFloor, absThreshold)
    int     HangoverCountDown;

} SVadObject_t;


//
// initialize SVAD
void SVadInit( SVadObject_t *sp, SVadConfig_t *cp );

//
// re-load runtime parameters during runtime
// from configuration object
void SVadLoadConfigParams(const SVadConfig_t *cp, SVadObject_t *sp);

// 
int DoSVad(
    SVadObject_t *sp,
    float  nfestConfidence,           // noise floor measurement confidence, set to zero if gear noise, 1.0 otherwise
    int16 *pInput_q15);                // input ptr

#endif
#ifdef __cplusplus
}
#endif

