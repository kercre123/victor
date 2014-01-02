///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     Series of utility functions to facilitate automated test
/// 
/// 
/// 
/// 
/// 

#ifndef SOFTWARE_TEST_UTILS_H
#define SOFTWARE_TEST_UTILS_H 

// 1: Includes      
// ----------------------------------------------------------------------------
#include "swcTestUtilsDefines.h"
#include "mv_types.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------
// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------


/// @return    The processor type the simulations are running on
///
tyProcessorType swcGetProcessorType();


/// Function that initializes the benchmark structure`s elements
/// initializes with either 0, or -1(-1 is used to avoid cases when execution cycles or stalls are 0)
/// @param[in]  perfStruct  pointer to the structure that should be initialized
/// @return  none
/// @code
///swcShaveProfInit(0, &perfStr);
///
///while( swcShaveProfGatheringDone(&perfStr) == -1)
///{
///
///    swcShaveProfStartGathering(0, &perfStr);
///    swcStartShave(0,(u32)&SVE0_main);
///    swcWaitShave(0);
///    swcShaveProfStopGathering(0, &perfStr);
///
///}
///swcShaveProfPrint(0, &perfStr);
///@endcode
///
void swcShaveProfInit(performanceStruct *perfStruct);


/// Function that starts the counters for structure`s members
/// @param[in]  shaveNumber  shave number to start
/// @param[in]  perfStruct  pointer to the structure that should be initialized
/// @return  none
///
void swcShaveProfStartGathering(u32 shaveNumber, performanceStruct *perfStruct);


/// Function that verifies if all the structure`s parameters are updated with the values from the counters
// @param[in]  shaveNumber  shave number to start
/// @param[in]  perfStruct  pointer to the structure that should be updated with the values read from counters
/// @return  returns -1 if not all structure`s filed are updated and 1 if they are
///
int swcShaveProfGatheringDone(performanceStruct *perfStruct);


/// Function that reads the values from the counters
/// @param[in]  shaveNumber  shave number to start
/// @param[in]  perfStruct  pointer to the structure that should be updated with the counter values
/// @return  none
///
void swcShaveProfStopGathering(u32 shaveNumber, performanceStruct *perfStruct);


/// Function that prints the values that were read from the counters
/// @param[in]  shaveNumber  shave number to start
/// @param[in]  perfStruct  pointer to the structure whose params are all updated
/// @return  none
///
void swcShaveProfPrint(u32 shaveNumber, performanceStruct *perfStruct);

/// Function that starts one counter at the time, finding information about one possible field only
/// @param[in]  shaveNumber  shave number to start
/// @param[in]  perfDefines  one of the fields from the enum perfCounterDef
/// @return  none
///
void swcShaveProfStartGatheringFields(u32 shaveNumber, performanceCounterDef perfDefines);
/// Function that prints and reads values from counters
/// @param[in]  shaveNumber  shave number to start
/// @param[in]  perfDefines  one of the fields from the enum perfCounterDef(for stalls, instructions, branches, timer and clk cycles)
/// @return     none
void swcShaveProfStopFieldsGatehering(u32 shaveNumber, performanceCounterDef perfDefines);

/// Function that start gathering information about cycles, stalls and instructions
/// @param[in]  shaveNumber  shave number to start
/// @return     none
void swcShaveProfileCyclesStart(u32 shaveNumber);

/// Function that prints and reads values from counters
/// @param[in]  shaveNumber  shave number to start
/// @return     none
void swcShaveProfileCyclesStop(u32 shaveNumber);
#endif 

