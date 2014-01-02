///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Default GPIO configuration for the VAPLAT Board
///
/// Using the structure defined by this board it is possible to initialise
/// all the GPIOS on the VAPLAT PCB to good safe initial defauls
///
#ifndef BRD_VXPLAT_GPIO_DEFAULTS_H_
#define BRD_VXPLAT_GPIO_DEFAULTS_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "brdVxplatDefines.h"

// 2: Defines
// ----------------------------------------------------------------------------

// 3: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 4: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------
// -----------------------------------------------------------------------

/////////////////////common
const drvGpioInitArrayType brdVxplatGpioCfgDefault =
{
    // -----------------------------------------------------------------------
    // I2C1 Configuration
    // -----------------------------------------------------------------------
    {VXPLAT_I2C1_SCL_PIN
            ,
              VXPLAT_I2C1_SDA_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select I2C Mode
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_8mA     |  // 8mA
              D_GPIO_PAD_VOLT_1V8      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // -----------------------------------------------------------------------
    // CAM1 Configuration
    //
    // -----------------------------------------------------------------------
    // CAM1_MCLK
    {VXPLAT_CAM1_MCLK_PIN
            ,
              VXPLAT_CAM1_MCLK_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select CAM1
              D_GPIO_DIR_OUT           |  // Output selected, not really necessary
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_8mA     |  // 8mA drive for the clock
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for Camera
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM1_PCLK
    {VXPLAT_CAM1_PCLK_PIN
            ,
              VXPLAT_CAM1_PCLK_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select CAM1
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for Camera
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_ON    |  // Schmitt Enabled
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM1_VSYNC
    {VXPLAT_CAM1_VSYNC_PIN
            ,
              VXPLAT_CAM1_VSYNC_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select CAM1
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_OFF      |  // Pin is inverted
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for Camera
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_ON    |  // Schmitt Enabled
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM1_HSYNC
    {VXPLAT_CAM1_HSYNC_PIN
            ,
              VXPLAT_CAM1_HSYNC_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select CAM1
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for Camera
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_ON    |  // Schmitt Enabled
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM1 Data 0-7
    {VXPLAT_CAM1_DAT0_PIN
            ,
              VXPLAT_CAM1_DAT7_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select CAM1
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for Camera
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_ON    |  // Schmitt Enabled
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM1 Data 8-11
    {VXPLAT_CAM1_DAT8_PIN
            ,
              VXPLAT_CAM1_DAT11_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_5            |  // Mode 5 to select CAM1
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for Camera
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_ON    |  // Schmitt Enabled
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // -----------------------------------------------------------------------
    // CAM2 Configuration
    //
    // -----------------------------------------------------------------------
    // CAM2_MCLK
    {VXPLAT_CAM2_MCLK_PIN
            ,
              VXPLAT_CAM2_MCLK_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_4            |  // Mode 4 to select CAM2_MCLK
              D_GPIO_DIR_OUT           |  // Output selected, not really necessary
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_8mA     |  // 8mA drive for the clock
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for Camera
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM2_PCLK
    {VXPLAT_CAM2_PCLK_PIN
            ,
              VXPLAT_CAM2_PCLK_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_1            |  // Mode 1 to select CAM2
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for Camera
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_ON    |  // Schmitt Enabled
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM2_VSYNC
    {VXPLAT_CAM2_VSYNC_PIN
            ,
              VXPLAT_CAM2_VSYNC_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_1            |  // Mode 1 to select CAM2
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_ON       |  // Pin is inverted !!!
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for Camera
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_ON    |  // Schmitt Enabled
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM2_HSYNC
    {VXPLAT_CAM2_HSYNC_PIN
            ,
              VXPLAT_CAM2_HSYNC_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_1            |  // Mode 1 to select CAM2
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for Camera
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_ON    |  // Schmitt Enabled
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM2 Data 0-7
    {VXPLAT_CAM2_DAT0_PIN
            ,
              VXPLAT_CAM2_DAT7_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_1            |  // Mode 1 to select CAM2
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for Camera
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_ON    |  // Schmitt Enabled
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM2 Data 8-11
    {VXPLAT_CAM2_DAT8_PIN
            ,
              VXPLAT_CAM2_DAT11_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_1            |  // Mode 1 to select CAM2
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for Camera
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_ON    |  // Schmitt Enabled
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM2 Reset Control
    {VXPLAT_CAM2_RESET_PIN
            ,
              VXPLAT_CAM2_RESET_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW              // Needs to be low according to the power up sequence
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |  // Driven Out
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for the reset signal (WARNING: This wasn't in original)
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // -----------------------------------------------------------------------
    // LCD1 Configuration
    //
    // -----------------------------------------------------------------------
    // LCD1_VSYNC
    {VXPLAT_LCD1_VSYNC_PIN
            ,
              VXPLAT_LCD1_VSYNC_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Default to driving low
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select LCD
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_ON       |  // Inverted
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
    },
    // LCD1_PCLK
    {VXPLAT_LCD1_PCLK_PIN
            ,
              VXPLAT_LCD1_PCLK_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Default to driving low
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select LCD
              D_GPIO_DIR_OUT           |
              D_GPIO_DATA_INV_OFF      |  // Inverted
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_4mA     |
              D_GPIO_PAD_VOLT_2V5      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_OFF  |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // LCD1_HSYNC
    {VXPLAT_LCD1_HSYNC_PIN
            ,
              VXPLAT_LCD1_HSYNC_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Default to driving low
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select LCD
              D_GPIO_DIR_IN            |  // WARNING: NOT PER THE ORGINAL
              D_GPIO_DATA_INV_ON       |  // Inverted
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
    },
    // LCD1_DATAEN
    {VXPLAT_LCD1_DATAEN_PIN
            ,
              VXPLAT_LCD1_DATAEN_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Default to driving low
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select LCD
              D_GPIO_DIR_OUT           |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
    },
    // LCD1 D0 -> D15
    {VXPLAT_LCD1_DATA0_PIN
            ,
              VXPLAT_LCD1_DATA15_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Default to driving low
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select LCD
              D_GPIO_DIR_IN            |  // WARNING: NOT PER THE ORGINAL
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_DEFAULTS      |  // Uses the default PAD configuration
              D_GPIO_PAD_SLEW_FAST        // But with fast slew rate
    },
    // LCD1 D16 -> D17
    {VXPLAT_LCD1_DATA16_PIN
            ,
              VXPLAT_LCD1_DATA17_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Default to driving low
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select LCD
              D_GPIO_DIR_IN            |  // WARNING: NOT PER THE ORGINAL
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_DEFAULTS      |  // Uses the default PAD configuration
              D_GPIO_PAD_SLEW_FAST        // But with fast slew rate
    },
    // LCD1 D19 -> D18
    {VXPLAT_LCD1_DATA19_PIN
            ,
              VXPLAT_LCD1_DATA18_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Default to driving low
            ,
              D_GPIO_MODE_2            |  // Mode 2 to select LCD
              D_GPIO_DIR_IN            |  // WARNING: NOT PER THE ORGINAL
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_DEFAULTS      |  // Uses the default PAD configuration
              D_GPIO_PAD_SLEW_FAST        // But with fast slew rate
    },
    // LCD1 D20 -> D23
    {VXPLAT_LCD1_DATA20_PIN
            ,
              VXPLAT_LCD1_DATA23_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Default to driving low
            ,
              D_GPIO_MODE_4            |  // Mode 4 to select LCD
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_DEFAULTS      |  // Uses the default PAD configuration
              D_GPIO_PAD_SLEW_FAST        // But with fast slew rate
    },
    // HDMI_RESET_N GPIO
    {VXPLAT_HDMI_RESETN_PIN
            ,
              VXPLAT_HDMI_RESETN_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Default to holding in reset for now
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |  // Drive out
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
    },
    // -----------------------------------------------------------------------
    // SPI Flash Interface
    // -----------------------------------------------------------------------
    {VXPLAT_SPI1_MOSI_PIN
            ,
              VXPLAT_SPI1_SS_OUT_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_0            |  // Mode0 SPI1)
              D_GPIO_DIR_OUT           |  // Drive out
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
    },
    // -----------------------------------------------------------------------
    // SPI 3
    // -----------------------------------------------------------------------
    {VXPLAT_SPI3_MOSI_PIN
            ,
              VXPLAT_SPI3_SCLK_IN_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_3            |  // Mode3 SPI3)
              D_GPIO_DIR_OUT           |  // Pin direction doesn't matter
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    {VXPLAT_SPI3_SS_IN_PIN
            ,
            VXPLAT_SPI3_SS_IN_PIN
            ,
            ACTION_UPDATE_ALL
            ,
            PIN_LEVEL_LOW
            ,
            D_GPIO_MODE_3            |  // Mode3 SPI3)
            D_GPIO_DIR_IN            |  // Pin direction matters! (this is for slave)
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_VOLT_1V8      |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // -----------------------------------------------------------------------
    // CEC Pin (Configure as an input for now, SW will need to control this pins enable
    // -----------------------------------------------------------------------
    {VXPLAT_HDMI_CEC_PIN
            ,
              VXPLAT_HDMI_CEC_PIN
            ,
              ACTION_UPDATE_ALL           // CEC Input Pin
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Direct GPIO mode
              D_GPIO_DIR_IN            |  // Input
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
    },
    // -----------------------------------------------------------------------
    // LCD2 Configuration (to LCD-CSI bridge)
    // -----------------------------------------------------------------------
    {VXPLAT_LCD2_DATA22_PIN
            ,
            VXPLAT_LCD2_DATA23_PIN
            ,
            ACTION_UPDATE_ALL // lcd2_data_[22,23]
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_3            | // LCD2 peripheral
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    {VXPLAT_LCD2_PCLK_PIN
            ,
            VXPLAT_LCD2_PCLK_PIN
            ,
            ACTION_UPDATE_ALL // lcd2_pclk
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_0            | // LCD2 peripheral
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_8mA     | // the pixel clock looks much better like this
            D_GPIO_PAD_SLEW_FAST     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    {VXPLAT_LCD2_VSYNC_PIN
            ,
            VXPLAT_LCD2_VSYNC_PIN
            ,
            ACTION_UPDATE_ALL // lcd2_vsync
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_0            | // LCD2 peripheral
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    {VXPLAT_LCD2_DATAEN_PIN
            ,
            VXPLAT_LCD2_DATA7_PIN
            ,
            ACTION_UPDATE_ALL // lcd2_data_en, lcd2_data_[0..7]
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_0            | // LCD2 peripheral
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    {VXPLAT_LCD2_DATA16_PIN
            ,
            VXPLAT_LCD2_DATA21_PIN
            ,
            ACTION_UPDATE_ALL // lcd2_data_[16-17,12-15,8-11,18-21]
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_3            | // LCD2 peripheral
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    {VXPLAT_CSI_RESX_PIN
            ,
            VXPLAT_CSI_RESX_PIN
            ,
            ACTION_UPDATE_ALL // this is MIPI_RESX, CSI bridge reset, active low
            ,
            PIN_LEVEL_LOW
            ,
            D_GPIO_MODE_7            |
            D_GPIO_DIR_OUT           |
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    {VXPLAT_CSI_CS_PIN
            ,
            VXPLAT_CSI_CS_PIN
            ,
            ACTION_UPDATE_ALL // this is MIPI_CS, connected to the CS pin of the CSI bridge
            ,                    // "Please tie/drive Low"
            PIN_LEVEL_LOW
            ,
            D_GPIO_MODE_7            |
            D_GPIO_DIR_OUT           |
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    {VXPLAT_FSYNC_PIN
            ,
            VXPLAT_FSYNC_PIN
            ,
            ACTION_UPDATE_ALL           // Fsync Output
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |  // Driven Out
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // -----------------------------------------------------------------------
    // Finally we terminate the Array
    // -----------------------------------------------------------------------
    {0,0    , ACTION_TERMINATE_ARRAY      // Do nothing but simply termintate the Array
            ,
              PIN_LEVEL_LOW               // Won't actually be updated
            ,
              D_GPIO_MODE_0               // Won't actually be updated
            ,
              D_GPIO_PAD_DEFAULTS         // Won't actually be updated
    },
};

const drvGpioInitArrayType extraVaplatGpioCfg =
{
    {VAPLAT_AP_IRQ_PIN
            ,
            VAPLAT_AP_IRQ_PIN
            ,
            ACTION_UPDATE_ALL
            ,
            PIN_LEVEL_LOW
            ,
            D_GPIO_MODE_7            |
            D_GPIO_DIR_OUT           |
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    // -----------------------------------------------------------------------
    // HDMI OUT ENABLE. Whitout this set high there is not data out to the HDMI
    // -----------------------------------------------------------------------
    {VXPLAT_HDMI_OUTPUT_EN
            ,
              VXPLAT_HDMI_OUTPUT_EN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_HIGH
            ,
              D_GPIO_MODE_7            |  // Mode 0 to select I2C Mode
              D_GPIO_DIR_OUT           |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_8mA     |  // 8mA
              D_GPIO_PAD_VOLT_2V5      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_OFF  |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // -----------------------------------------------------------------------
    // MIPI ENABLE for VDDC and VDD_MIPI. These must be enabled at the same time, or before VDDIO
    // -----------------------------------------------------------------------
    {VAPLAT_MIPI_1V2_EN
            ,
            VAPLAT_MIPI_1V2_EN
            ,
            ACTION_UPDATE_ALL
            ,
            PIN_LEVEL_HIGH
            ,
            D_GPIO_MODE_7            |  // Mode 0 to select I2C Mode
            D_GPIO_DIR_OUT           |
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_8mA     |  // 8mA
            D_GPIO_PAD_VOLT_1V8      |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_OFF  |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // -----------------------------------------------------------------------
    // MIPI ENABLE for VDDIO (1.8 V). This one has to be HIGH in order for I2C2 to work
    // Also, this must be enabled at the same time, or after VDDC and VDD_MIPI
    // -----------------------------------------------------------------------
    {VAPLAT_MIPI_1V8_EN
            ,
              VAPLAT_MIPI_1V8_EN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_HIGH
            ,
              D_GPIO_MODE_7            |  // Mode 0 to select I2C Mode
              D_GPIO_DIR_OUT            |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_8mA     |  // 8mA
              D_GPIO_PAD_VOLT_1V8       |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_OFF  |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // -----------------------------------------------------------------------
    // I2C2 Configuration
    // -----------------------------------------------------------------------
    {VXPLAT_I2C2_SCL_PIN(VAPLAT_BOARD)
            ,
              VXPLAT_I2C2_SCL_PIN(VAPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_4            |  // Mode 4 to select I2C Mode
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_ON       |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_8mA     |  // 8mA Drive strength (Max)
              D_GPIO_PAD_VOLT_1V8      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_ON    |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // -----------------------------------------------------------------------
    // I2C3 Configuration
    // -----------------------------------------------------------------------
    {VXPLAT_I2C3_SCL_PIN(VAPLAT_BOARD)
            ,
              VXPLAT_I2C3_SDA_PIN(VAPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 4 to select I2C Mode
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_ON       |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_8mA     |  // 8mA Drive strength (Max)
              D_GPIO_PAD_VOLT_2V5      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    //Cam1 flash; It's actually connected to the lm3642 STROBE pin
    {VAPLAT_CAM1_FLASH_PIN
            ,
              VAPLAT_CAM1_FLASH_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_8mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic just in case
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM1 Reset Control
    {VXPLAT_CAM1_RESET_PIN(VAPLAT_BOARD)
            ,
              VXPLAT_CAM1_RESET_PIN(VAPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW              // Needs to be low according to the power up sequence
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |  // Driven Out
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for the reset signal
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    //Cam1 Trig
    {VXPLAT_CAM1_TRIG_PIN(VAPLAT_BOARD)
            ,
              VXPLAT_CAM1_TRIG_PIN(VAPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // LM3642 TORCH PIN
    {VAPLAT_TORCH_EN_ISP_PIN
            ,
              VAPLAT_TORCH_EN_ISP_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic just in case
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    //Cam1 OE bar
    {VAPLAT_CAM1_OE_BAR_PIN
            ,
              VAPLAT_CAM1_OE_BAR_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    {VXPLAT_CAM2_TRIG_PIN(VAPLAT_BOARD)
            ,
              VXPLAT_CAM2_TRIG_PIN(VAPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM2 Standby
    {VXPLAT_CAM2_STANDBY_PIN(VAPLAT_BOARD)
            ,
              VXPLAT_CAM2_STANDBY_PIN(VAPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Drive Pin Low to make sure camera not in standby
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |  // Driven Out
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for the reset signal (WARNING: This wasn't in original)
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM2 Standby
    {VXPLAT_CAM2_OE_BAR_PIN(VAPLAT_BOARD)
            ,
              VXPLAT_CAM2_OE_BAR_PIN(VAPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Drive Pin Low to make sure camera not in standby
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |  // Driven Out
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for the reset signal (WARNING: This wasn't in original)
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    {VAPLAT_CAMB2_PWR_EN_2_PIN
            ,
              VAPLAT_CAMB1_PWR_EN_1_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    {VAPLAT_LCD2_MCLK_PIN
            ,
            VAPLAT_LCD2_MCLK_PIN
            ,
            ACTION_UPDATE_ALL // lcd2_mclk
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_0            | // LCD2 peripheral
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    // -----------------------------------------------------------------------
    // Finally we terminate the Array
    // -----------------------------------------------------------------------
    {0,0    , ACTION_TERMINATE_ARRAY      // Do nothing but simply termintate the Array
            ,
              PIN_LEVEL_LOW               // Won't actually be updated
            ,
              D_GPIO_MODE_0               // Won't actually be updated
            ,
              D_GPIO_PAD_DEFAULTS         // Won't actually be updated
    }
};

const drvGpioInitArrayType extraVcplatGpioCfg =
{
    {VCPLAT_AP_IRQ_PIN
            ,
            VCPLAT_AP_IRQ_PIN
            ,
            ACTION_UPDATE_ALL
            ,
            PIN_LEVEL_LOW
            ,
            D_GPIO_MODE_7            |
            D_GPIO_DIR_OUT           |
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    // -----------------------------------------------------------------------
    // FLASH LED Configuration
    // -----------------------------------------------------------------------
    {VCPLAT_REAR_LED_EN_PIN
            ,
              VCPLAT_REAR_LED_FLINH_PIN
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select I/O mode
              D_GPIO_DIR_OUT           |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_8mA     |  // 8mA
              D_GPIO_PAD_VOLT_1V8       |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_OFF  |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // -----------------------------------------------------------------------
    // SDHost/SDcard Interface
    //
    // -----------------------------------------------------------------------
    {85,90  , ACTION_UPDATE_ALL           // SDCard clk,cmd,D0-3
            ,
              PIN_LEVEL_LOW               // Default to driving low
            ,
              D_GPIO_MODE_2            |  // Mode 2 to select SD_HST1 Interface
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
    },
    {92,92  , ACTION_UPDATE_ALL           // SDCard Card Detect
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Direct Mode as we use a GPIO for this
              D_GPIO_DIR_IN            |  // Input
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
    },
    // -----------------------------------------------------------------------
    // I2C2 Configuration
    // -----------------------------------------------------------------------
    {VXPLAT_I2C2_SCL_PIN(VCPLAT_BOARD)
            ,
              VXPLAT_I2C2_SDA_PIN(VCPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_4            |  // Mode 4 to select I2C Mode
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_ON       |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_8mA     |  // 8mA Drive strength (Max)
              D_GPIO_PAD_VOLT_2V5      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // -----------------------------------------------------------------------
    // I2C3 Configuration
    // -----------------------------------------------------------------------
    {VXPLAT_I2C3_SCL_PIN(VCPLAT_BOARD)
            ,
              VXPLAT_I2C3_SDA_PIN(VCPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 4 to select I2C Mode
              D_GPIO_DIR_IN            |
              D_GPIO_DATA_INV_ON       |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_8mA     |  // 8mA Drive strength (Max)
              D_GPIO_PAD_VOLT_1V8      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM1 Reset Control
    {VXPLAT_CAM1_RESET_PIN(VCPLAT_BOARD)
            ,
              VXPLAT_CAM1_RESET_PIN(VCPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW              // Needs to be low according to the power up sequence
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |  // Driven Out
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for the reset signal
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    //Cam1 Trig
    {VXPLAT_CAM1_TRIG_PIN(VCPLAT_BOARD)
            ,
              VXPLAT_CAM1_TRIG_PIN(VCPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    {VCPLAT_CSI_CAM_R_3M_RESX_PIN
            ,
            VCPLAT_CSI_CAM_R_3M_RESX_PIN
            ,
            ACTION_UPDATE_ALL // this is MIPI_RESX, CSI bridge reset, active low
            ,
            PIN_LEVEL_LOW
            ,
            D_GPIO_MODE_7            |
            D_GPIO_DIR_OUT           |
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    {VXPLAT_CAM2_TRIG_PIN(VCPLAT_BOARD)
            ,
              VXPLAT_CAM2_TRIG_PIN(VCPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM2 Standby
    {VXPLAT_CAM2_STANDBY_PIN(VCPLAT_BOARD)
            ,
              VXPLAT_CAM2_STANDBY_PIN(VCPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Drive Pin Low to make sure camera not in standby
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |  // Driven Out
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for the reset signal (WARNING: This wasn't in original)
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    // CAM2 Standby
    {VXPLAT_CAM2_OE_BAR_PIN(VCPLAT_BOARD)
            ,
              VXPLAT_CAM2_OE_BAR_PIN(VCPLAT_BOARD)
            ,
              ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_LOW               // Drive Pin Low to make sure camera not in standby
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_OUT           |  // Driven Out
              D_GPIO_DATA_INV_OFF      |
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE
            ,
              D_GPIO_PAD_NO_PULL       |
              D_GPIO_PAD_DRIVE_2mA     |
              D_GPIO_PAD_VOLT_1V8      |  // 1.8V Logic for the reset signal (WARNING: This wasn't in original)
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT
    },
    {VCPLAT_CSI_CAM_R_3M_CS_PIN
            ,
            VCPLAT_CSI_CAM_R_3M_CS_PIN
            ,
            ACTION_UPDATE_ALL // this is MIPI_CS, connected to the CS pin of the CSI bridge
            ,                    // "Please tie/drive Low"
            PIN_LEVEL_LOW
            ,
            D_GPIO_MODE_7            |
            D_GPIO_DIR_OUT           |
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    {VCPLAT_LCD2_REFCLK
            ,
            VCPLAT_LCD2_REFCLK
            ,
            ACTION_UPDATE_ALL
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_0            | // cpr io clock
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    {VXPLAT_ISP_GYRO_INT_PIN
            ,
            VXPLAT_ISP_GYRO_INT_PIN
            ,
            ACTION_UPDATE_ALL // this is MIPI_CS, connected to the CS pin of the CSI bridge
            ,                    // "Please tie/drive Low"
            PIN_LEVEL_LOW
            ,
            D_GPIO_MODE_7            |
            D_GPIO_DIR_IN            |
            D_GPIO_DATA_INV_OFF      |
            D_GPIO_WAKEUP_OFF        |
            D_GPIO_IRQ_SRC_NONE
            ,
            D_GPIO_PAD_NO_PULL       |
            D_GPIO_PAD_DRIVE_2mA     |
            D_GPIO_PAD_SLEW_SLOW     |
            D_GPIO_PAD_SCHMITT_OFF   |
            D_GPIO_PAD_RECEIVER_ON   |
            D_GPIO_PAD_BIAS_2V5      |
            D_GPIO_PAD_LOCALCTRL_OFF |
            D_GPIO_PAD_LOCALDATA_LO  |
            D_GPIO_PAD_LOCAL_PIN_OUT |
            D_GPIO_PAD_VOLT_1V8
    },
    // -----------------------------------------------------------------------
    // Finally we terminate the Array
    // -----------------------------------------------------------------------
    {0,0    , ACTION_TERMINATE_ARRAY      // Do nothing but simply termintate the Array
            ,
              PIN_LEVEL_LOW               // Won't actually be updated
            ,
              D_GPIO_MODE_0               // Won't actually be updated
            ,
              D_GPIO_PAD_DEFAULTS         // Won't actually be updated
    }
};

#endif // BRD_VXPLAT_GPIO_DEFAULTS_H_
