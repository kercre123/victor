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

///use DDR address through L2 cache. Force it's use.
#define ADDR_DDRL2(x) (((u32)(x)) & 0xF7FFFFFF)

#ifndef SHAVE_INTERRUPT_LEVEL
#define SHAVE_INTERRUPT_LEVEL 3
#endif

typedef u32 swcShaveUnit_t;

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

///Get a pointer to the window registers for a particular shave
///@param shave_num shave number for which to get the pointer to registers
///@return a pointer to the beginning address of the window registers
u32* swcGetWinRegsShave(u32 shave_num);

/// Set a specific Shave window address register
/// @param[in] shave_num shave number on which the register should be set
/// @param[in] window_num number of the specific window register
/// @param[in] window_addr the value to which the register should be set
void swcSetShaveWindow(u32 shave_num, u32 window_num, u32 window_addr);

///Set window address registers for a specific Shave
/// @param[in] shave_num shave number on which the registers should be set
/// @param[in] windowA starting address of window A
/// @param[in] windowB starting address of window B
/// @param[in] windowC starting address of window C
/// @param[in] windowD starting address of window D
void swcSetShaveWindows(u32 shaveNumber, u32 windowA, u32 windowB, u32 windowC, u32 windowD);

///Reset windows to default values in case they are rewritten by other shaves
///@param[in] shaveNumber u32 shave number to start
void swcSetShaveWindowsToDefault(u32 shaveNumber);

///Check if a particular shave is running
///@param svu u32 shave number to check
///@return 1 - on running shave, 0 - on stopped shave
u32 swcShaveRunning(u32 svu);

/// Start shave shave_nr from entry_point
///@param shave_nr shave number to start
///@param entry_point memory address to load in the shave instruction pointer before starting
void swcRunShave( u32 shave_nr, u32 entry_point);

/// Starts non blocking execution of a shave
/// @param[in] shave_nr u32 shave number to start
/// @param[in] entry_point u32 memory address to load in the shave instruction pointer before starting
void swcStartShave( u32 shave_nr, u32 entry_point);

/// Starts asynchronous non blocking execution of a shave
/// @param[in] shave_nr u32 shave number to start
/// @param[in] entry_point u32 memory address to load in the shave instruction pointer before starting
/// @param[in] function to call when shave finished execution
void swcStartShaveAsync( u32 shave_nr, u32 entry_point, irq_handler function);

/// Starts non blocking execution of a shave
/// @param[in] shave_num - shave number to start
/// @param[in] pc - function called from the pc
/// @param[in] *fmt - string containing i, s, or v according to irf, srf or vrf ex. "iisv"
/// @param[in] ... - variable number of params according to fmt
/// @return    - void
///
void swcStartShaveCC(u32 shave_num, u32 pc, const char *fmt, ...);

/// Starts asynchronous non blocking execution of a shave
/// @param[in] shave_num - shave number to start
/// @param[in] pc - function called from the pc
/// @param[in] function to call when shave finished execution
/// @param[in] *fmt - string containing i, s, or v according to irf, srf or vrf ex. "iisv"
/// @param[in] ... - variabile number of params according to fmt
/// @return    - void
///
void swcStartShaveAsyncCC(u32 shave_num, u32 pc, irq_handler function, const char *fmt, ...);

/// Shave registers setup
/// @param[in] shave_num - shave number to set up
/// @param[in] *fmt - string containing i, s, or v according to irf, srf or vrf ex. "iisv"
/// @param[in] ... - variabile number of params according to fmt
/// @return    - void
///
void swcSetupShaveCC(u32 shave_num, const char *fmt, ...);

/// Function that starts one shave but at the same time also sets rounding bits
/// @param[in]  shave_no shave number to start
/// @param[in]  roundingBits rounding bits
/// @return - none
void swcSetRounding( u32 shave_no, u32 roundingBits);

//!@{
///Reset a particular shave
///@param  u32 shave number to reset
void swcResetShave(u32 shaveNumber);
void swcResetShaveLite(u32 shaveNumber);
//!@}

/// Function that waits for the shaves used to finish
/// @param[in]  no_of_shaves  number of shaves that are used
/// @param[in]  *shave_list  list of shaves used(an array which contains all the shaves used within the application)
/// @return none
void swcWaitShaves(u32 no_of_shaves, swcShaveUnit_t *shave_list);

///Function that waits for a specified shave to finish
/// @param[in] shave number to wait for
void swcWaitShave(u32 shave_nr);

///Function that checks if there is any shave running 
///in the group of shaves between "first" and "last"
///@param first the beggining of the interval
///@param last the end of the interval
u32 swcShavesRunning(u32 first, u32 last);

///Function that checks if there is any shave running
///in the group of shaves specified by "array"
///@param arr list of shaves to be checked
///@param N size of the array
u32 swcShavesRunningArr(u32 arr[], u32 N);

/// Function that translates Leon preferred CMX addresses to shave preferred ones
/// @param[in] LeonAddr Address to translate
/// @return - translated address
u32 swcTranslateLeonToShaveAddr(u32 LeonAddr);

/// Function that translates Shave preferred CMX addresses to Leon preferred ones
/// @param[in] ShaveAddr Address to translate
/// @return - translated address
u32 swcTranslateShaveToLeonAddr(u32 ShaveAddr);

//!@{
/// Converts a Shave -Relative address to a Systeme solved address (in 0x10000000 view), 
/// based on the [Target CMX Slice] and current widnow it relates to
/// @param inAddr shave relative address (can be code/data/absolute tyep of address)
/// @param targetS the target Shave
/// @return the resolved addr 
u32 swcSolveShaveRelAddr(u32 inAddr, u32 targetS);
u32 swcSolveShaveRelAddrAHB(u32 inAddr, u32 targetS);
//!@}

/// Dynamically load mbin file
///@param sAddr starting address where to load the mbin file
///@param targetS the target Shave
void swcLoadMbin(u8 *sAddr, u32 targetS);

/// Dynamically load shvdlib file - These are elf object files stripped of any symbols
///@param sAddr starting address where to load the shvdlib file
///@param targetS the target Shave
void swcLoadshvdlib(u8 *sAddr, u32 targetS);

/// Sets a default value for stack
/// !WARNING: only use this if you are using the default ldscript or really
/// know what you're doing!
/// @param[in] shave_num u32 Shave for which to set the default stack value

void swcSetWindowedDefaultStack(u32 shave_num);

/// Sets a default value for stack with absolute address
/// !WARNING: only use this if you are using the default ldscript or really
/// know what you're doing!
/// @param[in] shave_num u32 Shave for which to set the default stack value
void swcSetAbsoluteDefaultStack(u32 shave_num);

#endif // SWC_SHAVE_LOADER_H  
