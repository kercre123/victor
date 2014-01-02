///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup HdmiTxIteApi HdmiTxIte component
/// @{
/// @brief     Hdmi Tx Ite Functions API.
///

#ifndef _HDMI_TX_ITE_API_H_
#define _HDMI_TX_ITE_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "HdmiTxIteApiDefines.h"
#include "DrvI2cMasterDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------


// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

///Configure the i2c for tx driver
///@param hdmiI2cHandle the I2C device handle
int HdmiSetup(I2CM_Device * hdmiI2cHandle);

///Configure the entire TX
void HdmiConfigure(void); 
/// @}
#endif // _HDMI_TX_ITE_API_H_
