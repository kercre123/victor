///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Default GPIO configuration for the MV0153 Board
///
/// Using the structure defined by this board it is possible to initialise
/// all the GPIOS on the MV0171 PCB to good safe initial defauls
/// 
#ifndef BRD_MV0171_GPIO_DEFAULTS_H_
#define BRD_MV0171_GPIO_DEFAULTS_H_

// 1: Includes             
// ----------------------------------------------------------------------------

// 2: Defines
// ----------------------------------------------------------------------------
#define LOCAL_D_GPIO_DEFAULTS              (D_GPIO_DATA_INV_OFF      |  \
                                            D_GPIO_WAKEUP_OFF        |  \
                                            D_GPIO_IRQ_SRC_NONE )

#define LOCAL_D_GPIO_PAD_DEFAULTS          (D_GPIO_PAD_NO_PULL       |  \
                                            D_GPIO_PAD_DRIVE_2mA     |  \
                                            D_GPIO_PAD_VOLT_1V8      |  \
                                            D_GPIO_PAD_SLEW_SLOW     |  \
                                            D_GPIO_PAD_SCHMITT_OFF   |  \
                                            D_GPIO_PAD_RECEIVER_ON   |  \
                                            D_GPIO_PAD_BIAS_2V5      |  \
                                            D_GPIO_PAD_LOCALCTRL_OFF |  \
                                            D_GPIO_PAD_LOCALDATA_LO  |  \
                                            D_GPIO_PAD_LOCAL_PIN_OUT)   

#define LOCAL_D_GPIO_PAD_DEFAULTS1_8CAM    (D_GPIO_PAD_NO_PULL       |  \
                                            D_GPIO_PAD_DRIVE_2mA     |  \
                                            D_GPIO_PAD_VOLT_1V8      |  \
                                            D_GPIO_PAD_SLEW_SLOW     |  \
                                            D_GPIO_PAD_SCHMITT_ON    |  \
                                            D_GPIO_PAD_RECEIVER_ON   |  \
                                            D_GPIO_PAD_BIAS_2V5      |  \
                                            D_GPIO_PAD_LOCALCTRL_OFF |  \
                                            D_GPIO_PAD_LOCALDATA_LO  |  \
                                            D_GPIO_PAD_LOCAL_PIN_OUT)

#define LOCAL_D_GPIO_PAD_DEFAULTS2_5       (D_GPIO_PAD_NO_PULL       |  \
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

const drvGpioInitArrayType brdMv0171GpioCfgDefault =
{
    // -----------------------------------------------------------------------
    // I2C1 Configuration 
    // -----------------------------------------------------------------------
    {65, 66 , ACTION_UPDATE_ALL          // I2C1 Pins.. WARNING: 0xC implies bit 10 (reserved is set)
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select I2C Mode
              D_GPIO_DIR_IN            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },      
    // -----------------------------------------------------------------------
    // I2C2 Configuration 
    // -----------------------------------------------------------------------
    {67, 68 , ACTION_UPDATE_ALL          // I2C2 Pins.. WARNING: 0xC implies bit 10 (reserved is set)
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select I2C Mode
              D_GPIO_DIR_IN            | 
              D_GPIO_DATA_INV_OFF      |  
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE 
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },      
    // -----------------------------------------------------------------------
    // MIPI gpio cfg Configuration 
    // 
    // -----------------------------------------------------------------------
   {114, 114, ACTION_UPDATE_ALL          // MCLK A
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_0            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

    },      
    {13, 13 , ACTION_UPDATE_ALL          // MCLK B
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_4            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

    },
    {112, 112 , ACTION_UPDATE_ALL          // A MSEL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
 
    },      
    {51, 51 , ACTION_UPDATE_ALL          // B MSEL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
  
    },      
    {23, 23 , ACTION_UPDATE_ALL          // msel c
            ,
              PIN_LEVEL_HIGH               // this is set as csi2 out paralel in
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

    },      
    {85, 85 , ACTION_UPDATE_ALL          //  A CS
            ,
              PIN_LEVEL_HIGH
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

    },      
    {3, 3 , ACTION_UPDATE_ALL          //  B CS
            ,
              PIN_LEVEL_HIGH
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
  
    },      
    {26, 26 , ACTION_UPDATE_ALL          //  C CS
            ,
              PIN_LEVEL_HIGH               // not selecteed
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

    },      
    {113,113, ACTION_UPDATE_ALL          //  A Resx
            ,
              PIN_LEVEL_HIGH               // not in reset
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

    },      
    {53, 53 , ACTION_UPDATE_ALL          //  B Resx
            ,
              PIN_LEVEL_HIGH               // not in reset
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
   
    },      
    {22, 22 , ACTION_UPDATE_ALL          //  C Resx
            ,
              PIN_LEVEL_HIGH               // not in reset
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },

    // -----------------------------------------------------------------------
    // CAM Configuration
    // -----------------------------------------------------------------------
     {0, 2, ACTION_UPDATE_ALL      // pclk, hs, vs
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_1             |
              D_GPIO_DIR_IN             |
              LOCAL_D_GPIO_DEFAULTS
            ,
              LOCAL_D_GPIO_PAD_DEFAULTS1_8CAM
    },
    {58, 58, ACTION_UPDATE_ALL      // pwdn
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7              |
              D_GPIO_DIR_OUT             |
              LOCAL_D_GPIO_DEFAULTS
            ,
              LOCAL_D_GPIO_PAD_DEFAULTS1_8CAM
    },
    {4, 11, ACTION_UPDATE_ALL      // d0 - d7
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_1             |
              D_GPIO_DIR_IN             |
              LOCAL_D_GPIO_DEFAULTS
            ,
              LOCAL_D_GPIO_PAD_DEFAULTS1_8CAM
    },
    {14, 15 , ACTION_UPDATE_ALL      // d8 d9
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_1             |
              D_GPIO_DIR_IN             |
              LOCAL_D_GPIO_DEFAULTS
            ,
              LOCAL_D_GPIO_PAD_DEFAULTS1_8CAM
    },
    // -----------------------------------------------------------------------
    // LEDS Configuration
    // -----------------------------------------------------------------------
    {74, 74, ACTION_UPDATE_ALL      // Power LED
            ,
              PIN_LEVEL_LOW         // off
            ,
              D_GPIO_MODE_7             |
              D_GPIO_DIR_OUT            |
              LOCAL_D_GPIO_DEFAULTS
            ,
              LOCAL_D_GPIO_PAD_DEFAULTS2_5
    },
    {57, 57, ACTION_UPDATE_ALL      // LED1
            ,
              PIN_LEVEL_LOW         // off
            ,
              D_GPIO_MODE_7             |
              D_GPIO_DIR_OUT            |
              LOCAL_D_GPIO_DEFAULTS
            ,
              LOCAL_D_GPIO_PAD_DEFAULTS2_5
    },
    {12, 12, ACTION_UPDATE_ALL      // LED2
            ,
              PIN_LEVEL_LOW         // off
            ,
              D_GPIO_MODE_7             |
              D_GPIO_DIR_OUT            |
              LOCAL_D_GPIO_DEFAULTS
            ,
              LOCAL_D_GPIO_PAD_DEFAULTS2_5
    },
	{56, 56, ACTION_UPDATE_ALL      // LED3
            ,
              PIN_LEVEL_LOW         // off
            ,
              D_GPIO_MODE_7             |
              D_GPIO_DIR_OUT            |
              LOCAL_D_GPIO_DEFAULTS
            ,
              LOCAL_D_GPIO_PAD_DEFAULTS2_5
    },
    {52, 52, ACTION_UPDATE_ALL      // LED4
            ,
              PIN_LEVEL_LOW         // off
            ,
              D_GPIO_MODE_7             |
              D_GPIO_DIR_OUT            |
              LOCAL_D_GPIO_DEFAULTS
            ,
              LOCAL_D_GPIO_PAD_DEFAULTS2_5
    },

    // -----------------------------------------------------------------------
    // LCD2 Configuration 
    // 
    // -----------------------------------------------------------------------
    {100, 101 , ACTION_UPDATE_ALL          //  pclk vs
            ,
              PIN_LEVEL_NA              
            ,
              D_GPIO_MODE_0            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
   
    },          
    {103, 111 , ACTION_UPDATE_ALL          //  DE, D0 - D7
            ,
              PIN_LEVEL_NA              
            ,
              D_GPIO_MODE_0            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
  
    },
    {93, 96 , ACTION_UPDATE_ALL          //  D8 - D11
                ,
                  PIN_LEVEL_NA
                ,
                  D_GPIO_MODE_4            |
                  D_GPIO_DIR_OUT           |
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

#endif // BRD_MV0171_GPIO_DEFAULTS_H_
