///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief
///

#ifndef __SVU_COMMON_SHAVE_MM__
#define __SVU_COMMON_SHAVE_MM__

// 1: Includes
// ----------------------------------------------------------------------------
#include "myriadModel.h"
#include "mv_types.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "svuCommonShaveDefines.h"



#define CMX_SIZE_BYTES (1024 * 1024)
#define DDR_SIZE_BYTES (16 * 1024 * 1024)

u8*   p_ddr;
u8*   p_cmx;

u32  mm_valid = 0;
u32  mm_start_count = 0;
u32  mm_tasks_started = 0;

typedef struct
{
    u8*               p_dst;     //  src - source for task
    u8*               p_src;     //  dst - destination for task
    u32               num_bytes;    //  numBytes - length in bytes
    u32               line_width;   //  width (bytes)
    u32               strideSrc;       //  stride (bytes)
    u32               strideDst;       //  stride (bytes)
    u32               completed;    //  flags for debug
} tDmaCfg;


// Store all of the dma tasks here:
tDmaCfg   dmaTasks[4];
static u8*   mm_p_heap;

u32 convTaskNum(u32 task)
{
    switch (task)
    {
    case    DMA_TASK_0: return 0;
    case    DMA_TASK_1: return 1;
    case    DMA_TASK_2: return 2;
    default:            return 3;
    }
}


// Functions to creat and destroy myriad model
void mmCreate(void)
{
    p_ddr = (u8*) malloc(DDR_SIZE_BYTES);
    p_cmx = (u8*) malloc(CMX_SIZE_BYTES);

    if ((p_ddr == NULL) || (p_cmx == NULL))
    {
        printf("memory allocation failure \n");
        exit(0);
    }
    // printf("ddr in sys mem is 0x%0x\n", (u32) p_ddr);
    for (u8* i = p_ddr; i < (p_ddr + DDR_SIZE_BYTES); i++)
        *i = 0xEF;
    //printf("p_cmx in sys mem is 0x%0x\n", (u32) p_cmx);
    // Initialise dmaTasks structure to save values
    for (u32 i = 0; i < 4; i++)
    {
        memset(&dmaTasks[i], 0x00, sizeof(tDmaCfg));
        dmaTasks[i].completed = 0;
    }

    mm_valid = 1;
    mm_tasks_started = 0;
}

void mmDestroy()
{
    free(p_ddr);
    free(p_cmx);
    mm_valid = 0;
}

u8* mmGetPtr(u32 addr)
{
#ifdef USE_MYRIAD1_MEMORY_MAP
    u8* p_mem;
    if (!mm_valid)
    {
        printf("Bad MM \n");
        exit(0x1F);
    }

    switch (addr >> 24)
    {
	case 0x10 :
        p_mem = &(p_cmx[addr&0xFFFFFF]);
		break;
    case 0x1c :
        p_mem = &(p_cmx[addr&0xFFFFFF]);
        break;
    case 0x90 :
        p_mem = &(p_cmx[addr&0xFFFFFF]);
        break;
    case 0x48 :
        p_mem = &(p_ddr[addr&0xFFFFFF]);
        break;
    case 0x40 :
        p_mem = &(p_ddr[addr&0xFFFFFF]);
        break;
    default:
        printf("Bad addr passed to myriadModel 0x%x\n", (unsigned int)addr);
        exit(1);
    }

    return p_mem;
#else
	return (u8*)addr;
#endif
}

void  mmInitHeap(u32 cmx_heap_addr)
{
    if (!mm_valid)
    {
        printf("Bad MM \n");
        exit(0x1F);
    }

    mm_p_heap = mmGetPtr(cmx_heap_addr);
}

void* mmMalloc(u32 sz_bytes)
{
    u8* retval = mm_p_heap;

    if (!mm_valid)
    {
        printf("Bad MM \n");
        exit(0x1F);
    }

    mm_p_heap += sz_bytes;
    return retval;
}


// Set address in the memory model
void mmSetMemcpy(u32 addr, void* src, u32 sz_bytes)
{
    u8* p_mem = mmGetPtr(addr);
    if (!mm_valid)
    {
        printf("Bad MM \n");
        exit(0x1F);
    }

    memcpy(p_mem, src, sz_bytes);
}

// Get data from myriad model
void mmGetMemcpy(void* dst, u32 addr, u32 sz_bytes)
{
    u8* p_mem = mmGetPtr(addr);
    if (!mm_valid)
    {
        printf("Bad MM \n");
        exit(0x1F);
    }

    memcpy(dst, p_mem, sz_bytes);
}


/// File is overloading implementation of shave functions
/// defined in svuCommonShave.h
/// ----------------------------------------------------------------------------

/// Simple DMA setup
/// @param[in] - taskNum - task number
/// @param[in] - src - source for task
/// @param[in] - dst - destination for task
/// @param[in] - numBytes - length in bytes
/// @return    - void
///
void scDmaSetup(swcDmaTask_t taskNum, unsigned char *src, unsigned char *dst, unsigned int numBytes)
{
	u32 task = convTaskNum(taskNum);

    if (taskNum >= 4)
        exit(2);
    if (!mm_valid)
    {
        printf("Bad MM \n");
        exit(0x1F);
    }

    dmaTasks[task].p_dst        = mmGetPtr((u32) dst);
    dmaTasks[task].p_src        = mmGetPtr((u32) src);
    dmaTasks[task].num_bytes    = numBytes;
    dmaTasks[task].line_width   = 0;
    dmaTasks[task].strideDst    = 0;
    dmaTasks[task].strideSrc    = 0;
    dmaTasks[task].completed    = 0;
}

/// Simple DMA setup with destination stride
/// @param[in] - taskNum - task number
/// @param[in] - src - source for task
/// @param[in] - dst - destination for task
/// @param[in] - numBytes - length in bytes
/// @param[in] - width
/// @param[in] - stride
/// @return    - void
///
void scDmaSetupStrideDst(unsigned int task, u8* src, u8* dst, unsigned int numBytes, unsigned int width, unsigned int stride)
{
    u32 taskNum = convTaskNum(task);

    if (taskNum >= 4)
        exit(2);
    if (!mm_valid)
    {
        printf("Bad MM \n");
        exit(0x1F);
    }

    dmaTasks[taskNum].p_dst        = mmGetPtr((u32) dst);
    dmaTasks[taskNum].p_src        = mmGetPtr((u32) src);
    dmaTasks[taskNum].num_bytes    = numBytes;
    dmaTasks[taskNum].line_width   = width;
    dmaTasks[taskNum].strideDst    = stride;
    dmaTasks[taskNum].strideSrc    = 0;
    dmaTasks[taskNum].completed    = 0;
}

/// Simple DMA setup with source stride
/// @param[in] - taskNum - task number
/// @param[in] - src - source for task
/// @param[in] - dst - destination for task
/// @param[in] - numBytes - length in bytes
/// @param[in] - width
/// @param[in] - stride
/// @return    - void
///
void scDmaSetupStrideSrc(unsigned int task, u8* src, u8* dst, unsigned int numBytes, unsigned int width, unsigned int stride)
{
    u32 taskNum = convTaskNum(task);

    if (taskNum >= 4)
        exit(2);
    if (!mm_valid)
    {
        printf("Bad MM \n");
        exit(0x1F);
    }

    dmaTasks[taskNum].p_dst        = mmGetPtr((u32) dst);
    dmaTasks[taskNum].p_src        = mmGetPtr((u32) src);
    dmaTasks[taskNum].num_bytes    = numBytes;
    dmaTasks[taskNum].line_width   = width;
    dmaTasks[taskNum].strideDst    = 0;
    dmaTasks[taskNum].strideSrc    = stride;
    dmaTasks[taskNum].completed    = 0;
}

/// Full DMA setup
/// @param[in] - taskNum - task number
/// @param[in] - cfg - DMA configuration
/// @param[in] - src - source for task
/// @param[in] - dst - destination for task
/// @param[in] - numBytes - length in bytes
/// @param[in] - width
/// @param[in] - stride
/// @return    - void
///
void scDmaSetupFull(unsigned int task, unsigned int cfg, unsigned char *src, unsigned char *dst, unsigned int numBytes, unsigned int width, unsigned int stride)
{
    if (!mm_valid)
    {
        printf("Bad MM \n");
        exit(0x1F);
    }
    printf("scDmaSetupFull nor supported \n");
}

/// This will start DMA task number taskNum
/// @param[in] - taskNum - task number
/// @return    - void
///
void scDmaStart(unsigned int taskNum)
{
    if (!mm_valid)
    {
        printf("Bad MM \n");
        exit(0x1F);
    }

    if (mm_start_count)
    {
        printf("Error - consecutive DMA Starts with no wait!\n");
    }

    // process tasks in same order as h/w
    for (s32 i = taskNum; i >= 0; i--)
    {
        if (dmaTasks[i].completed)
            printf("Dma task %d not initialized before scDmaStart called \n", (int)i);

        if ((dmaTasks[i].strideDst == 0) && (dmaTasks[i].strideSrc == 0))
        {
            // robustness enforcement:  memset area to be garbage here.  we do the real transfer when complete.
            memset(dmaTasks[i].p_dst, 0xFF,  dmaTasks[i].num_bytes);
        }
        else
        {
            printf("Stride not supported yet\n");
            exit(3);
        }
    }

    mm_tasks_started = taskNum;

    mm_start_count++;
}

/// This will wait that DMA finishes task number taskNum
/// @param[in] - void
/// @return    - void
///
void scDmaWaitFinished(void)
{
    //printf("scDmaWaitFinished\n");
    if (!mm_valid)
    {
        printf("Bad MM \n");
        exit(0x1F);
    }

    // process tasks in same order as h/w
    for (s32 i = mm_tasks_started; i >= 0 ; i--)
    {
        /*if (dmaTasks[i].completed)
        {
            printf("Dma task %d not initialised before scDmaWaitFinished called \n", (int)i);
        }
        else*/
		if (!dmaTasks[i].completed)
        {
            if ((dmaTasks[i].strideDst == 0) && (dmaTasks[i].strideSrc == 0))
            {
                memcpy(dmaTasks[i].p_dst, dmaTasks[i].p_src, dmaTasks[i].num_bytes);
                dmaTasks[i].completed  = 1;
            }
            else
            {
                printf("Stride not supported yet\n");
                exit(3);
            }
        }
    }

    if (mm_start_count)
        mm_start_count--;

    // do nothing -- DMAs are instantaneous here
}


/// Blocking DMA Copy
/// @param[in] - dst - destination for copy
/// @param[in] - src - source for copy
/// @param[in] - numBytes - length in bytes for copy
/// @return    - void
///
void scDmaCopyBlocking(unsigned char *dst, unsigned char *src, u32 numBytes)
{
    scDmaSetup(DMA_TASK_0, src, dst, numBytes);
	scDmaStart(DMA_TASK_0);
	scDmaWaitFinished();
}

/// Mutex Request
/// @param[in] - mutexNum - mutex number requested
/// @return    - void
///
void scMutexRequest(unsigned int mutexNum)
{
    printf("scMutexRequest nor supported \n");
}

/// This will release mutex
/// @param[in] - mutexNum - mutex number that will be released
/// @return    - void
///
void scMutexRelease(unsigned int mutexNum)
{
    printf("scMutexRelease nor supported \n");
}

#endif
