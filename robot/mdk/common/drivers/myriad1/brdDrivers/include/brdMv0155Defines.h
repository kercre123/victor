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

#ifndef BRD_MV0155_DEF_H
#define BRD_MV0155_DEF_H 

#include "DrvGpioDefines.h"
#include "DrvI2cMasterDefines.h"


// 1: Defines
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// LED 
// ----------------------------------------------------------------------------

#define LED0 80
#define LED1 79
#define LED2 78
#define LED3 77
#define LED4 74

#define NO_LEDS 5
// ----------------------------------------------------------------------------
// I2C Addresses
// ----------------------------------------------------------------------------

#define TSH_MIPI_I2C_ADR   0x07

#define TSH_CAM_I2C_ADR    0x10
// ----------------------------------------------------------------------------
// MIPI GPIO defines
// ----------------------------------------------------------------------------
#define CAM1_CS     141
#define CAM1_MSEL   98
#define CAM1_RESX   127
#define CAM1_MCLK   114

#define CAM2_CS     3
#define CAM2_MSEL   112
#define CAM2_RESX   113
#define CAM2_MCLK   13

#define LCD1_CS     26   // lcd1_hs
#define LCD1_MSEL   126
#define LCD1_RESX   63
#define LCD1_MCLK   64   // io_clkl, mode 0

// ----------------------------------------------------------------------------
// I2C properties 
// ----------------------------------------------------------------------------
#define I2C2_SCL    67  // mode 0
#define I2C2_SDA1   68  // mode 0
#define I2C2_SDA2   75  // mode 4
#define I2C2_SDA3   81  // mode 1

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
} TBrd155I2cDevicesEnum;


// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // BRD_MV0155_DEF_H

