///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for accessing the hardware mutexes of the Myriad platform.
///

#ifndef DRV_MUTEX_H
#define DRV_MUTEX_H

#include <mv_types.h>

// Define specific mutex mask and bits
#define MTX_NUM_MASK            0x001F
#define MTX_STAT_MASK           0x01FF

#define MTX_NO_ACCES            0x0000
#define MTX_REQ_ON_RETRY        0x0100
#define MTX_RELEASE             0x0200
#define MTX_REQ_OFF_RETRY       0x0300
#define MTX_CLEAR_OWN_PENDING   0x0400
#define MTX_CLEAR_ALL_PENDING   0x0500

// status bits meaning
#define MTX_STAT_IN_USE	        0x100
#define MTX_GARANTED_BY_LOS     0xc
#define MTX_GARANTED_BY_LRT     0xd


/// @brief Request a hardware mutex with auto repetition.
///
/// @param[in] mutex Number of the requested hardware mutex (0..31)
u32 DrvMutexLock(u32 mutex);

/// @brief Wait for mutex to be taken, if prevision was requested with auto retry
///
/// @param[in] mutex Number of the requested hardware mutex (0..31)
void DrvMutexWaitToKetIt(u32 mutex);

/// @brief Release a hardware mutex.
///
/// @param[in] mutex Number of the requested hardware mutex (0..31)
void DrvMutexUnlock(u32 mutex);

///
u32 DrvMutexLockIfAvailable(u32 mutex);

void DrvMutexClearOwnPendingRequest(u32 mutex);

void DrvMutexClearAllPendingRequest(void);

u32 DrvMutexGetStatus(u32 mutex);

u32 DrvMutexGetAllStatus(void);

// wrong function : TODO: need clarification what will be the usage situation, can be use LRT interrupts for Los interrupts ...

void DrvMutexClearInterupts(u32 intMaskToClear);

void DrvMutexConfigInterupts(u32 intEnableMask);

u32 DrvMutexGetInteruptsStatus(void);

//TODO: implement interrupt Drv support, with handler assign

#endif
