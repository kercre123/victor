///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Default GPIO configuration for the MV0153 Board
///
/// Using the structure defined by this board it is possible to initialise
/// all the GPIOS on the MV0162 PCB to good safe initial defauls
/// 
#ifndef BRD_MV0162_GPIO_DEFAULTS_H_
#define BRD_MV0162_GPIO_DEFAULTS_H_ 

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


// 3: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 4: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

const drvGpioInitArrayType brdMv0162GpioCfgDefault =
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
    {64, 64 , ACTION_UPDATE_ALL          // MCLK C
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_0            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

    },      
    {76, 76 , ACTION_UPDATE_ALL          // A MSEL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
 
    },      
    {74, 74 , ACTION_UPDATE_ALL          // B MSEL
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
  
    },      
    {52, 52 , ACTION_UPDATE_ALL          // msel c
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
    {75, 75 , ACTION_UPDATE_ALL          //  A Resx
            ,
              PIN_LEVEL_HIGH               // not in reset
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

    },      
    {46, 46 , ACTION_UPDATE_ALL          //  B Resx
            ,
              PIN_LEVEL_HIGH               // not in reset
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
   
    },      
    {49, 49 , ACTION_UPDATE_ALL          //  C Resx
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
    //  External MIPI bridges config gpio
    // 
    // -----------------------------------------------------------------------
    {54, 54 , ACTION_UPDATE_ALL          //  AT Resx
            ,
              PIN_LEVEL_HIGH               // not in reset
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

    },      
    {56, 56 , ACTION_UPDATE_ALL          //  BT Resx
            ,
              PIN_LEVEL_HIGH               // not in reset
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
   
    },      
    {58, 58 , ACTION_UPDATE_ALL          //  CT Resx
            ,
              PIN_LEVEL_HIGH               // not in reset
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

    },      
    {55, 55 , ACTION_UPDATE_ALL          //  AT MSEL
            ,
              PIN_LEVEL_HIGH               // csi out 
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
 
    },      
    {57, 57 , ACTION_UPDATE_ALL          //  BT MSEL
            ,
              PIN_LEVEL_HIGH               // csi out 
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
   
    },      
    {63, 63 , ACTION_UPDATE_ALL          //  CT MSEL
            ,
              PIN_LEVEL_LOW               // csi in 
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
  
    },      
    {77, 77 , ACTION_UPDATE_ALL          //  AT CS
            ,
              PIN_LEVEL_HIGH               // not selected
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },      
    {102, 102 , ACTION_UPDATE_ALL        //  BT CS
            ,
              PIN_LEVEL_HIGH               // not selected
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
 
    },      
    {73, 73 , ACTION_UPDATE_ALL          //  CT CS
            ,
              PIN_LEVEL_HIGH             // not selected
            ,
              D_GPIO_MODE_7            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

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
    // -----------------------------------------------------------------------
    // LCD1 Configuration 
    // 
    // -----------------------------------------------------------------------
    {24, 25 , ACTION_UPDATE_ALL          //   vs pclk
            ,
              PIN_LEVEL_NA              
            ,
              D_GPIO_MODE_0            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
 
    },          
    {27, 43 , ACTION_UPDATE_ALL          //  DE, D0 - D15
            ,
              PIN_LEVEL_NA              
            ,
              D_GPIO_MODE_0            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

    },          
    {47, 48 , ACTION_UPDATE_ALL          //  d16 d17
            ,
              PIN_LEVEL_NA              
            ,
              D_GPIO_MODE_0            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
  
    },          
    {44, 45 , ACTION_UPDATE_ALL          //  d18 d19
            ,
              PIN_LEVEL_NA              
            ,
              D_GPIO_MODE_2            |
              D_GPIO_DIR_OUT           | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS

    },          
    {59, 62 , ACTION_UPDATE_ALL          //  D20 - D23
            ,
              PIN_LEVEL_NA              
            ,
              D_GPIO_MODE_4           |
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

#endif // BRD_MV0153_GPIO_DEFAULTS_H_
