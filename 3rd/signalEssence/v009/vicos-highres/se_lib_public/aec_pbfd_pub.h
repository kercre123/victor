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
#ifndef AEC_PBFD_PUB_H_
#define AEC_PBFD_PUB_H_

#include "se_types.h"

//
// configuration parameters
typedef struct
{
    int32 blockSize;       // input block size
    int32 lenChanModel;    // length of channel model
    int32 sampleRate_Hz;


    //
    // channel model profile
    int profileFlatDurationIndex;
    float32 profileDecayDbPerMsec;
    float32 profileMinGainDb;

    uint16  sinScalingRshifts;  // number of rshifts to apply to SIn

    float32 refPowerBinCorrectionDb; // correction applied to per-bin power during adaptation

    float32 muPerBlock; // stepsize per block (used to compute stepsize per update)
    float32 maxMuPerUpdate; // maximum stepsize per update

    uint16 bypassBackupChanModel; // 1 = don't use backup channel model for echo cancellation

    int32 numMics;   // number of mics; for staggered update scheduling
                      // set to 0 for default (see AecPbfdInit)
    //
    // echo-canceller parameters
    int ecLenDft;

    // length of step size modifier transition
    float32 lenStepSizeTransitionPercent;
} AecPbfdConfig_t;

//
// configure AEC
void AecPbfdSetDefaultConfig(AecPbfdConfig_t* pConfig,
                             int32 blockSize,
                             int32 lenChanModel,
                             int32 sampleRate_Hz);


#endif // 
#ifdef __cplusplus
}
#endif

