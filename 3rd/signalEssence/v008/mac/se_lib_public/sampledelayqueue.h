#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2009 Signal Essence; All Rights Reserved
  
  Module Name  - sampledelayqueue.h
  
  Author: Hugh McLaughlin
  
  History
  
     Dec03,09    hjm    created

*/

#ifndef __sampledelayqueue_h
#define __sampledelayqueue_h

#include "se_types.h"

/* definitions */
typedef struct {
    int16   *BaseAddress;
    int32   BufferSize;           // aka size of the allocated array

    uint16 HasReceivedInitialWrite;  // queue has received initial write
    uint32 InitialDelay;   // initial delay
    int32 NSamplesInQueue;
    uint32 NWrites; // number of write requests
    uint32 NReads;  
    uint32 NUnderflowReadsPreInit; // number of reads which precede initial write
    uint32 NUnderflowReadsPostInit; // number of underflow reads (post init write)
    int32  NWriteIncrReadDecr; // counter:  write increments, read decrements
} SampleDelayQueue_t;

/* Function Prototypes */

void InitSampleDelayQueue(SampleDelayQueue_t *sp, 
                          int16 *bufAddr, 
                          uint32 bufferSize,  // size of the allocated array
                          uint32 initialDelay,
                          char *pSeDiagPrefix);

uint32 GetNumSamplesDelay(SampleDelayQueue_t *rp);

//void WriteSampleDelayQueue( 
void SampleDelayWrite(SampleDelayQueue_t *rp, 
                      const int16 *inbufptr,
                      uint32 numSampsIn);

//void ReadSampleDelayQueue( 
int SampleDelayPeek(SampleDelayQueue_t *rp, 
                    int16 *outbufptr,
                    uint32 numSamples);

int SampleDelayDequeue(SampleDelayQueue_t *sp,
                       int16 *outBufPtr,
                       uint32 numSamples);

uint16 GetSampleDelayHasReceivedInitialWrite(SampleDelayQueue_t *rp);

uint32 GetSampleDelayNUnderflowReadsPreInit(SampleDelayQueue_t *rp);

uint32 GetSampleDelayNUnderflowReadsPostInit(SampleDelayQueue_t *rp);

int32 GetNumSamplesInQueue(SampleDelayQueue_t *rp);

void ClearSampleDelayQueue(SampleDelayQueue_t *sp);
#endif
#ifdef __cplusplus
}
#endif

