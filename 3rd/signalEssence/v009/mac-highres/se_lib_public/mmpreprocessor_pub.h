#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2010 Signal Essence LLC; All Rights Reserved 

Module Name  - mmpreprocessor.h

Author: Hugh McLaughlin

History

   09-18-10       hjm    created

Description

Machine/Compiler: ANSI C
-------------------------------------------------------------*/

#ifndef __mmpreprocessor_pub_h
#define __mmpreprocessor_pub_h

#include "mmglobalsizes.h"
#include "se_types.h"
#include "frdelay_pub.h"

//
// preprocessor can up to N single-pole DC removal filters
#define MMP_MAX_NUM_DC_RMV_POLES  10

/* Data Structure Definitions */
typedef struct {
    int32  NumMics;
    int32  NumSinChannels;
    int32  BlockSize;
    
    float32   CutoffHz;
    int32    SampleRate_kHz;

    int32 NumDelaySamples;
    
    //
    // you can configure the number of single-pole DC removal filters applied to the input signal (up to MMP_MAX_NUM_POLES).
    // this feature allows you to emulate the "c67 highpass filter" feature formerly implemented in the
    // mmif layer.
    int32  NumDcRmvPoles;

    //
    // channel re-mapping
    //
    // pChannelRemapSinSourceIndex[3] = 6
    // means microphone 3 is sourced from Sin channel 6.
    // 
    int32 pChannelRemapSinSourceIndex[MAX_MICS];

    //
    // per-mic gain, applied AFTER remapping
    float32 GSinPerMic[MAX_MICS];

    FrDelayConfig_t FrDelayConfig;
} MMPreProcConfig_t;

/* Function Prototypes */
void MMPreProcessorSetDefaultConfig(MMPreProcConfig_t *cp,
                                    int32 numMics,
                                    int32 blockSize,
                                    int32 sampleRate_kHz);  // in kHz


#endif

#ifdef __cplusplus
}
#endif

