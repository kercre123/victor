///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Default GPIO configuration for the MV0174 Daughter Board
///
/// Using the structure defined by this board it is possible to initialise
/// all the GPIOS on the MV0174 PCB to good safe initial defauls
/// 
#ifndef BRD_MV0174_GPIO_DEFAULTS_H_
#define BRD_MV0174_GPIO_DEFAULTS_H_

// 1: Includes             
// ----------------------------------------------------------------------------

#include "DrvGpio.h"
#include "icMipiTC358746Defines.h"

// 2: Defines
// ----------------------------------------------------------------------------
#define GPIO_MV0174_MIPI_CS   (113)
#define GPIO_MV0174_MIPI_RESX (112)
#define GPIO_MV0174_MIPI_MSEL (PIN_NOT_CONNECTED_TO_GPIO)

#define LOCAL_D_GPIO_DEFAULTS              (D_GPIO_DATA_INV_OFF      |  \
                                            D_GPIO_WAKEUP_OFF        |  \
                                            D_GPIO_IRQ_SRC_NONE )

#define LOCAL_D_GPIO_PAD_DEFAULTS          (D_GPIO_PAD_NO_PULL       |  \
                                            D_GPIO_PAD_DRIVE_2mA     |  \
                                            D_GPIO_PAD_SLEW_SLOW     |  \
                                            D_GPIO_PAD_SCHMITT_OFF   |  \
                                            D_GPIO_PAD_RECEIVER_ON   |  \
                                            D_GPIO_PAD_BIAS_2V5      |  \
                                            D_GPIO_PAD_LOCALCTRL_OFF |  \
                                            D_GPIO_PAD_LOCALDATA_LO  |  \
                                            D_GPIO_PAD_LOCAL_PIN_OUT)   
// Signals that talk to the CSI bride:
#define LOCAL_D_GPIO_PAD_DEFAULTS_MIPI     (LOCAL_D_GPIO_PAD_DEFAULTS|  \
                                            D_GPIO_PAD_VOLT_1V8)

// 3: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 4: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

const drvGpioInitArrayType brdMv0174GpioCfgDefault =
{
    // -----------------------------------------------------------------------
    // LCD2 Configuration (to LCD-CSI bridge)
    // -----------------------------------------------------------------------
    {97, 98 , ACTION_UPDATE_ALL // lcd2_data_[22,23]
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_3            | // LCD2 peripheral
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS_MIPI
    },
    {99, 99, ACTION_UPDATE_ALL // lcd2_mclk
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_0            | // LCD2 peripheral
            LOCAL_D_GPIO_DEFAULTS
            ,
            LOCAL_D_GPIO_PAD_DEFAULTS_MIPI
    },
    {100, 100, ACTION_UPDATE_ALL // lcd2_pclk
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_0            | // LCD2 peripheral
            LOCAL_D_GPIO_DEFAULTS
            ,
            LOCAL_D_GPIO_PAD_DEFAULTS_MIPI |
            D_GPIO_PAD_DRIVE_8mA           | // the pixel clock looks much better like this
            D_GPIO_PAD_SLEW_FAST
    },
    {101, 101, ACTION_UPDATE_ALL // lcd2_vsync
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_0            | // LCD2 peripheral
            LOCAL_D_GPIO_DEFAULTS
            ,
            LOCAL_D_GPIO_PAD_DEFAULTS_MIPI
    },
    {102, 102, ACTION_UPDATE_ALL // this is SPI_OE_N. It should be handled as open-collector
             ,
             PIN_LEVEL_NA
             ,
             D_GPIO_MODE_7         |
             D_GPIO_DIR_IN         |
             LOCAL_D_GPIO_DEFAULTS
             ,
             LOCAL_D_GPIO_PAD_DEFAULTS
    },
    {103, 111, ACTION_UPDATE_ALL // lcd2_data_en, lcd2_data_[0..7]
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_0            | // LCD2 peripheral
            LOCAL_D_GPIO_DEFAULTS
            ,
            LOCAL_D_GPIO_PAD_DEFAULTS_MIPI
    },
    {127, 140 , ACTION_UPDATE_ALL // lcd2_data_[16-17,12-15,8-11,18-21]
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_3            | // LCD2 peripheral
            LOCAL_D_GPIO_DEFAULTS
            ,
            LOCAL_D_GPIO_PAD_DEFAULTS_MIPI
    },
    {112, 112, ACTION_UPDATE_ALL // this is MIPI_RESX, CSI bridge reset, active low
            ,
            PIN_LEVEL_LOW
            ,
            D_GPIO_MODE_7         |
            D_GPIO_DIR_OUT         |
            LOCAL_D_GPIO_DEFAULTS
            ,
            LOCAL_D_GPIO_PAD_DEFAULTS_MIPI
    },
    {113, 113, ACTION_UPDATE_ALL // this is LCD2_DISABLE, connected to the CS pin of the CSI bridge
            ,                    // "Please tie/drive Low"
            PIN_LEVEL_LOW
            ,
            D_GPIO_MODE_7         |
            D_GPIO_DIR_OUT         |
            LOCAL_D_GPIO_DEFAULTS
            ,
            LOCAL_D_GPIO_PAD_DEFAULTS_MIPI
    },
    // -----------------------------------------------------------------------
    // I2C Configuration (to talk to LCD-CSI bridge) (indirect, through TXS0102)
    // -----------------------------------------------------------------------
    // this is already configured by Mv0153
    /*{74, 75, ACTION_UPDATE_ALL // I2C2_SCL, I2C2_SDA
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_4         |
            LOCAL_D_GPIO_DEFAULTS
            ,
            LOCAL_D_GPIO_PAD_DEFAULTS |
            D_GPIO_PAD_DRIVE_8mA},*/
    // -----------------------------------------------------------------------
    // HINT_N signal to application processor
    // -----------------------------------------------------------------------
    {54, 54, ACTION_UPDATE_ALL
            ,
            PIN_LEVEL_HIGH
            ,
            D_GPIO_MODE_7         |
            D_GPIO_DIR_OUT         |
            LOCAL_D_GPIO_DEFAULTS
            ,
            LOCAL_D_GPIO_PAD_DEFAULTS |
            D_GPIO_PAD_VOLT_1V8 // APQ runs at 1.8 Volts
    },
    // -----------------------------------------------------------------------
    // SPI(slave) connected to application processor
    // -----------------------------------------------------------------------
    {69, 71, ACTION_UPDATE_ALL // MOSI, MISO, SCLK
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_0 |
            LOCAL_D_GPIO_DEFAULTS
            ,
            LOCAL_D_GPIO_PAD_DEFAULTS
    },
    // Note: gpios 72 and 67 are connected together on The modified Mv0153 R2 that is needed
    //         to support the Mv0174 Daughter Card
    {72, 72, ACTION_UPDATE_ALL // SS
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_0 |
            D_GPIO_DIR_IN         | // Note: the gpio direction matters here
            LOCAL_D_GPIO_DEFAULTS
            ,
            LOCAL_D_GPIO_PAD_DEFAULTS
    },
    {67, 67, ACTION_UPDATE_ALL // SPI1_SS_OUT1, configured as GPIO input
            ,
            PIN_LEVEL_NA
            ,
            D_GPIO_MODE_7         |
            D_GPIO_DIR_IN         |
            LOCAL_D_GPIO_DEFAULTS
            ,
            LOCAL_D_GPIO_PAD_DEFAULTS
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

#endif // BRD_MV0174_GPIO_DEFAULTS_H_
