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

#ifndef BRD_MV0153_DEF_H
#define BRD_MV0153_DEF_H 

#include "DrvGpioDefines.h"
#include "DrvI2cMasterDefines.h"
#include "icRegulatorLT3906.h"
#include "icPllCDCE913.h"
#include "brdMv0153Defines.h"
#include "brdCommonDefines.h"

// 1: Defines
// ----------------------------------------------------------------------------

// API Constants
#define MV0153_LED_SUCCESS                  ( 0)
#define ERR_INVALID_LED                     (-1)

// ----------------------------------------------------------------------------
// I2C Addresses
// ----------------------------------------------------------------------------

#define MV0153_I2C1_REG_U15_8BW   (0xC0)
#define MV0153_I2C1_REG_U15_8BR   (MV0153_I2C1_REG_U15_8BR |  1)
#define MV0153_I2C1_REG_U15_7B    (MV0153_I2C1_REG_U15_8BW >> 1)
                 
#define MV0153_I2C1_TSC_ADC_8BW   (0x90)
#define MV0153_I2C1_TSC_ADC_8BR   (MV0153_I2C1_TSC_ADC_8BW |  1)
#define MV0153_I2C1_TSC_ADC_7B    (MV0153_I2C1_TSC_ADC_8BW >> 1)
                   
#define MV0153_I2C2_REG_U17_8BW   (0xC0)
#define MV0153_I2C2_REG_U17_8BR   (MV0153_I2C2_REG_U17_8BR |  1)
#define MV0153_I2C2_REG_U17_7B    (MV0153_I2C2_REG_U17_8BW >> 1)

#define MV0153_I2C2_CLK_GEN_8BW   (0xCA)
#define MV0153_I2C2_CLK_GEN_8BR   (MV0153_I2C2_CLK_GEN_8BW |  1)
#define MV0153_I2C2_CLK_GEN_7B    (MV0153_I2C2_CLK_GEN_8BW >> 1)

#define MV0153_I2C2_HDMI_TX_8BW   (0x98)
#define MV0153_I2C2_HDMI_TX_8BR   (MV0153_I2C2_HDMI_TX_8BW |  1)
#define MV0153_I2C2_HDMI_TX_7B    (MV0153_I2C2_HDMI_TX_8BW >> 1)

// This is a register address within the I2C Regulator block, used for testing               
#define MV0153_REG_BUCK_LDO_ENABLE (0x10)
#define MV0153_REG_BUCK_LDO_STATUS (0x11)

                                       
// ----------------------------------------------------------------------------
// LED Definitions
// ----------------------------------------------------------------------------

// optional use                        
#define LED_ON           (1)
#define LED_OFF          (0)

// optional use                        
#define MV0153_LED1      (1)
#define MV0153_LED2      (2) 
#define MV0153_LED3      (3) 
#define MV0153_LED4      (4) 
#define MV0153_LED5      (5) 
#define MV0153_LED6      (6) 
#define MV0153_LED7      (7) 
#define MV0153_LED8      (8) 

#define MV0153_R0R1_LED1_GPIO (3 )
#define MV0153_R0R1_LED2_GPIO (49)
#define MV0153_R0R1_LED3_GPIO (55)
#define MV0153_R0R1_LED4_GPIO (80)
#define MV0153_R0R1_LED5_GPIO (81)
#define MV0153_R0R1_LED6_GPIO (82)
#define MV0153_R0R1_LED7_GPIO (17)
#define MV0153_R0R1_LED8_GPIO (78)

#define MV0153_R2_LED1_GPIO  (3 )
#define MV0153_R2_LED2_GPIO  (49)
#define MV0153_R2_LED3_GPIO  (55)
#define MV0153_R2_LED4_GPIO  (17)  // NOTE: This is the key change.. In R1 this was GPIO 80!!

#define MV0153_R0R1_NUM_LEDS (4 ) // We artificially limit MV0153-R0,R1 to 4 LEDs to force SW compatibility with R2  
#define MV0153_R2_NUM_LEDS   (4 )

// Reset Pins
#define MV0153_HDMI_RESET_PIN           (23)

// Reset Flags
#define MV0153_RESET_ALL                (0xFFFFFFFF) // Selects all devices for reset
#define MV0153_HDMI_RESET_FLAG          (1 << 0)

// Voltage Control Pins                                
#define MV0153_CAMERA_POWER_EN_PIN      (53)
#define MV0153_CORE_VOLTAGE_PWM_PIN     (68)

#define MV0153_CLK_SEL_PIN 		        (46)
#define MV0153_LCD1_PCLK_PIN 	        (25)
#define MV0153_LCD1_PCLK_MODE_LCD 	    (D_GPIO_MODE_0)

// CEC Pin definition
#define MV0153_R0R1_HDMI_CEC_PIN        (76)
#define MV0153_R2_HDMI_CEC_PIN          (63)

// Infrared Pin definition
#define MV0153_INFRA_RED_PIN            (58)

// Voltage CONFIGURATIONS
#define MV0153_DEFAULT_CORE_VOLTAGE_NONUSB   (1230)   // Voltage for us in non-usb applications
#define MV0153_DEFAULT_CORE_VOLTAGE          (1190)   // General default core voltage

#define MV0153_I2C1_SCL_PIN            (65)
#define MV0153_I2C1_SDA_PIN            (66)
#define MV0153_I2C1_SPEED_KHZ_DEFAULT  (100)
#define MV0153_I2C1_ADDR_SIZE_DEFAULT  (ADDR_7BIT) 
        
#define MV0153_I2C2_SCL_PIN            (74)
#define MV0153_I2C2_SDA_PIN            (75)
#define MV0153_I2C2_SPEED_KHZ_DEFAULT  (100)
#define MV0153_I2C2_ADDR_SIZE_DEFAULT  (ADDR_7BIT) 

// PCB Revision Detection
#define MV0153_REV_DETECT_BIT0  (57)
#define MV0153_REV_DETECT_BIT1  (56)
#define MV0153_REV_DETECT_BIT2  (52)

#define MV0153_REV_CODE_R0R1    (0 )
#define MV0153_REV_CODE_R2      (1 )

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
typedef enum 
{
    DEV_I2C_M
} tyDevHandleID;

typedef enum
{
    MV0153_REV_NOT_INIT = 0,    // Initial state of the revision so we know it needs to be detected
    MV0153_R0R1         = 1,    // MV0153-R0 and R1 PCB revisions are electrically and software compatible so they return the same rev.
    MV0153_R2           = 2,
}tyMv0153PcbRevision;

typedef struct
{
  tyLT3906Config voltCfgU15;
  tyLT3906Config voltCfgU17;
} tyDcVoltageCfg;

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // BRD_MV0153_DEF_H

