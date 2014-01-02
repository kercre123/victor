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

#ifndef BRD_MV0155_H
#define BRD_MV0155_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "brdMv0155Defines.h"

#include "DrvI2cMaster.h"
#include "icMipiTC358746.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

extern TMipiBridge mv155MipiBridges[];
extern int mv155MipiBridgesCount;

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
int brd155InitialiseI2C(tyI2cConfig * i2cCfg, I2CM_Device ** i2cDev);

int brd155InitialiseMipiBridges(I2CM_Device *i2cDev);

void brd155GpioToI2cDevice(TBrd155I2cDevicesEnum tsh, I2CM_Device ** i2cDev);

I2CM_StatusType brd155ToshibaCamRegSet(I2CM_Device * i2cDev, u32 data[][2], u32 len);
I2CM_StatusType brd155ToshibaCamRegGet(I2CM_Device * i2cDev, u32 data[][2], u32 len);
#endif // BRD_MV0162_H  
