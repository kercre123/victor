///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief
///

#ifndef __SVU_COMMON_SHAVE__
#define __SVU_COMMON_SHAVE__

// 1: Includes
// ----------------------------------------------------------------------------
#include <swcDmaTypes.h>
#include "svuCommonShaveDefines.h"
__asm(".include svuCommonDefinitions.incl");

// 2: Macros
// ----------------------------------------------------------------------------

//Switch u32 contents of variable
//@param[in] u32 variable to switch contents to
#define swcShaveSwapU32VarContents(var) __asm("SAU.SWZBYTE %0, %1, [0123]\n":"=r"(var):"r"(var))

//Switch u32 contents of variable and place results in a different variable
//@param[in]  u32 variable to switch contents
//@param[out] u32 variable to place switched contents into
#define swcShaveSwapU32VarsContents(varin,varout) __asm("SAU.SWZBYTE %0, %1, [0123]\n":"=r"(varout):"r"(varin))

/// Swaps endianness of a 32-bit integer (use this preferably on constants, not variables)
/// @param[in] value u32 integer to be swapped
/// @returns swapped integer
#define swcShaveSwapU32(value) \
        ((((u32)((value) & 0x000000FF)) << 24) | \
                ( ((u32)((value) & 0x0000FF00)) <<  8) | \
                ( ((u32)((value) & 0x00FF0000)) >>  8) | \
                ( ((u32)((value) & 0xFF000000)) >> 24))

///  Read 1 word from own FIFO
/// @param[in] - void
/// @return    - 1 word
///
#define scFifoRead() \
    int t; \
    asm ("lsu0.lda32 %0, SLICE_LOCAL, FIFO_RD_FRONT_L\n": "=r"(t): : );

// Write anything to clear own FIFO
/// @param[in] - void
/// @return    - void
///
#define scFifoOwnClear() \
    asm (\
        "lsu0.ldil i0, FIFO_CLEAR\n" \
        "lsu0.sta32 i0, SLICE_LOCAL, FIFO_CLEAR\n" : : : "i0");

/// This will release mutex
/// @param[in] - mutexNum -  mutex number that will be released
/// @return    - void
///
#define scMutexRelease(mutexNum) \
    asm (\
        "lsu1.ldil   i0, MUTEX_RELEASE_0\n" \
        "iau.or      i0, i0, %0\n" \
        "lsu0.sta32  i0, SLICE_LOCAL, MUTEX_CTRL_ADDR\n" : : "r"(mutexNum): "i0");

/// This will start DMA task number taskNum
/// @param[in] - taskNum - task number
/// @return    - void
#define scDmaStart(taskNum) asm ("LSU0.STA32 %0, SLICE_LOCAL, DMA_TASK_REQ\nNOP 5\n" : : "r"(taskNum): );

/// This will wait that DMA finishes task number taskNum
/// @param[in] - void
/// @return    - void
///
#define scDmaWaitFinished() \
    asm (\
    "LSU1.LDIL i0, DMA_BUSY_MASK\n" \
    "CMU.CMTI.BITP i0, P_GPI\n" \
    "PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i0, P_GPI\n" : : : "i0");

/// Get shave number
/// @param[in] - void
/// @return    - shave number
///
#define scGetShaveNumber() ({\
    int t; \
    asm ("cmu.cpti %0, P_SVID\n" : "=r"(t): : ); \
    t;})

/// Get TRF register value
/// @param[in] - regNum - TRF register
/// @return    - Value of TRF register
///
#define scGetTrfReg(regNum) ({\
    int t = 0x69; \
    asm (\
        "cmu.cpti i0, P_SVID  || iau.shl %1, %1, 2\n" \
        "lsu0.ldil i1, 0x1100 || lsu1.ldih i1, 0x8014   || iau.shl i0, i0, 16\n" \
        "iau.add i0, i0, %1\n" \
        "iau.add i0, i0, i1\n" \
        "lsu0.ld32 i0, i0\n"  \
        "nop 5\n" \
        "cmu.cpii %0, i0\n" \
        : "=r"(t):"r"(regNum) : "i0", "i1"); \
    t;})

// 3:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------
// 4:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
/// Simple DMA setup
/// @param[in] - taskNum - task number
/// @param[in] - src - source for task
/// @param[in] - dst - destination for task
/// @param[in] - numBytes - length in bytes
/// @return    - void
///
#ifdef __cplusplus
extern "C"
#endif
void scDmaSetup(swcDmaTask_t taskNum, void *src, void *dst, unsigned int numBytes);

/// Simple DMA setup with stride for Source,  will add stride content
/// @param[in] - taskNum - task number
/// @param[in] - src - source for task
/// @param[in] - dst - destination for task
/// @param[in] - numBytes - length in bytes
/// @param[in] - width
/// @param[in] - stride
/// @return    - void
///
#ifdef __cplusplus
extern "C"
#endif
void scDmaSetupStrideSrc(swcDmaTask_t taskNum, void *src, void *dst, unsigned int numBytes, unsigned int width, int stride);

/// Simple DMA setup with stride for Destination, will remove stride content
/// @param[in] - taskNum - task number
/// @param[in] - src - source for task
/// @param[in] - dst - destination for task
/// @param[in] - numBytes - length in bytes
/// @param[in] - width
/// @param[in] - stride
/// @return    - void
///
#ifdef __cplusplus
extern "C"
#endif
void scDmaSetupStrideDst(swcDmaTask_t taskNum, void *src, void *dst, unsigned int numBytes, unsigned int width, int stride);

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
#ifdef __cplusplus
extern "C"
#endif
void scDmaSetupFull(swcDmaTask_t taskNum, unsigned int cfg, void *src, void *dst, unsigned int numBytes, unsigned int width, int stride);

/// Blocking DMA Copy
/// @param[in] - dst - destination for copy
/// @param[in] - src - source for copy
/// @param[in] - numBytes - length in bytes for copy
/// @return    - void
///
#ifdef __cplusplus
extern "C"
#endif
void scDmaCopyBlocking(void *dst, void *src, unsigned int numBytes);

/// Mutex Request
/// @param[in] - mutexNum - mutex number requested
/// @return    - void
///
#ifdef __cplusplus
extern "C"
#endif
void scMutexRequest(unsigned int mutexNum);

/// This will release mutex
/// @param[in] - mutexNum - mutex number that will be released
/// @return    - void
///
#ifdef __cplusplus
extern "C"
#endif
void scDmaMemsetDDR(void *dst, unsigned char val, unsigned int numBytes, unsigned int linw_width, int stride);

#endif
