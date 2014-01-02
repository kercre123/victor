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

#ifndef BRD_MV0171_H
#define BRD_MV0171_H

// 1: Includes
// ----------------------------------------------------------------------------
#include "brdMv0171Defines.h"

#include "DrvI2cMaster.h"
#include "icMipiTC358746.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

extern TMipiBridge mv171MipiBridges[];
extern int mv171MipiBridgesCount;

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
/// Initialise the default configuration for I2C on the MV0171 Board (default is setting
/// up I2C2 for MIPI communication)
/// 
/// @param[in] pointer to an I2C configuration structure for I2C (OR NULL to use board defaults)
/// @param[in] pointer to storage for an *I2CM_Device Handle for I2C Device
/// @return  0 on Success
int brd171InitialiseI2C(tyI2cConfig * i2cCfg, I2CM_Device ** i2cDev);

/// Initialise the default configuration for MIPI bridges on the MV0171 Board
/// 
/// @param[in] pointer to storage for an *TMipiBridgeCollection Handle for Mipi bridges 
/// @param[in] pointer for an I2CM_Device Handle for I2C Device
/// @return  0 on Success
int brd171InitialiseMipiBridges(I2CM_Device * i2cDev);

/// Switch the I2C configuration to targeted device
void brd171GpioToI2cDevice(TBrd171I2cDevicesEnum tsh, I2CM_Device ** i2cDev);

/// Write data to Toshiba Camera sensor
I2CM_StatusType brd171ToshibaCamRegSet(I2CM_Device * i2cDev, u32 data[][2], u32 len);

/// Read data from Toshiba Camera sensor
I2CM_StatusType brd171ToshibaCamRegGet(I2CM_Device * i2cDev, u32 data[][2], u32 len);

#endif // BRD_MV0171_H
