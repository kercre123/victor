///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// 
/// 
/// 
/// 
/// 
/// 

#ifndef BRD_MV0162_H
#define BRD_MV0162_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "brdMv0162Defines.h"

#include "DrvI2cMaster.h"
#include "icMipiTC358746.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

extern TMipiBridge mv162MipiBridges[];
extern int mv162MipiBridgesCount;

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
/// Initialise the default configuration for I2C1 and I2C2 on the MV0162 Board
/// 
/// @param[in] pointer to an I2C configuration structure for I2C1 (OR NULL to use board defaults)
/// @param[in] pointer to an I2C configuration structure for I2C2 (OR NULL to use board defaults)
/// @param[in] pointer to storage for an *I2CM_Device Handle for I2C Device 1
/// @param[in] pointer to storage for an *I2CM_Device Handle for I2C Device 2
/// @return  0 on Success
int brd162InitialiseI2C(tyI2cConfig * i2cCfg0, tyI2cConfig * i2cCfg1,I2CM_Device ** i2c1Dev,I2CM_Device ** i2c2Dev);

/// Initialise the default configuration for MIPI bridges on the MV0162 Board
/// 
/// @param[in] pointer to storage for an *TMipiBridgeCollection Handle for Mipi bridges 
/// @param[in] pointer for an I2CM_Device Handle for I2C Device 1
/// @param[in] pointer for an I2CM_Device Handle for I2C Device 2
/// @return  0 on Success
int brd162InitialiseMipiBridges(I2CM_Device * i2cDev1, I2CM_Device * i2cDev2);         
#endif // BRD_MV0162_H  
