///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     API manipulating Slice functionalities
///
///


#ifndef SWCSLICEUTILS_H_
#define SWCSLICEUTILS_H_

#include <swcDmaTypes.h>
#include <mv_types.h>

/// Function that releases a certain hardware mutex
/// @param[in] mutex_no mutex to release
/// @return - none
void swcSliceReleaseMutex(unsigned int mutex_no);

/// Function that requests a certain hardware mutex
/// @param[in] mutex_no mutex to request
/// @return - none
void swcSliceRequestMutex(unsigned int mutex_no);

/// Function that checks if a certain hardware mutex is free
/// @param[in] mutex_no mutex to request
/// @return  1 if mutex is in use, 0 otherwise(free)
int swcSliceIsMutexFree(unsigned int mutex_no);


/// Function waiting for the dma tasks on one specific Slice to finish
/// @param[in] svu_nr u32 Shave of the DMA engine
/// @return - none
void swcWaitDmaDone(u32 svu_nr);

/// Function to realize a DMA memory transfer
/// @param[in] svu_nr u32 Shave of the DMA engine
/// @param[in] dst u32* Destination address
/// @param[in] src u32* Source address
/// @param[in] len u32 length of bytes to transfer
/// @param[in] blocking tells the function whether to wait for the transfer to finish or not
void swcDmaMemcpy(u32 svu_nr, u32 *dst , u32 *src, u32 len, u32 blocking);

/// Function returns true if all DMA jobs on specified Shave are finished
/// otherwise it returns FALSE
/// Typically this can be used in conjunction with the non-blocking
/// version of swcDma functions to check on current DMA status without blocking
/// @param[in] svu_nr u32 Shave of the DMA engine to be checked
/// @return TRUE if JOB finished, FALSE otherwise
int swcIsDmaDone(u32 svu_nr);

/// Dma copy with stride
/// @param[in] svu_nr u32 Shave who's DMA engine to use
/// @param[in] dst u32* Destination address
/// @param[in] src u32* Source address
/// @param[in] len u32 length of bytes to transfer
/// @param[in] len_width u32 length of the contiguous area to transfer
/// @param[in] len_stride u32 Source stride used
/// @param[in] blocking tells the function whether to wait for the transfer to finish or not
void swcDmaRxStrideMemcpy(u32 svu_nr, u32 *dst , u32 *src, u32 len, u32 len_width, u32 len_stride, u32 blocking);


/// Function to set up a shave DMA engine for one task with input stride
/// @param[in] svu_nr u32 Shave who's DMA engine to use
/// @param[in] task_no swcDmaTask_t Task number to configure
/// @param[in] dst u32* Destination address
/// @param[in] src u32* Source address
/// @param[in] len u32 length of bytes to transfer
/// @param[in] len_width u32 length of the contiguous area to transfer
/// @param[in] len_stride u32 Source stride used
void swcShaveDmaSetupStrideSrc(u32 svu_nr, swcDmaTask_t task_no, u32 *dst , u32 *src, u32 len, u32 len_width, u32 len_stride);

/// Function to copy using a shave DMA engine with input stride
/// @param[in] svu_nr u32 Shave who's DMA engine to use
/// @param[in] dst u32* Destination address
/// @param[in] src u32* Source address
/// @param[in] len u32 length of bytes to transfer
/// @param[in] len_width u32 length of the contiguous area to transfer
/// @param[in] len_stride u32 Source stride used
void swcShaveDmaCopyStrideSrcBlocking(u32 svu_nr, u32 *dst , u32 *src, u32 len, u32 len_width, u32 len_stride);

/// Function to set up a shave DMA engine for one task with output stride
/// @param[in] svu_nr u32 Shave who's DMA engine to use
/// @param[in] task_no swcDmaTask_t Task number to configure
/// @param[in] dst u32* Destination address
/// @param[in] src u32* Source address
/// @param[in] len u32 length of bytes to transfer
/// @param[in] len_width u32 length of the contiguous area to transfer
/// @param[in] len_stride s32 destination stride used
void swcShaveDmaSetupStrideDst(u32 svu_nr, swcDmaTask_t task_no, u32 *dst , u32 *src, u32 len, u32 len_width, s32 len_stride);

/// Function to copy using a shave DMA engine with output stride
/// @param[in] svu_nr u32 Shave who's DMA engine to use
/// @param[in] dst u32* Destination address
/// @param[in] src u32* Source address
/// @param[in] len u32 length of bytes to transfer
/// @param[in] len_width u32 length of the contiguous area to transfer
/// @param[in] len_stride s32 destination stride used
void swcShaveDmaCopyStrideDstBlocking(u32 svu_nr, u32 *dst , u32 *src, u32 len, u32 len_width, s32 len_stride);

/// Function to set up a shave DMA engine for one task
/// @param[in] svu_nr u32 Shave who's DMA engine to use
/// @param[in] task_no swcDmaTask_t Task number to configure
/// @param[in] dst u32* Destination address
/// @param[in] src u32* Source address
/// @param[in] len u32 length of bytes to transfer
void swcShaveDmaSetup(u32 svu_nr, swcDmaTask_t task_no, u32 *dst , u32 *src, u32 len);

/// Function to start a number of tasks for one shave
/// @param[in] svu_nr u32 Shave who's DMA engine to use
/// @param[in] number_of_tasks_to_start u32 Number of tasks to start
void swcShaveDmaStartTransfer(u32 svu_nr, u32 number_of_tasks_to_start);

/// Wait for a shave DMA transfer to complete
/// @param[in] svu_nr u32 Shave for which's DMA to wait
void swcShaveDmaWaitTransfer(u32 svu_nr);

/// Setup a DMA transfer using full set of features for one shave
/// @param[in] svu_nr u32 Shave for which to set up DMA
/// @param[in] task_no u32 DMA task to configure
/// @param[in] cfg u32 DMA configuration
/// @param[in] src u8* DMA source
/// @param[in] dst u8* DMA destination
/// @param[in] len u32 bytes to transfer
/// @param[in] len_width u32 width length
/// @param[in] len_stride s32 stride length
void swcShaveDmaSetupFull(u32 svu_nr, swcDmaTask_t task_no, u32 cfg, u32 *dst, u32 *src, u32 len, u32 len_width, s32 len_stride);


/// Function to set up a shave DMA engine for transferring 4 bytes in a single task
/// @param[in] svu_nr u32 Shave who's DMA engine to use
/// @param[in] task_no swcDmaTask_t Task number to configure
/// @param[in] dst u32* Destination address
/// @param[in] src u32* Source address
static inline void swcShaveDmaMemset32(u32 svu_nr, swcDmaTask_t task_no, u32 *dst, u32 *src)
{	
	swcShaveDmaSetup(svu_nr, task_no, dst , src, 4);
}


/// Function to set up a shave DMA engine for transferring 8 bytes in a single task
/// @param[in] svu_nr u32 Shave who's DMA engine to use
/// @param[in] task_no swcDmaTask_t Task number to configure
/// @param[in] dst u32* Destination address
/// @param[in] src u32* Source address
static inline void swcShaveDmaMemset64(u32 svu_nr, swcDmaTask_t task_no, u32 *dst, u32 *src)
{
	swcShaveDmaSetup(svu_nr, task_no, dst , src, 8);
}

#endif /* SWCSLICEUTILS_H_ */
