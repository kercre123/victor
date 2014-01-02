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

#ifndef BRD_MV0163_DEF_H
#define BRD_MV0163_DEF_H 

#include "DrvGpioDefines.h"
#include "DrvI2cMasterDefines.h"


// 1: Defines
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// LED 
// ----------------------------------------------------------------------------

#define LED0 39
#define LED1 55
#define LED2 21
#define LED3 45

#define NO_LEDS 4
// ----------------------------------------------------------------------------
// I2C Addresses
// ----------------------------------------------------------------------------

#define TSH_MIPI_I2C_ADR   0x07
#define TSH_CAM_I2C_ADR    0x10

// ----------------------------------------------------------------------------
// MIPI GPIO defines
// ----------------------------------------------------------------------------

// defines for bridge chips in Katana
#define GPIO_MIPI_A_PCLK  115
#define GPIO_MIPI_A_HS    117
#define GPIO_MIPI_A_VS    116
#define GPIO_MIPI_A_D0    118
#define GPIO_MIPI_A_D7    125
#define GPIO_MIPI_A_MCLK  114
#define GPIO_MIPI_A_RESX  75
#define GPIO_MIPI_A_MSEL  98
#define GPIO_MIPI_A_CS    85

#define GPIO_MIPI_B_PCLK  0
#define GPIO_MIPI_B_HS    1
#define GPIO_MIPI_B_VS    2
#define GPIO_MIPI_B_D0    4
#define GPIO_MIPI_B_D7    11
#define GPIO_MIPI_B_MCLK  13
#define GPIO_MIPI_B_RESX  53
#define GPIO_MIPI_B_MSEL  51
#define GPIO_MIPI_B_CS    3

#define GPIO_MIPI_C_PCLK  25
#define GPIO_MIPI_C_DE    27
#define GPIO_MIPI_C_VS    24
#define GPIO_MIPI_C_D0    28
#define GPIO_MIPI_C_D7    35
#define GPIO_MIPI_C_MCLK  64
#define GPIO_MIPI_C_RESX  22
#define GPIO_MIPI_C_MSEL  52
#define GPIO_MIPI_C_CS    26

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
} TBrd163I2cDevicesEnum;
// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // BRD_MV0163_DEF_H

