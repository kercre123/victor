///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup I2CSlaveApi I2C Slave API
/// @{
/// @brief     I2C Slave Functions API.
///


#ifndef _I2C_SLAVE_API_H_
#define _I2C_SLAVE_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "I2CSlaveApiDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------
extern u32 addrReg;
extern volatile u32 dataVal[0x100000];

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
/// This will initialize the I2C Slave Component
/// @param[in] hndl         pointer to the I2C slave handler
/// @param[in] blockNumber  enum type reffering to one of the I2C blocks used
/// @param[in] config       pointer to I2CSlaveAddr configuration
/// return
int I2CSlaveInit(I2CSlaveHandle *hndl, u32 blockNumber, I2CSlaveAddrCfg *config);

/// Set callback functions for the I2C component
/// @param[in] hndl           pointer to I2C slave handler
/// @param[in] cbReadAction   pointer to read function handler
/// @param[in] cbWriteAction  pointer to write function handler
/// @param[in] cbBytesToSend  pointer to bytes to send function handler
/// @return      void
void I2CSlaveSetupCallbacks(I2CSlaveHandle *hndl, i2cReadAction*  cbReadAction, i2cWriteAction* cbWriteAction, i2cBytesToSend* cbBytesToSend);


// 4:  Component Usage
// --------------------------------------------------------------------------------
//  In order to use the component the following steps are ought to be done:
//  1. Declare a variable of "I2CSlaveAddrCfg" type
//  2. Initialize the members of the variable above declared
//  3. Declare a variable of "I2CSlaveHandle" type (handler)
//  4. Initialize a member of the handler declared, "i2cConfig"
//  5  Call I2CSlaveSetupCallbacks function, having as parameters address of handler declared,
//                       result of i2cRead function, result of i2cRead function, result of BytesToRead function
//  6. Call I2CSlaveInit function, having as parameters the address of the handler declared,
//                       I2C block, and the address of I2CSlaveAddrCfg variable
//
/// @}
#endif
