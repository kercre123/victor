///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     API manipulating Slice functionalities
///
///


#ifndef MCL_DMA_TRANSFER_H_
#define MCL_DMA_TRANSFER_H_

#include "mv_types.h"
#include "mclDmaTransferDefines.h"

/// Primary interface... add tasks...

/// Initialise handle
/// note handle will be private to module and have different type on Myriad1 and Myriad2
void mclDmaInit(void *handle, u8 shaveNum);

/// Dma copy with stride
/// @param[in] svu_nr u32 Shave who's DMA engine to use
/// @param[in] dst u32* Destination address
/// @param[in] src u32* Source address
/// @param[in] len u32 length of bytes to transfer
/// @param[in] len_width u32 length of the contiguous area to transfer
/// @param[in] len_stride u32 Source stride used
///
/// Myriad1 only supports up to 4 tasks
void mclDmaAddTask(void *handle, u32 *dst , u32 *src, u32 len);
void mclDmaAddTaskStride(void *handle, u32 *dst , u32 *src, u32 len, u32 src_line_size, u32 src_stride, u32 dst_line_size, u32 dst_stride);

/// Dma copy with stride
/// Myriad-2:  May do nothing
/// Myriad-1:  Required

void mclDmaStartTasks(void *handle);


/// Function waiting for the dma tasks on one specific Slice to finish
/// @param[in] svu_nr u32 Shave of the DMA engine
/// @return - none
void mclDmaWaitAllTasksCompleted(void *handle);



/// Primary interface... add tasks...
/// ====================================================================


/// Function to realize a DMA memory transfer
/// @param[in] svu_nr u32 Shave of the DMA engine
/// @param[in] dst u32* Destination address
/// @param[in] src u32* Source address
/// @param[in] len u32 length of bytes to transfer
void mclDmaMemcpy(void *handle, u32 *dst, u32 *src, u32 len);
void mclDmaMemcpyStride(void *handle, u32 *dst , u32 *src, u32 len, u32 src_line_size, u32 src_stride, u32 dst_line_size, u32 dst_stride);


/// @param[in] len number of WORDS which should be set to the requested value
void mclDmaMemset32(void *handle, u32 *dst, u32 len, u32 value);
void mclDmaMemset64(void *handle, u32 *dst, u32 len, u64 value);
#endif /* SWCSLICEUTILS_H_ */
