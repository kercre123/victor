///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by the MV0153 Board Driver API
/// 
/// This header contains all necessary hardware defined constants for this board
/// e.g. GPIO assignments, I2C addresses
/// 

/// This header is not yet complete, pending completion of the schematic !!


#ifndef BRD_MV0177_DEF_H
#define BRD_MV0177_DEF_H 

#include "DrvGpioDefines.h"
//#include "drv_i2c_master_types.h"
#include "DrvI2cMasterDefines.h"


// 1: Defines
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// LED 
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// I2C Addresses
// ----------------------------------------------------------------------------

#define TSH_MIPI_I2C_ADR   0x07
#define CAM_I2C_ADR  	   0x10

// ----------------------------------------------------------------------------
// MIPI GPIO defines
// ----------------------------------------------------------------------------

#define GPIO_MIPI_MCLK 		114
#define GPIO_MIPI_RESX 		16

// ----------------------------------------------------------------------------
// I2C properties 
// ----------------------------------------------------------------------------

#define I2C1_SDA  66
#define I2C1_SCL  65

#define I2C2_SDA  75
#define I2C2_SCL  74

#define I2C1_SPEED_KHZ_DEFAULT  (50)
#define I2C1_ADDR_SIZE_DEFAULT  (ADDR_7BIT)

#define I2C2_SPEED_KHZ_DEFAULT  (50)
#define I2C2_ADDR_SIZE_DEFAULT  (ADDR_7BIT)

#define NUM_I2C_DEVICES                 (2)
// 

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // BRD_MV0177_DEF_H

