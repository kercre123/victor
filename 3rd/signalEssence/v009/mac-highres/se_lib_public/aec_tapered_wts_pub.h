#ifdef __cplusplus
extern "C" {
#endif

#ifndef AEC_TAPERED_WTS_PUB_H
#define AEC_TAPERED_WTS_PUB_H
#include "se_types.h"
/***************************************************************************
   (C) Copyright 2011 SignalEssence; All Rights Reserved

    Module Name: aec_tapered_wts

    Author: Robert Yu

    Description:
    public interface for aec tapered weights module

    History:
    2011-11-28 ryu split private-public interface
 
    Machine/Compiler:
    (ANSI C)
**************************************************************************/

void AecWeightsFlatThenExpDecay(float *pWeights,
                                int lenWeights,
                                float32 sampleRateHz,
                                int breakSampIndex, 
                                float decayDbPerMsec,
                                float32 minGainDb);


#endif
#ifdef __cplusplus
}
#endif

