///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Definitions and types dealing with shave low level functionality
///

#ifndef _DRVSVU_
#define _DRVSVU_

// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>
#include "registersMyriad.h"
#include "DrvSvuDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------
// Do this so we can write to SVUs using for loops
extern u32 SVU_BASE_ADDR[TOTAL_NUM_SHAVES];
extern u32 SVU_CTRL_ADDR[TOTAL_NUM_SHAVES];
extern u32 SVU_DATA_ADDR[TOTAL_NUM_SHAVES];
extern u32 SVU_INST_BASE[TOTAL_NUM_SHAVES];

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Read the value of an I-Register from a specific Shave
/// @param[in] svu - shave number to read I-Register value from
/// @param[in] irfLoc - I-Register position to read
/// @return    - u32 value of the read register
///
u32 DrvSvutIrfRead(u32 svu, u32 irfLoc);

/// Read the value of a element from a V-Register from a specific Shave
/// @param[in] svu - shave number to read V-Register value from
/// @param[in] vrfLoc - V-Register position to read
/// @param[in] vrfEle - V-Register location to read
/// @return    - u32 value of the read register
///
u32 DrvSvutVrfRead(u32 svu, u32 vrfLoc, u32 vrfEle );

/// Read the value of T-Register from a specific Shave
/// @param[in] svu - shave number to read T-Register value from
/// @param[in] trfLoc - T-Register position to read
/// @return    - u32 value of the read register
///
u32 DrvSvutTrfRead(u32 shaveNumber, u32 registerOffset);

/// Write the value to an I-Register from a specific Shave
/// @param[in] svu - shave number to read I-Register value from
/// @param[in] irfLoc - I-Register position to write to
/// @param[in] irfVal - value to write
/// @return    - void
///
void DrvSvutIrfWrite(u32 svu, u32 irfLoc, u32 irfVal);

/// Write the value to a V-Register position from a specific Shave
/// @param[in] svu - shave number to read V-Register value from
/// @param[in] vrfLoc - V-Register position to write to
/// @param[in] vrfEle - V-Register location to write to
/// @param[in] vrfVal - value to write
/// @return    - void
///
void DrvSvutVrfWrite(u32 svu, u32 vrfLoc, u32 vrfEle, u32 vrfVal);

/// Write the value to a T-Register from a specific Shave
/// @param[in] svu - shave number to read T-Register value from
/// @param[in] trfLoc - T-Register position to write to
/// @param[in] trfVal - value to write
/// @return    - void
///
void DrvSvutTrfWrite(u32 shaveNumber, u32 registerOffset, u32 value);

/// Check if a shave is halted or not
/// @param[in] svu - shave number to check if halted ot not
/// @return    - u32 0-is not halted, 1 - is halted
///
u32 DrvSvuIsStopped(u32 svu);

/// Check shave tag
/// @param[in] svu - shave number to check tag for
/// @return    - u32 tag
///
u32 DrvSvutSwiTag(u32 svu);

/// Zero all register files
/// @param[in] svu - shave number to zero register files for
/// @return    - void
///
void DrvSvutZeroRfs(u32 svu);

/// Function used to start a shave from a specific address
/// @param[in] svu - shave number to start
/// @param[in] pc - program counter value
/// @return       - void
///
void DrvSvuStart(u32 svu, u32 pc);

/// Stop a specific shave
/// @param[in] svu - shave number
/// @return    - void
///
void DrvSvutStop(u32 svu);

/// Enable performance counter
/// @param[in] shaveNumber - shave number
/// @param[in] pcNb - Performance counter number. 0 or 1
/// @param[in] countType - Count type. One of PC_IBP0_EN (instructions on performance counter 0)
///                         PC_IBP1_EN (instructions on performance counter 1)
///                         PC_DBP0_EN (data counter on performance counter 0)
///                         PC_DBP1_EN (data counter on performance counter 0)
///                         any other value: just enable history
/// @return    - void
///
void DrvSvuEnablePerformanceCounter(u32 shaveNumber, u32 pcNb, u32 countType);

/// Read executed instructions
/// @param[in] shaveNumber - shave number
/// @return    - u32 get executed Instructions count
///
u32 DrvSvuGetPerformanceCounter0(u32 shaveNumber);

/// Read stall cycles
/// @param[in] shaveNumber - shave number
/// @return    - u32 get stall cycles
///
u32 DrvSvuGetPerformanceCounter1(u32 shaveNumber);

/// Enable L1 cache for a specified slice
/// @param[in] slice - slice (SHAVE) for which to enable L1 cache
/// @return     - void
void DrvSvuEnableL1Cache(u32 slice);

/// L1 cache control register setting
/// @param[in] shvNumber - shave number
/// @param[in] ctrlType - bit field options. See L1C_INVAL_LINE
/// @return    - void
///
void DrvSvuL1CacheCtrl(u32 shvNumber,     u32 ctrlType);

/// L1 cache line control register setting
/// @param[in] shvNumber - shave number
/// @param[in] ctrlType - bit field options. See L1C_INVAL_LINE
/// @param[in] address - address
/// @return    - void
///
void DrvSvuL1CacheCtrlLine(u32 shvNumber, u32 ctrlType, u32 address);

/// Invalidate L1 cache
/// @param[in] shvNumber - shave number
/// @return    - void
///
void DrvSvuInvalidateL1Cache(u32 shvNumber);

/// Resets the entire shave slice
/// @param[in] shvNumber - shave number
/// @return    - void
///
void DrvSvuSliceResetAll(u32 shaveNumber);

/// Resets the specified SHAVE
/// @param[in] shvNumber - shave number
/// @return    - void
///
void DrvSvuSliceResetOnlySvu(u32 shaveNumber);

#endif //_DRVSVU_
