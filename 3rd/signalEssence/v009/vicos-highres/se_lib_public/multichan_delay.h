#ifdef __cplusplus
extern "C" {
#endif
/* 
**************************************************************************
 (C) Copyright 2015 Signal Essence; All Rights Reserved
  
  Module Name  - multichan_delay
  
  Author: Robert Yu
  
  History
  2015-11-20  ryu  created to implement multichannel reference delay line  
  
**************************************************************************
*/

#ifndef MULTICHAN_DELAY_H
#define MULTICHAN_DELAY_H
#include "multichan_delay.h"
#include "sampledelayqueue.h"

typedef struct {
    int32 numChans;
    int32 numDelaySamples;
    int32 blockSize;
} MultiChanDelayConfig_t ;

typedef struct {
    MultiChanDelayConfig_t config;
    SampleDelayQueue_t *pDelayQueuePerChan;
    int16 **ppDelayBufPerChan;
} MultiChanDelay_t;

void McdSetDefaultConfig(MultiChanDelayConfig_t *pConfig, int32 blockSize);
void McdInit(MultiChanDelay_t *p, MultiChanDelayConfig_t *pConfig);
void McdDestroy(MultiChanDelay_t *p);
void McdWrite(MultiChanDelay_t *p, const int16 *pIn);
void McdDequeue(MultiChanDelay_t *p, int16 *pOut);


#endif //MULTICHAN_DELAY_H

#ifdef __cplusplus
}
#endif
