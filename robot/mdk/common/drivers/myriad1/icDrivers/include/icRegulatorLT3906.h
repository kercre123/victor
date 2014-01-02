///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for the Regulator IC LT3906
/// 
/// This driver supports the configuration of the LT3906 regulator
/// 
/// 
/// 

#ifndef REG_LT3906_H
#define REG_LT3906_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>
#include <isaac_registers.h>
#include <DrvGpio.h>
#include <DrvI2c.h>

#include "icRegulatorLT3906Defines.h"
   
// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Configure the 4 Voltages on an LT3906 Regulator
/// @param[in] Handle of active I2C bus with target regulator attached
/// @param[in] i2C Slave address of the regulator
/// @param[in] pCfg -> Required Voltage Configuration
/// @return    
int icLT3906Configure(I2CM_Device * i2cDev,u32 moduleAddr, tyLT3906Config * pCfg);


/// Check that regulator status is OK and voltages are programmed as per the desired config
/// @param[in] Handle of active I2C bus with target regulator attached
/// @param[in] i2C Slave address of the regulator
/// @param[in] pCfg -> Desired Voltage Configuration
/// @return    
int icLT3906CheckVoltageOK(I2CM_Device * i2cDev,u32 moduleAddr, tyLT3906Config * pCfg);

#endif      
