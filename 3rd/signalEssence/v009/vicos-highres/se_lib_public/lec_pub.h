#ifdef __cplusplus
extern "C" {
#endif

/*
************************************************************************** 
  (C) Copyright 2014 Signal Essence; All Rights Reserved
  
  Module Name  - line echo canceller (lec)
  
  Author: Robert Yu
  
  History
  2014-05-14  ryu
**************************************************************************
*/
#ifndef LEC_PUB_H
#define LEC_PUB_H

#include "se_types.h"
#include "aecmonitorpub.h"
#include "se_nr_pub.h"
#include "meta_aec_pub.h"


//
// LEC-specific parameters
typedef struct 
{
    int32 blockSize;
    int32 sampleRate_kHz;

    int32 numDelaySamps;
    int32 maxLenSoutDelayLine;
} LecParams_t;

//
// nested configuration structures for LEC components
typedef struct
{
    LecParams_t selfConfig;
    int32 lenChanModel;
    AecMonitorConfig_t aecMonitorConfig;
    SenrConfig_t senrConfig;
    MetaAecConfig_t aecConfig;
} LineEchoCancellerConfig_t;



void LecSetDefaultConfig(LineEchoCancellerConfig_t *pConfig,
                         int32 sampleRate_kHz,
                         int32 blockSize);

#endif // LEC_PUB_H

#ifdef __cplusplus
}
#endif
