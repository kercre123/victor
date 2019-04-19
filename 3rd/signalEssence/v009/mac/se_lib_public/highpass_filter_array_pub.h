#ifdef __cplusplus
extern "C" {
#endif

#ifndef HPFA_PUB_H
#define HPFA_PUB_H

#include "se_types.h"

/* 
**************************************************************************
(C) Copyright 2015 Signal Essence; All Rights Reserved
  
  Highpass Filter Array
  
  Author: Robert Yu
  
  History

  2015-09-17  created
**************************************************************************
*/


#define HPFA_LEN_SEDIAG_PREFIX 128

typedef struct {
    int32                numPoles;
    float32              cutoffHz;
    int32                numChans;
    float32              sampleRatekHz;
    int32                blockSize;
    char                 pSediagPrefix[HPFA_LEN_SEDIAG_PREFIX];
} HighpassFilterArrayConfig_t;


#endif // HPFA_PUB_H

#ifdef __cplusplus
}
#endif

