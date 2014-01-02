///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief
///

#ifndef __MYRIAD_MODEL__
#define __MYRIAD_MODEL__

// 1: Includes
// ----------------------------------------------------------------------------
#include "svuCommonShaveDefines.h"
#include "mv_types.h"
#include "stdio.h"
#include "swcDmaTypes.h"

//Global defines
#define CMX_DATA_START_ADR 0x1C000000
#define DDR_DATA_START_ADR 0x48000000


// Functions to creat and destroy myriad model
void mmCreate();
void mmDestroy();

// Functions to insert and extract data from the DDR/CMX models
void mmSetMemcpy(u32 addr, void* src, u32 sz_bytes);
void mmGetMemcpy(void* dst, u32 addr, u32 sz_bytes);

// Utitlity function to convert a CMX/DDR pointer to an action PC memory pointer
u8*   mmGetPtr(u32 addr);

void  mmInitHeap(u32 cmx_heap_addr);
void* mmMalloc(u32 sz_bytes);


// Remainder of this header file is overloaded implementation of shave functions 
// defined in svuCommonShave.h
// ----------------------------------------------------------------------------

/// Simple DMA setup
/// @param[in] - taskNum - task number
/// @param[in] - src - source for task
/// @param[in] - dst - destination for task
/// @param[in] - numBytes - length in bytes
/// @return    - void
///
void scDmaSetup(swcDmaTask_t taskNum, unsigned char *src, unsigned char *dst, unsigned int numBytes);

/// Simple DMA setup with destination stride
/// @param[in] - taskNum - task number
/// @param[in] - src - source for task
/// @param[in] - dst - destination for task
/// @param[in] - numBytes - length in bytes
/// @param[in] - width
/// @param[in] - stride
/// @return    - void
///
void scDmaSetupStrideDst(unsigned int taskNum, u8* src, u8* dst, unsigned int numBytes, unsigned int width, unsigned int stride);

/// Simple DMA setup with source stride
/// @param[in] - taskNum - task number
/// @param[in] - src - source for task
/// @param[in] - dst - destination for task
/// @param[in] - numBytes - length in bytes
/// @param[in] - width
/// @param[in] - stride
/// @return    - void
///
void scDmaSetupStrideSrc(unsigned int taskNum, u8* src, u8* dst, unsigned int numBytes, unsigned int width, unsigned int stride);

//l Full DMA setup
/// @param[in] - taskNum - task number
/// @param[in] - cfg - DMA configuration
/// @param[in] - src - source for task
/// @param[in] - dst - destination for task
/// @param[in] - numBytes - length in bytes
/// @param[in] - width
/// @param[in] - stride
/// @return    - void
///
void scDmaSetupFull(unsigned int taskNum, unsigned int cfg, unsigned char *src, unsigned char *dst, unsigned int numBytes, unsigned int width, unsigned int stride);

/// This will start DMA task number taskNum
/// @param[in] - taskNum - task number
/// @return    - void
///
void scDmaStart(unsigned int taskNum);

/// This will wait that DMA finishes task number taskNum
/// @param[in] - void
/// @return    - void
///
void scDmaWaitFinished(void);


/// Blocking DMA Copy
/// @param[in] - dst - destination for copy
/// @param[in] - src - source for copy
/// @param[in] - numBytes - length in bytes for copy
/// @return    - void
///
void scDmaCopyBlocking(unsigned char *dst, unsigned char *src, u32 numBytes);

/// Mutex Request
/// @param[in] - mutexNum - mutex number requested
/// @return    - void
///
void scMutexRequest(unsigned int mutexNum);

/// This will release mutex
/// @param[in] - mutexNum - mutex number that will be released
/// @return    - void
///
void scMutexRelease(unsigned int mutexNum);

#endif
