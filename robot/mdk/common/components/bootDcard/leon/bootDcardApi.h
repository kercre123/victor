///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup bootDcardApi Daughter card boot
/// @{
/// @brief Implementation of a method to boot daughter card via i2c
#ifndef _BOOT_DCARD_API_H_
#define _BOOT_DCARD_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "bootDcardApiDefines.h"
#include "DrvI2cMaster.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------


// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Set the currently stored control parameters, if shave is free it also kicks of processing.
/// Setting parameters cyclically is the way to assign new frames to be resized.
/// No interrupt support to report resize completion, so this function return code signals completion.
/// @param   bootImg	pointer to start of boot image
/// @param   bootSize	size of boot image in bytes
/// @param   dev		i2c device handler
/// @return  i2c specific error code, 0 if success
///
I2CM_StatusType bootDcardI2C(u8 *bootImg, u32 bootSize, I2CM_Device* dev);
/// @}
#endif /* BOOT_DCARD_API_H_ */
