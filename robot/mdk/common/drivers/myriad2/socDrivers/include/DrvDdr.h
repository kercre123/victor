///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for the DRAM Driver
/// 

#ifndef DRV_DDR_H
#define DRV_DDR_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
//#include "DrvDdrDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Initialise the system DRAM and configure for specified frequency.
///
/// This function first checks to see if the DRAM is already initialised.
/// If it is initialised then the driver simply tunes the DRAM for the 
/// desired frequency.
/// If the DRAM is not initialised, then the driver will reset the DDR controller
/// and initialise the DRAM, and then tune for the desired frequency. 
/// 
/// @param[in] ddrFrequencyKhz - Operation frequency 

void DrvDdrInitialise(u32 ddrFrequencyKhz);

/** Notify the DDR driver that the frequency of DRAM has changed
	
	This will typically happen after the user changes system frequency 
	using CPR Driver functions.
	Note: The user does not have to manually call this function
	as this particular notification is handled directly from the CPR driver
	@param[in] newDdrClockKhz - new ddr frequency
*/
//void DrvDdrNotifyClockChange(u32 newDdrClockKhz);


#endif // DRV_DDR_H  
