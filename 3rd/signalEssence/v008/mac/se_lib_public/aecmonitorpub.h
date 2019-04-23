#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2009 Avistar-Signal Essence LLC; All Rights Reserved 

Module Name  - aecmonitor.h

Author: Hugh McLaughlin

History

   04-22-09       hjm    created
   09-01-09       hjm    added clipping actions: added MicClippingDetected flag to AecMeasurement_t
   11-29-11       ryu    split header into private/public
Description

Machine/Compiler: ANSI C
-------------------------------------------------------------*/

#ifndef  aecpmc_pub_h__
#define  aecpmc_pub_h__

#include "mmglobalsizes.h"
#include "se_types.h"

#define AECMON_MAX_LEN_PREFIX 5

//
// configuration parameters
typedef struct
{
    float MicWeights[MAX_MICS]; 
    int32 NumMics;

   // rates of adjustment
   float    AlphaRefinTrk;
   float    AlphaNearTalkerC;

    // ERL measurement parameters
    float    ErlMinGainHeadset;
    float    ErlMaxGainHeadset;
    float    ErlMinGainSpeakerphone;
    float    ErlMaxGainSpeakerphone;
    float    ErlMinGainMeasurement;
    float    ErlMaxGainMeasurement;
    float    ErlStepSizeUp;
    float    ErlStepSizeDown;
    float    ErlInitialGain;

    // ERLE measurement parameters
    float    ErleMinGainOp;     // minGain for operational ERLE
    float    ErleMinGain;
    float    ErleMaxGain;
    float    ErleStepSizeUp;
    float    ErleStepSizeDown;
    float ErleInitialGain;

    //
    // Refin-related parameters
    float    RefinRmsMaxLevel;   // RmsMaxLevel sets the maximum output level (specified as Q15) at Rout
    float    RefinGainEstThresh;
    float    RefinMaxGain;
    float    RefinThresh;
    // for rcv path control
    float ExpectedTotalEchoGain;

    // fixed thresholds
    float    SinMinThresh;
    float    ClipMargin;
    float    ClipPowerThreshold;

    //
    // noise-floor measurement
    float NoiseFloorInitEst;
    float NoiseFloorMin;
    float NoiseFloorMax;
    
    float    UseBackupFactor;
    float    UpgradeBackupFactor;

    // sediag prefix
    char pSeDiagPrefix[AECMON_MAX_LEN_PREFIX];
}
 AecMonitorConfig_t;

void AecMonitorSetDefaultConfig(AecMonitorConfig_t *cfgp,
                                   int32 numMics);

#endif
#ifdef __cplusplus
}
#endif

