#ifdef __cplusplus
extern "C" {
#endif

/* 
(C) Copyright 2016 Signal Essence; All Rights Reserved
  
  Module Name  - sampledelayqueue_f32.h
*/

#ifndef __sampledelayqueuef32_h
#define __sampledelayqueuef32_h

#include "se_types.h"

/* definitions */
typedef struct {
    float32   *BaseAddress;
    int32   BufferSize;           // aka size of the allocated array

    uint16 HasReceivedInitialWrite;  // queue has received initial write
    int32 InitialDelay;   // initial delay
    int32 NSamplesInQueue;
    int32 NWrites; // number of write requests
    int32 NReads;  
    int32 NUnderflowReadsPreInit; // number of reads which precede initial write
    int32 NUnderflowReadsPostInit; // number of underflow reads (post init write)
    int32 NWriteIncrReadDecr; // counter:  write increments, read decrements
} SampleDelayQueueF32_t;

/* Function Prototypes */

void InitSampleDelayQueue_f32(SampleDelayQueueF32_t *sp, 
                          float32 *bufAddr, 
                          int32 bufferSize,  // size of the allocated array
                          int32 initialDelay,
                          char *pSeDiagPrefix);

int32 GetNumSamplesDelay_f32(SampleDelayQueueF32_t *rp);

//void WriteSampleDelayQueue( 
void SampleDelayWrite_f32(SampleDelayQueueF32_t *rp, 
                          const float32 *inbufptr,
                          int32 numSampsIn);

//void ReadSampleDelayQueue( 
int32 SampleDelayPeek_f32(SampleDelayQueueF32_t *rp, 
                          float32 *outbufptr,
                          int32 numSamples);

int32 SampleDelayDequeue_f32(SampleDelayQueueF32_t *sp,
                             float32 *outBufPtr,
                             int32 numSamples);

uint16 GetSampleDelayHasReceivedInitialWrite_f32(SampleDelayQueueF32_t *rp);

int32 GetSampleDelayNUnderflowReadsPreInit_f32(SampleDelayQueueF32_t *rp);

int32 GetSampleDelayNUnderflowReadsPostInit_f32(SampleDelayQueueF32_t *rp);

int32 GetNumSamplesInQueue_f32(SampleDelayQueueF32_t *rp);

void ClearSampleDelayQueue_f32(SampleDelayQueueF32_t *sp);
#endif
#ifdef __cplusplus
}
#endif

