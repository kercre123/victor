///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup bootSlaveI2C BootSlave component
/// @{
/// @brief    Boot 156 image over i2c from 153
///

#ifndef _BOOT_SLAVE_I2C_H_
#define _BOOT_SLAVE_I2C_H_

// Includes
#include "app_types.h"

/// Power up the 156 board from 153
///
/// @return     
///
int PowerUp156();


/// Power up the 156 board from 153
///
/// @param	mvcmd_img  pointer to the begining of the .mvcmd image you want to boot
/// @param	len		   size of image in bytes
/// @return            void
///
void BootMv156(unsigned char* mvcmd_img, unsigned int len);
/// @}
#endif
