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
#include <registersMyriad2.h>
#include <DrvRegUtilsDefines.h>

// DMA used for fast transfer
void swcDmaMemcpy(u32 svu_nr, u32 *dst, u32 *src, u32 len, u32 blocking)
{
    int stat, dma_base;
    u32 safe_dst = (u32)dst;
    u32 safe_src = (u32)src;

    if ((safe_dst & CMX_BASE_ADR) ==CMX_BASE_ADR)
    {
        safe_dst = (safe_dst & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
    }
    if ((safe_src & CMX_BASE_ADR) == CMX_BASE_ADR)
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

