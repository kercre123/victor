///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Default GPIO configuration for the MV0153 Board
///
/// Using the structure defined by this board it is possible to initialise
/// all the GPIOS on the MV0153 PCB to good safe initial defauls
/// 
#ifndef BRD_MV0172_GPIO_DEFAULTS_H_
#define BRD_MV0172_GPIO_DEFAULTS_H_ 

// 1: Includes             
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// 2: Defines
// ----------------------------------------------------------------------------

// 3: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 4: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

const drvGpioInitArrayType brdMv0172GpioCfgDefault =
{                                              
    // -----------------------------------------------------------------------
    // CAM1 Extra Configuration 
    // 
    // -----------------------------------------------------------------------
	{93,96, ACTION_UPDATE_ALL           // CAM1 Data 8-11
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_5           |  // Mode 5 to select CAM1
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
    {91 ,91 , ACTION_UPDATE_ALL           // CAM1 Standby
            ,
              PIN_LEVEL_LOW               // Drive Pin Low to make sure camera not in standby
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
    {64,64  , ACTION_UPDATE_ALL           // CAM1 Reset Control
            ,
              PIN_LEVEL_LOW               // Need to be driven High to keep the sensor ON
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
	{20,20  , ACTION_UPDATE_ALL           // CAM1 Trigger
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
              D_GPIO_PAD_DRIVE_4mA     | 
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
    // CAM2 Extra Configuration 
    // 
    // -----------------------------------------------------------------------
	{14  ,16 , ACTION_UPDATE_ALL           // CAM2 Data 9-11
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
	{19,19  , ACTION_UPDATE_ALL           // CAM2 Standby
            ,
              PIN_LEVEL_LOW               // Need to be driven High to keep the sensor ON
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
	{12,12  , ACTION_UPDATE_ALL           // CAM2 Reset Control
            ,
              PIN_LEVEL_LOW               // Default to not holding camera in reset
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
	{18,18  , ACTION_UPDATE_ALL           // CAM2 Trigger
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
              D_GPIO_PAD_DRIVE_4mA     | 
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

#endif // BRD_MV0172_GPIO_DEFAULTS_H_
