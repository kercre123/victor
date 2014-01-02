///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     API manipulating Slice functionalities
///
///

#include "swcSliceUtils.h"
#include <mv_types.h>
#include <isaac_registers.h>
#include <DrvRegUtilsDefines.h>

void swcSliceReleaseMutex(unsigned int mutex_no)
{
    // release access to mutex
    SET_REG_WORD(LHB_CMX_MTX_BASE + (CMX_MTX_0_GET + mutex_no << 8), 0x000000FF);
    return;
}

void swcSliceRequestMutex(unsigned int mutex_no)
{
    int index;
    // request access to mutex
    do
    {
        index = GET_REG_WORD_VAL(LHB_CMX_MTX_BASE + (CMX_MTX_0_GET + mutex_no << 8));
    }
    while (index == 0x0);

    return ;
}

int swcSliceIsMutexFree(unsigned int mutex_no)
{
    int index;
    // request access to mutex
    index = GET_REG_WORD_VAL(LHB_CMX_MTX_BASE + (CMX_MTX_0_GET + mutex_no << 8));
    if (index == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

int swcIsDmaDone(u32 shave_no)
{
    int stat, dma_base;
    dma_base = SHAVE_0_BASE_ADR + (shave_no * 0x10000) + SLC_OFFSET_DMA;
    stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
    if (stat & 0x4) // If DMA Busy
        return FALSE;
    else
        return TRUE;
}

// DMA used for fast transfer
void swcDmaMemcpy(u32 svu_nr, u32 *dst, u32 *src, u32 len, u32 blocking)
{
    int stat, dma_base;
    u32 safe_dst = (u32)dst;
    u32 safe_src = (u32)src;

    if ((safe_dst & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_dst = (safe_dst & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }
    if ((safe_src & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_src = (safe_src & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }

    dma_base = SHAVE_0_BASE_ADR + (svu_nr * 0x10000) + SLC_OFFSET_DMA;

    //Make sure DMA is idle before we start it again
    do
    {
        stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
    } while (stat & 0x4);

    //Enable
    SET_REG_WORD(dma_base + SHV_DMA_0_CFG, 1);
    //Source
    SET_REG_WORD(dma_base + SHV_DMA_0_SRC_ADDR, safe_src);
    //Destination
    SET_REG_WORD(dma_base + SHV_DMA_0_DST_ADDR, safe_dst);
    //Length
    SET_REG_WORD(dma_base + SHV_DMA_0_SIZE, len);
    //Start DMA0
    SET_REG_WORD(dma_base + SHV_DMA_TASK_REQ, 0);

    if (blocking)
    {// Wait till completes
        do
        {
            stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
        } while (stat & 0x4);
    }
}

// DMA used for fast transfer
void swcDmaRxStrideMemcpy(u32 svu_nr, u32 *dst, u32 *src, u32 len, u32 len_width, u32 len_stride, u32 blocking)
{
    int stat, dma_base;
    u32 safe_dst = (u32)dst;
    u32 safe_src = (u32)src;

    if ((safe_dst & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_dst = (safe_dst & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }
    if ((safe_src & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_src = (safe_src & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }

    dma_base = SHAVE_0_BASE_ADR + (svu_nr * 0x10000) + SLC_OFFSET_DMA;

    //Make sure DMA is idle before we start it again
    do
    {
        stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
    } while (stat & 0x4);
    //Enable
    SET_REG_WORD(dma_base + SHV_DMA_0_CFG, DMA_ENABLE | DMA_SRC_USE_STRIDE);
    //Source
    SET_REG_WORD(dma_base + SHV_DMA_0_SRC_ADDR, safe_src);
    //Destination
    SET_REG_WORD(dma_base + SHV_DMA_0_DST_ADDR, safe_dst);
    //Line width
    SET_REG_WORD(dma_base + SHV_DMA_0_LINE_WIDTH, len_width);
    //Line stride
    SET_REG_WORD(dma_base + SHV_DMA_0_LINE_STRIDE, len_stride);
    //Length
    SET_REG_WORD(dma_base + SHV_DMA_0_SIZE, len);
    //Start DMA
    SET_REG_WORD(dma_base + SHV_DMA_TASK_REQ, 0);

    if (blocking)
    {// Wait till completes
        do
        {
            stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
        } while (stat & 0x4);
    }
}

// DMA used for fast transfer
void swcShaveDmaSetupStrideSrc(u32 svu_nr, swcDmaTask_t task_no, u32 *dst , u32 *src, u32 len, u32 len_width, u32 len_stride)
{
    int stat, dma_base;
    u32 safe_dst = (u32)dst;
    u32 safe_src = (u32)src;

    if ((safe_dst & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_dst = (safe_dst & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }
    if((safe_src & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_src = (safe_src & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }

    dma_base = SHAVE_0_BASE_ADR + (svu_nr * 0x10000) + SLC_OFFSET_DMA;

    //Make sure DMA is idle before we start it again
    do
    {
        stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
    } while (stat & 0x4);

    //Enable
    SET_REG_WORD(dma_base + (SHV_DMA_0_CFG + (((u32)task_no) << 8)), DMA_ENABLE | DMA_SRC_USE_STRIDE);
    //Source
    SET_REG_WORD(dma_base + (SHV_DMA_0_SRC_ADDR + (((u32)task_no) << 8)), safe_src);
    //Destination
    SET_REG_WORD(dma_base + (SHV_DMA_0_DST_ADDR + (((u32)task_no) << 8)), safe_dst);
    //Line width
    SET_REG_WORD(dma_base + (SHV_DMA_0_LINE_WIDTH + (((u32)task_no) << 8)), len_width);
    //Line stride
    SET_REG_WORD(dma_base + (SHV_DMA_0_LINE_STRIDE + (((u32)task_no) << 8)), (*((u32*)&len_stride)));
    //Length
    SET_REG_WORD(dma_base + (SHV_DMA_0_SIZE + (((u32)task_no) << 8)), len);
}

// DMA used for fast transfer
void swcShaveDmaSetupFull(u32 svu_nr, swcDmaTask_t task_no, u32 cfg, u32 *dst, u32 *src, u32 len, u32 len_width, s32 len_stride)
{
    int stat, dma_base;
    u32 safe_dst = (u32)dst;
    u32 safe_src = (u32)src;

    if ((safe_dst & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_dst = (safe_dst & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }
    if ((safe_src & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_src = (safe_src & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }

    dma_base     = SHAVE_0_BASE_ADR + (svu_nr * 0x10000) + SLC_OFFSET_DMA;

    //Make sure DMA is idle before we start it again
    do
    {
        stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
    } while (stat & 0x4);

    //Enable
    SET_REG_WORD(dma_base + (SHV_DMA_0_CFG + (((u32)task_no) << 8)), cfg);
    //Source
    SET_REG_WORD(dma_base + (SHV_DMA_0_SRC_ADDR + (((u32)task_no) << 8)), safe_src);
    //Destination
    SET_REG_WORD(dma_base + (SHV_DMA_0_DST_ADDR + (((u32)task_no) << 8)), safe_dst);
    //Line width
    SET_REG_WORD(dma_base + (SHV_DMA_0_LINE_WIDTH + (((u32)task_no) << 8)), len_width);
    //Line stride
    SET_REG_WORD(dma_base + (SHV_DMA_0_LINE_STRIDE + (((u32)task_no) << 8)), (*((u32*)&len_stride)));
    //Length
    SET_REG_WORD(dma_base + (SHV_DMA_0_SIZE + (((u32)task_no) << 8)), len);
}

void swcShaveDmaCopyStrideSrcBlocking(u32 svu_nr, u32 *dst , u32 *src, u32 len, u32 len_width, u32 len_stride)
{
    int stat, dma_base;
    u32 safe_dst = (u32)dst;
    u32 safe_src = (u32)src;

    if ((safe_dst & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_dst = (safe_dst & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }
    if((safe_src & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_src = (safe_src & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }

    dma_base = SHAVE_0_BASE_ADR + (svu_nr*0x10000) + SLC_OFFSET_DMA;

    //Make sure DMA is idle before we start it again
    do
    {
        stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
    } while (stat & 0x4);

    //Enable
    SET_REG_WORD(dma_base + (SHV_DMA_0_CFG + (((u32)DMA_TASK_0) << 8)), DMA_ENABLE | DMA_SRC_USE_STRIDE);
    //Source
    SET_REG_WORD(dma_base + (SHV_DMA_0_SRC_ADDR + (((u32)DMA_TASK_0) << 8)), safe_src);
    //Destination
    SET_REG_WORD(dma_base + (SHV_DMA_0_DST_ADDR + (((u32)DMA_TASK_0) << 8)), safe_dst);
    //Line width
    SET_REG_WORD(dma_base + (SHV_DMA_0_LINE_WIDTH + (((u32)DMA_TASK_0) << 8)), len_width);
    //Line stride
    SET_REG_WORD(dma_base + (SHV_DMA_0_LINE_STRIDE + (((u32)DMA_TASK_0) << 8)), (*((u32*)&len_stride)));
    //Length
    SET_REG_WORD(dma_base + (SHV_DMA_0_SIZE + (((u32)DMA_TASK_0) << 8)), len);

    //Start DMA
    SET_REG_WORD(dma_base + SHV_DMA_TASK_REQ, 0);

    //Wait for the transfer to finish
    do
    {
        stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
    } while (stat & 0x4);

    return;

}

// DMA used for fast transfer
void swcShaveDmaSetupStrideDst(u32 svu_nr, swcDmaTask_t task_no, u32 *dst , u32 *src, u32 len, u32 len_width, s32 len_stride)
{
    int stat, dma_base;
    u32 safe_dst = (u32)dst;
    u32 safe_src = (u32)src;

    if ((safe_dst & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_dst = (safe_dst & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }
    if ((safe_src & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_src = (safe_src & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }

    dma_base = SHAVE_0_BASE_ADR + (svu_nr * 0x10000) + SLC_OFFSET_DMA;

    //Make sure DMA is idle before we start it again
    do
    {
        stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
    } while (stat & 0x4);

    //Enable
    SET_REG_WORD(dma_base + (SHV_DMA_0_CFG + (((u32)task_no) << 8)), DMA_ENABLE | DMA_DST_USE_STRIDE);
    //Source
    SET_REG_WORD(dma_base + (SHV_DMA_0_SRC_ADDR + (((u32)task_no) << 8)), safe_src);
    //Destination
    SET_REG_WORD(dma_base + (SHV_DMA_0_DST_ADDR + (((u32)task_no) << 8)), safe_dst);
    //Line width
    SET_REG_WORD(dma_base + (SHV_DMA_0_LINE_WIDTH + (((u32)task_no) << 8)), len_width);
    //Line stride
    SET_REG_WORD(dma_base + (SHV_DMA_0_LINE_STRIDE + (((u32)task_no) << 8)), (*((u32*)&len_stride)));
    //Length
    SET_REG_WORD(dma_base + (SHV_DMA_0_SIZE + (((u32)task_no) << 8)), len);
}

void swcShaveDmaCopyStrideDstBlocking(u32 svu_nr, u32 *dst , u32 *src, u32 len, u32 len_width, s32 len_stride)
{
    int stat, dma_base;
    u32 safe_dst = (u32)dst;
    u32 safe_src = (u32)src;

    if ((safe_dst & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_dst = (safe_dst & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }
    if ((safe_src & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_src = (safe_src & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }

    dma_base = SHAVE_0_BASE_ADR + (svu_nr * 0x10000) + SLC_OFFSET_DMA;

    //Make sure DMA is idle before we start it again
    do
    {
        stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
    } while (stat & 0x4);


    //Enable
    SET_REG_WORD(dma_base + (SHV_DMA_0_CFG + (((u32)DMA_TASK_0) << 8)), DMA_ENABLE | DMA_DST_USE_STRIDE);
    //Source
    SET_REG_WORD(dma_base + (SHV_DMA_0_SRC_ADDR + (((u32)DMA_TASK_0) << 8)), safe_src);
    //Destination
    SET_REG_WORD(dma_base + (SHV_DMA_0_DST_ADDR + (((u32)DMA_TASK_0) << 8)), safe_dst);
    //Line width
    SET_REG_WORD(dma_base + (SHV_DMA_0_LINE_WIDTH + (((u32)DMA_TASK_0) << 8)), len_width);
    //Line stride
    SET_REG_WORD(dma_base + (SHV_DMA_0_LINE_STRIDE + (((u32)DMA_TASK_0) << 8)), (*((u32*)&len_stride)));
    //Length
    SET_REG_WORD(dma_base + (SHV_DMA_0_SIZE + (((u32)DMA_TASK_0) << 8)), len);

    //Start DMA
    SET_REG_WORD(dma_base + SHV_DMA_TASK_REQ, 0);

    //Wait for the transfer to finish
    do
    {
        stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
    } while (stat & 0x4);

    return;
}

// DMA used for fast transfer
void swcShaveDmaSetup(u32 svu_nr, swcDmaTask_t task_no, u32 *dst , u32 *src, u32 len)
{
    int stat, dma_base;
    u32 safe_dst = (u32)dst;
    u32 safe_src = (u32)src;

    if ((safe_dst & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_dst = (safe_dst & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }
    if ((safe_src & LHB_CMX_BASE_ADR) == LHB_CMX_BASE_ADR)
    {
        safe_src = (safe_src & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }

    dma_base = SHAVE_0_BASE_ADR + (svu_nr * 0x10000) + SLC_OFFSET_DMA;

    //Make sure DMA is idle before we start it again
    do
    {
        stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
    } while (stat & 0x4);

    //Enable
    SET_REG_WORD(dma_base + (SHV_DMA_0_CFG + (((u32)task_no) << 8)), DMA_ENABLE);
    //Source
    SET_REG_WORD(dma_base + (SHV_DMA_0_SRC_ADDR + (((u32)task_no) << 8)), safe_src);
    //Destination
    SET_REG_WORD(dma_base + (SHV_DMA_0_DST_ADDR + (((u32)task_no) << 8)), safe_dst);
    //Length
    SET_REG_WORD(dma_base + (SHV_DMA_0_SIZE + (((u32)task_no) << 8)), len);
}

void swcShaveDmaStartTransfer(u32 svu_nr, u32 number_of_tasks_to_start)
{
    int dma_base;
    dma_base = SHAVE_0_BASE_ADR + (svu_nr * 0x10000) + SLC_OFFSET_DMA;
    //Start DMA
    SET_REG_WORD(dma_base + SHV_DMA_TASK_REQ, number_of_tasks_to_start - 1);
    return;
}

void swcShaveDmaWaitTransfer(u32 svu_nr)
{
    int stat, dma_base;
    dma_base = SHAVE_0_BASE_ADR + (svu_nr * 0x10000) + SLC_OFFSET_DMA;
    do
    {
        stat = GET_REG_WORD_VAL(dma_base + SHV_DMA_TASK_STAT);
    } while (stat & 0x4);
    return;
}






