
/* 
**************************************************************************
(C) Copyright 2012 Signal Essence; All Rights Reserved
  
  Module Name  - system_tests_pub.h
  
  
**************************************************************************
*/
#ifndef _SYSTEM_TESTS_PUB_H_
#define _SYSTEM_TESTS_PUB_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "se_types.h"

typedef struct
{
    int32 sampleRatekHz;

    int32 nbngStartBand;
    int32 nbngEndBand;
    int16 nbngFinalGain_q10;
    int32 nbngNumBlocksToAnalyze;

    uint16 sineMagnitude;
    float32 sineFreqHz;
} SystemTestsConfig_t;

void SystemTests_SetDefaults(SystemTestsConfig_t *pConfig,
                             int32 sampleRatekHz);

#ifdef __cplusplus
}
#endif
#endif
