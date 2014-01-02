///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by software test library
/// 
#ifndef SOFTWARE_TEST_UTILS_DEF_H
#define SOFTWARE_TEST_UTILS_DEF_H 

#include "DrvTimer.h"
#include <mv_types.h>
// 1: Defines
// ----------------------------------------------------------------------------

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
typedef enum
{
    MVI_UNKNOWN,     ///< Platform type unknwon
    MVI_IC,          ///< ASIC
    MVI_VCS,         ///< VCS Simulation
    MVI_SSIM,        ///< SabreSim Simulation
    MVI_FPGA         ///< FPGA Simulation
}tyProcessorType;

typedef struct
{
     u32 perfCounterStall;          ///< counts the stalls
     u32 perfCounterExec;           ///< counts the execution cycles
     u32 perfCounterClkCycles;      ///< counts the clock cycles
     u32 perfCounterBranch;         ///< counts the branches taken
     unsigned long long perfCounterTimer;       ///< counts the total execution of the program
     u32 countShCodeRun;            ///< counts how many times the shave code was executed
     tyTimeStamp executionTimer;    ///< assignes its value to perfCounterTimer in order to display total execution(in cycles, [us] and [ms])
}performanceStruct;


typedef enum
{
     PERF_STALL_COUNT,       ///< counts the stalls
     PERF_INSTRUCT_COUNT,    ///< counts the instruction cycles
     PERF_CLK_CYCLE_COUNT,   ///< counts the clock cycles
     PERF_BRANCH_COUNT,      ///< counts the branches taken
     PERF_TIMER_COUNT        ///< counts the total execution of the program
}performanceCounterDef;


// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // SOFTWARE_TEST_UTILS_API_DEF_H

