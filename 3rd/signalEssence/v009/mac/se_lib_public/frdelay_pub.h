#ifdef __cplusplus
extern "C" {
#endif

/* 
-------------------------------------------------------------
(C) Copyright 2017 Signal Essence; All Rights Reserved

frdelay - fractional delay
-------------------------------------------------------------
*/

#ifndef __frdelay_pub_h
#define __frdelay_pub_h
#include "se_types.h"
#include "mmglobalsizes.h"

typedef struct
{
    int32 numMics;
    int32 blockSize;
    int32 numPhases;   // number of subsample phases; specify 0 to disable fractional delay
    int32 pDesiredPhasePerMic[MAX_MICS];
} FrDelayConfig_t;


void FrDelaySetDefaultConfig(FrDelayConfig_t *pConfig,
                             int numMics,
                             int blockSize);

#endif

#ifdef __cplusplus
}
#endif

