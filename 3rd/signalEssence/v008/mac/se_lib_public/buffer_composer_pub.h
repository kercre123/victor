/***************************************************************************
   (C) Copyright 2016 SignalEssence; All Rights Reserved

    Module Name: buffer_composer

    Description:
    Compose multi-channel output signal from multiple, multi-channel inputs

    Author: Robert Yu

**************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
#include "se_types.h"

#ifndef BUFFER_COMPOSER_PUB
#define BUFFER_COMPOSER_PUB

#define BC_MAX_NUM_CHANS 10

typedef enum 
{
    BC_FIRST_DONT_USE,
    BC_SENR_OUT,         // select block from SENR output
    BC_SPATFILT_NOISE_REF,      // select block from spatial filter noise reference
    BC_LAST                 
} BufferCompSrc_t;

typedef struct
{
    int32 blockSize;
    float32 sampleRateHz;
    int32 numOutputChans;  // total number of channels in output buffer

    // Channel mapping:
    // The Nth source and channel index specifies what samples are to be copied into
    // the output buffer.
    // e.g.  0: (SENR_OUT, 0_
    //       1: (SPATFILT_NOISE_REF, 0)
    //
    // the final sout buffer will contain
    // chan 0:  senr output chan 0
    // chan 1:  spatial filter noise ref chan 0
    BufferCompSrc_t pSource[BC_MAX_NUM_CHANS];
    int32           pSourceIndex[BC_MAX_NUM_CHANS];
} BufferCompConfig_t;


void SoutSetDefaultConfig(BufferCompConfig_t *pConfig, int blockSize, float32 sampleRateHz);
void SoutSetNumChans(BufferCompConfig_t *pConfig, int numChans);
void SoutSetChanSource(BufferCompConfig_t *pConfig, int outChanIndex, BufferCompSrc_t source, int sourceIndex);

#endif // buffer_composer

#ifdef __cplusplus
}
#endif
