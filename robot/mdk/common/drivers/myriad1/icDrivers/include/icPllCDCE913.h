///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API to the Driver for the External PLL CDCE913PW
/// 
/// 
/// 
/// 
/// 

#ifndef IC_EXT_PLL_913_H
#define IC_EXT_PLL_913_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "DrvI2cMaster.h"
#include "icPllCDCE913Defines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
int icPllCDCE913Configure(I2CM_Device * dev,u32 config_index);

#endif // IC_EXT_PLL_913_H  
