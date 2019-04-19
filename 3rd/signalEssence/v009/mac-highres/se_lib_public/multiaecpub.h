#ifdef __cplusplus
extern "C" {
#endif

/* 
**************************************************************************
(C) Copyright 2009 Signal Essence; All Rights Reserved
  
  Module Name  - _mmfx.h
  
  Author: Hugh McLaughlin
  
  History
  
  Mar11,09    hjm  created
  2010-02-09  ryu  incorporate aec
  2011-11-29  ryu  split api into private/public header files
  
  
**************************************************************************
*/

#ifndef ___se_multiaec_pub_h
#define ___se_multiaec_pub_h

#include "se_types.h"
#include "meta_aec_pub.h"
#include "aec_td_pub.h"

typedef struct 
{
    int NumMics;
    uint16 BlockSize;
    uint16 SampleRateHz;
    uint16 pBypassCancelPerAec[MAX_MICS];
    uint16 pBypassUpdatePerAec[MAX_MICS];

    MetaAecConfig_t MetaAecConfig;

#ifdef SE_THREADED_AEC
    // only used when multi-threaded.
	int ScratchPoolSBytes; 
	int ScratchPoolHBytes;
	int ScratchPoolXBytes;
    int NumThreads;
#endif
} MultiAecConfig_t;

void MultiAecSetDefaultConfig(MultiAecConfig_t *pConfig,
                              uint16 blockSize,
                              uint16 numMics,
                              uint16 sampleRateHz);

//
// get implementation specific aec config object;
// you need to recast the returned pointer into the appropriate
// AEC config type
void *MultiAecGetConfig(MultiAecConfig_t *pConfig, MetaAecImplementation_t type);


#endif //___se_multiaec_pub_h
#ifdef __cplusplus
}
#endif

