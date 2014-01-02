///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief  API for the Shave Loader module    
/// 
/// 
/// 
/// 
/// 

#ifndef SWC_SHAVE_LOADER_H
#define SWC_SHAVE_LOADER_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include <DrvIcb.h>
#include <swcDmaTypes.h>

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 2.5: Global definitions
// ----------------------------------------------------------------------------
//Shave dummy wrappers
#define SVU(x) x
#define IRF(x) x
#define SRF(x) x
#define VRF(x) x

//use DDR address through L2 cache. Force it's use.
#define ADDR_DDRL2(x) (((u32)(x)) & 0xF0FFFFFF)

#ifndef SHAVE_INTERRUPT_LEVEL
#define SHAVE_INTERRUPT_LEVEL 3
#endif

typedef u32 swcShaveUnit_t;

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
void swcSetAbsoluteDefaultStack(u32 shave_num);
u32* swcGetWinRegsShave(u32 shave_num);
void swcSetShaveWindow(u32 shave_num, u32 window_num, u32 window_addr);
void swcSetShaveWindows(u32 shaveNumber, u32 windowA, u32 windowB, u32 windowC, u32 windowD);
//Reset windows to default values in case they are rewritten by other shaves
void swcSetShaveWindowsToDefault(u32 shaveNumber);
u32 swcShaveRunning(u32 svu);
void swcRunShave( u32 shave_nr, u32 entry_point);

/// Starts non blocking execution of a shave
/// @param[in] shave_nr u32 shave number to start
/// @param[in] entry_point u32 memory address to load in the shave instruction pointer before starting
void swcStartShave( u32 shave_nr, u32 entry_point);

/// Starts non blocking execution of a shave
/// @param[in] shave_nr u32 shave number to start
/// @param[in] entry_point u32 memory address to load in the shave instruction pointer before starting
/// @param[in] function to call when shave finished execution
void swcStartShaveAsync( u32 shave_nr, u32 entry_point, irq_handler function);

/// Write the value to a IRF/SRF/VRF Registers from a specific Shave
/// @param[in] shave_num - shave number to read T-Register value from
/// @param[in] pc - function called from the pc
/// @param[in] *fmt - string containing i, s, or v according to irf, srf or vrf ex. "iisv"
/// @param[in] ... - variabile number of params according to fmt
/// @return    - void
///
void swcStartShaveCC(u32 shave_num, u32 pc, const char *fmt, ...);

/// Write the value to a IRF/SRF/VRF Registers from a specific Shave
/// @param[in] shave_num - shave number to read T-Register value from
/// @param[in] pc - function called from the pc
/// @param[in] function to call when shave finished execution
/// @param[in] *fmt - string containing i, s, or v according to irf, srf or vrf ex. "iisv"
/// @param[in] ... - variabile number of params according to fmt
/// @return    - void
///
void swcStartShaveAsyncCC(u32 shave_num, u32 pc, irq_handler function, const char *fmt, ...);

/// Write the value to a IRF/SRF/VRF Registers from a specific Shave
/// @param[in] shave_num - shave number to read T-Register value from
/// @param[in] *fmt - string containing i, s, or v according to irf, srf or vrf ex. "iisv"
/// @param[in] ... - variabile number of params according to fmt
/// @return    - void
///
void swcSetupShaveCC(u32 shave_num, const char *fmt, ...);
/// Function that starts one shave but at the same time also sets rounding bits
/// @param[in] - shave_nr - shave number to start
/// @param[in] - rounding bits
/// @param[out] - none
void swcSetRounding( u32 shave_no, u32 roundingBits);
void swcResetShave(u32 shaveNumber);
void swcResetShaveLite(u32 shaveNumber);
/// Function that waits for the shaves used to finish
/// @param[in]  no_of_shaves  number of shaves that are used
/// @param[in]  *shave_list  list of shaves used(an array which contains all the shaves used within the application)
/// @param[out]  none
void swcWaitShaves(u32 no_of_shaves, swcShaveUnit_t *shave_list);
void swcWaitShave(u32 shave_nr);
u32 swcShavesRunning(u32 first, u32 last);
u32 swcShavesRunningArr(u32 arr[], u32 N);


/// Translate windowed address into real physical address
/// Non-windowed address are passed through.
/// @param[in] vAddr Input virtual(windowed) Address 
/// @param[in] shaveNumber u32 Shave to which the virtual address relates
u32 swcSolveShaveRelAddr(u32 vAddr, u32 shaveNumber);

/// Translate windowed address into real physical address
/// Non-windowed address are passed through.
/// Note: This API variant is provided simply for backwards
/// compatibility with Myriad1 code. Not advised for new code.
/// @param[in] vAddr Input virtual Address
/// @param[in] shaveNumber u32 Shave to which the virtual address relates
static inline u32 swcSolveShaveRelAddrAHB(u32 vAddr, u32 shaveNumber)
{
    return swcSolveShaveRelAddr(vAddr,shaveNumber);
}

void swcLoadMbin(u8 *sAddr, u32 targetS);

/// Sets a default value for stack
/// !WARNING: only use this if you are using the default ldscript or really
/// know what you're doing!
/// @param[in] shave_num u32 Shave for which to set the default stack value
void swcSetWindowedDefaultStack(u32 shave_num);

/// Dynamically load shvdlib file - These are elf object files stripped of any symbols
///@param sAddr starting address where to load the shvdlib file
///@param targetS the target Shave
void swcLoadshvdlib(u8 *sAddr, u32 targetS);

#endif // SWC_SHAVE_LOADER_H  
