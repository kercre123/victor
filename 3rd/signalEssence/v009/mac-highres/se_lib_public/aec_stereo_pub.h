#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   (C) Copyright 2013 SignalEssence; All Rights Reserved

    Module Name: aec_pbfd

    Author: Robert Yu

    Description:
    Partitioned-block frequency domain adaptive echo canceller

    History:
    2013-04-25  ryu

    Machine/Compiler:
    (ANSI C)
**************************************************************************/
#ifndef AEC_STEREO_PUB_H_
#define AEC_STEREO_PUB_H_

#include "se_types.h"

typedef enum {
    AECST_FIRST_DONT_USE,
    AECST_SUM_DIFF,        // EC0 = (ref0 + ref1), EC1 = (ref0 - ref1)
    AECST_LEFT_RIGHT,      // EC0 = ref0, EC1 = ref1
    AECST_LAST
} AecStCancelStrategy_t;
    

//
// configuration parameters
typedef struct
{
    int32 blockSize;       // input block size
    int32 lenChanModel;    // length of channel model
    int32 sampleRate_Hz;


    //
    // channel model profile
    int32 profileFlatDurationIndex;
    float32 profileDecayDbPerMsec;
    float32 profileMinGainDb;

    int32  sinScalingRshifts;  // number of rshifts to apply to SIn

    float32 refPowerBinCorrectionDb;

    float32 muPerBlock; // stepsize per block (used to compute stepsize per update)
    float32 maxMuPerUpdate; // maximum stepsize per update

    uint16 bypassBackupChanModel; // 1 = don't use backup channel model for echo cancellation

    int32 numMics;   // number of mics; for staggered update scheduling
                      // set to 0 for default (see AecStInit)
    //
    // echo-canceller parameters
    int32 ecLenDft;

    AecStCancelStrategy_t cancelStrategy;

    // 0x01 = enable first canceller
    // 0x02 = enable 2nd canceller
    // 0x03 = enable both cancellers
    uint32 enableCancellerMask;

    // length of step size modifier transition
    float32 lenStepSizeTransitionPercent;
} AecStConfig_t;

//
// configure AEC
void AecStSetDefaultConfig(AecStConfig_t* pConfig,
                             int32 blockSize,
                             int32 lenChanModel,
                             int32 sampleRate_Hz);


#endif // 
#ifdef __cplusplus
}
#endif

