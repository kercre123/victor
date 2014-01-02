///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Default GPIO configuration for the MV0153 Board
///
/// Using the structure defined by this board it is possible to initialise
/// GPIOS on the MV0153 for the connection to MV0167 to good safe initial defauls
/// 
#ifndef BRD_MV0167_GPIO_DEFAULTS_H_
#define BRD_MV0167_GPIO_DEFAULTS_H_ 

// 1: Includes             
// ----------------------------------------------------------------------------

// 2: Defines
// ----------------------------------------------------------------------------
#define LOCAL_D_GPIO_DEFAULTS              (D_GPIO_DATA_INV_OFF      |  \
                                            D_GPIO_WAKEUP_OFF        |  \
                                            D_GPIO_IRQ_SRC_NONE )

#define LOCAL_D_GPIO_PAD_DEFAULTS          (D_GPIO_PAD_NO_PULL       |  \
                                            D_GPIO_PAD_DRIVE_2mA     |  \
                                            D_GPIO_PAD_VOLT_2V5      |  \
                                            D_GPIO_PAD_SLEW_SLOW     |  \
                                            D_GPIO_PAD_SCHMITT_OFF   |  \
                                            D_GPIO_PAD_RECEIVER_ON   |  \
                                            D_GPIO_PAD_BIAS_2V5      |  \
                                            D_GPIO_PAD_LOCALCTRL_OFF |  \
                                            D_GPIO_PAD_LOCALDATA_LO  |  \
                                            D_GPIO_PAD_LOCAL_PIN_OUT)   											
// 3: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 4: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

const drvGpioInitArrayType brdMv0167GpioCfgDefault =
{
    // -----------------------------------------------------------------------
    // Wifi IRQ SPI and Reset configuration
    // -----------------------------------------------------------------------
    {54, 54 , ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_7            |  // Used as gpio IRQ
              D_GPIO_DIR_IN            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },
    {67, 67 , ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_2            |  // Mode 2 to select SPI Mode
              LOCAL_D_GPIO_DEFAULTS
            ,
              LOCAL_D_GPIO_PAD_DEFAULTS
    },
   {69, 71, ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_0            |  // Mode 2 to select SPI Mode
              LOCAL_D_GPIO_DEFAULTS
            ,
              LOCAL_D_GPIO_PAD_DEFAULTS
    },
    {105, 105 , ACTION_UPDATE_ALL
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_7            |  // Used as gpio for reseting rsiWifi
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            ,
              LOCAL_D_GPIO_PAD_DEFAULTS
    },
    // -----------------------------------------------------------------------
    // Finally we terminate the Array
    // -----------------------------------------------------------------------
    {0, 0   , ACTION_TERMINATE_ARRAY      // Do nothing but simply termintate the Array
            ,
              PIN_LEVEL_LOW               // Won't actually be updated
            ,
              D_GPIO_MODE_0               // Won't actually be updated 
            ,
              D_GPIO_PAD_DEFAULTS         // Won't actually be updated 
    },
};

#endif // BRD_MV0167_GPIO_DEFAULTS_H_ 
