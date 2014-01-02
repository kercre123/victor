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


#ifndef BRD_MV0171_DEF_H
#define BRD_MV0171_DEF_H 

#include "DrvGpioDefines.h"
//#include "drv_i2c_master_types.h"
#include "DrvI2cMasterDefines.h"


// 1: Defines
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// LED 
// ----------------------------------------------------------------------------

// TODO
//#define LED0 39
#define LED1 57
#define LED3 56

#define NO_LEDS 2
// ----------------------------------------------------------------------------
// I2C Addresses
// ----------------------------------------------------------------------------

// TODO
#define TSH_MIPI_I2C_ADR   0x07
#define CAM_I2C_ADR  			0x10

// ----------------------------------------------------------------------------
// MIPI GPIO defines
// ----------------------------------------------------------------------------

// TODO
//defines for bridge chips in Katana
#define GPIO_MIPI_B_PCLK		0
#define GPIO_MIPI_B_HS			2
#define GPIO_MIPI_B_VS    		1
#define GPIO_MIPI_B_D0    		4
#define GPIO_MIPI_B_D7    		11
#define GPIO_MIPI_B_MCLK  		13
#define GPIO_MIPI_B_RESX  		53
#define GPIO_MIPI_B_MSEL  		51
#define GPIO_MIPI_B_CS    		3

// ----------------------------------------------------------------------------
// I2C properties 
// ----------------------------------------------------------------------------

#define I2C1_SDA  66
#define I2C1_SCL  65

#define I2C2_SDA  68
#define I2C2_SCL  67

#define I2C1_SPEED_KHZ_DEFAULT  (50)
#define I2C1_ADDR_SIZE_DEFAULT  (ADDR_7BIT)

#define I2C2_SPEED_KHZ_DEFAULT  (50)
#define I2C2_ADDR_SIZE_DEFAULT  (ADDR_7BIT)

#define NUM_I2C_DEVICES                 (2)
// 

                                       
// ----------------------------------------------------------------------------
// LED Definitions
// ----------------------------------------------------------------------------



// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
typedef enum
{
   ToshibaChip1 = 0,
   ToshibaChip2,
   ToshibaChip3,
   ToshibaCam1,
   ToshibaCam2
} TBrd171I2cDevicesEnum;
// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // BRD_MV0171_DEF_H

