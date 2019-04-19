#ifdef __cplusplus
extern "C" {
#endif

#ifndef REF_PROC_PUB_H
#define REF_PROC_PUB_H

/* 
**************************************************************************
 (C) Copyright 2015 Signal Essence; All Rights Reserved
  
  Module Name  - ref_proc
                 Collect all reference signal processing in a single module
  
  Author: Robert Yu
  
  History
  2015-11-20  ryu  created 
  
**************************************************************************
*/

#include "multichan_delay.h"

typedef struct 
{

    int16 PeakHoldDecayCoef;
    int32 numChans;
    int32 blockSize;
    float32 hpfCutoffHz;
    int32 numCutoffPoles;
    int32 sampleRateHz;

    // numDelaySamples will be copied into the multiChanDelayConfig during init
    // so you don't have to dig through this config struct just to set the multichannel delay parameter
    int32 numDelaySamples;   
    MultiChanDelayConfig_t multiChanDelayConfig;
} RefProcConfig_t;

#endif //ifndef

#ifdef __cplusplus
} // cplusplus
#endif
