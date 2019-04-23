#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   (C) Copyright 2017 SignalEssence; All Rights Reserved

    Module Name: aec_msu

    Author: Robert Yu

    Description:
    Partitioned-block freq domain AEC, multi-stage update (MSU)

    History:
    2017-01-31 
**************************************************************************/
#ifndef AEC_MSU_PUB_H_
#define AEC_MSU_PUB_H_

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
} AecMsuConfig_t;

//
// configure AEC
void AecMsuSetDefaultConfig(AecMsuConfig_t* pConfig,
                             int32 blockSize,
                             int32 lenChanModel,
                             int32 sampleRate_Hz);


#endif // 
#ifdef __cplusplus
}
#endif

