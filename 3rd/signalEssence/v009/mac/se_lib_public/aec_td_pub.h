#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   (C) Copyright 2009 SignalEssence; All Rights Reserved

    Module Name: aec

    Author: Robert Yu

    Description:
    acoustic echo canceller

    History:
    2010-01-11 ryu First edit
    2010-06-03 ryu add backup filter
    2011-11-29 ryu separate header file into private/public

    Machine/Compiler:
    (ANSI C)
**************************************************************************/
#ifndef AEC_PUB_H_
#define AEC_PUB_H_

#include "se_types.h"
#include "mmglobalsizes.h"

//
// configuration parameters
typedef struct
{
    int32 blockSize;       // input block size
    int32  sampleRate_Hz; // sample rate
    int32 lenChanModel;    // length of channel model
    uint16  stepSizeAdjust_q10;  // step size adjustment

    
    //
    // channel model profile weights, normal time order (i.e. not reversed)
    int16  pProfileWts[AEC_MAX_LEN_CHAN_MODEL];
} AecTdConfig_t;

//
// the fields in the public structure are collected together
// because they're generally useful for viewing and 
// debugging
//
// fields are marked
// read-write (RW) and read-only (RO)
//
// TODO: move RW enable flags to config object
typedef struct
{
    // performance metrics
    //////////////////////////////////////
    // set enableDebugPerfMetrics=1 to enable perf metric computation
    //
    int16 enableDebugPerfMetricsRW;
    uint32 magInRO;  // sum magnitude of input block (not ref)
    uint32 magOutRO; // sum magnitude output block
    uint32 pMagChanModelRO[2]; // sum magnitude channel model (two channel models)

    //
    // misc counts and values
    /////////////////////////////////////////////
    uint16 numAdaptationsRO; // number of times channel model has been updated
    uint16 numResetsRO; // number of times channel model has been zeroed
    uint16 latestStepSizeModifierRO; // stepSizeModifier specified in latest update call



    // filter mirroring for debug
    ///////////////////////////////////////////
    // set to 1 to enable
    // mirroring current filter weights * profile weights
    // to debug buffer
    uint16 enableCopyWeightedFilterOnCancelRW;
    int16 pWeightedFilter16CopyRO[AEC_MAX_LEN_CHAN_MODEL];
    int16 pWeightedFilterBackup16CopyRO[AEC_MAX_LEN_CHAN_MODEL];

    //
    // copy of channel model pointers
    int32 *(ppChanModel32RW[2]);   // ppChanModel32 is an array[2] of pointers to int32


    // filter bank control
    //////////////////////////
    // allow AEC to choose between alternate channel models during cancellation
    uint16 bypassBackupChanModel; //enablePickBestChanModelRW;  

    //
    // how many times has AecTdPickBestChanModel picked the non-active channel model?
    uint32 numUseBackupChanModelRO;
           
    //
    // debug for channel update calc
    uint32 onUpdateRefAvgPwrRO;

    //
    // location of peak channel model response (units = samples)
    int16 chanModelPeakDelay;
} AecTdPublicObject_t;

//
// see aec.c for function documentation
void AecTdSetDefaultConfig(AecTdConfig_t* pConfig,
                           int32 blockSize,
                           int32 lenChanModel,
                           int32 sampleRateHz);




#endif // AEC_PUB_H_
#ifdef __cplusplus
}
#endif

