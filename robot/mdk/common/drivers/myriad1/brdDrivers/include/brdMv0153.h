///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for the MV0153 Board Driver
/// 
/// 
/// 
/// 
/// 
#ifndef BRD_MV0153_H
#define BRD_MV0153_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include <icPllCDCE913.h>
#include "brdMv0153Defines.h"


// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
/// Reset peripherals on MV0153 and its attached daughtercard
/// @param[in] 32 bit mask to allow selection of devices to reset
/// @param[in] action RST_NORMAL -> Apply reset and release; RST_AND_HOLD -> stayin reset
/// @return    0 on success, non-zero otherwise  
int brd153Reset(u32 devMask,tyResetAction action);
    
/// Initialise the swcLed module with the specific LED configuration for this board
/// @return    0 on success, non-zero otherwise  
int brd153InitialiseLeds(void);
            
/// Initialise the default configuration for I2C1 and I2C2 on the MV0153 Board
/// 
/// The caller can subsequently get access to the relevant I2C handles by calling
/// brd153GetDeviceHandle()
/// @param[in] pointer to an I2C configuration structure for I2C1 (OR NULL to use board defaults)
/// @param[in] pointer to an I2C configuration structure for I2C2 (OR NULL to use board defaults)
/// @param[in] pointer to storage for an *I2CM_Device Handle for I2C Device 1
/// @param[in] pointer to storage for an *I2CM_Device Handle for I2C Device 2
/// @return  0 on Success
int brd153InitialiseI2C(tyI2cConfig * i2cCfg1, tyI2cConfig * i2cCfg2,I2CM_Device ** i2c1Dev,I2CM_Device ** i2c2Dev);

// ----------------------------------------------------------------------------
// Voltage Configuration
// ----------------------------------------------------------------------------

/// Set the core Myriad voltage to a given value in mv
/// @param[in] targetCoreVoltage in mV
/// @return  0 on success, non-zero on fail  
int brd153SetCoreVoltage(int coreVoltMv);

/// Configures the External PLL to a given frequency
/// @param[in] config_index (See icPllCDCE913Defines.h for usable indexes)
/// @return    0 on success, non-zero on fail
int brd153ExternalPllConfigure(u32 config_index);

/// Configure the 8 voltages on the 2 external regulators U14,U15
///
/// These regulators provide 8 rails (4 switching and 4 LDO) to the Camera
/// daughtercard on MV0153
/// @param[in] voltage configuration structure which configures both sets of voltages
/// @return  0 on success, on-zero on fail
int brd153CfgDaughterCardVoltages(tyDcVoltageCfg * voltageConfig);


/// Returns the revision number of the PCB
///
tyMv0153PcbRevision brd153GetPcbRevison(void);

/// Returns the GPIO connected to the HDMI CEC pin
///
int brd153GetHdmiCecGpioNum(void);

/// Returns the GPIO connected to the InfraRed Sensor
///
int brd153GetInfraRedSensorGpioNum(void);

#endif // BRD_MV0153_H  


