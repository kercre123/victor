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
#ifndef BRD_MV0155_GPIO_DEFAULTS_H_
#define BRD_MV0155_GPIO_DEFAULTS_H_ 

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
                                            D_GPIO_PAD_SCHMITT_ON   |  \
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

const drvGpioInitArrayType brdMv0155GpioCfgDefault =
{
    // -----------------------------------------------------------------------
    // I2C1 Configuration 
    // -----------------------------------------------------------------------
    {67, 67 , ACTION_UPDATE_ALL          // I2C1 Pins.. WARNING: 0xC implies bit 10 (reserved is set)
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_7            |  // let it floating for now 
              D_GPIO_DIR_IN            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },      
    {68, 68 , ACTION_UPDATE_ALL          // I2C1 Pins.. WARNING: 0xC implies bit 10 (reserved is set)
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_7            |  // let it floating for now 
              D_GPIO_DIR_IN            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },      
    {75, 75 , ACTION_UPDATE_ALL          // I2C1 Pins.. WARNING: 0xC implies bit 10 (reserved is set)
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_7            |  // let it floating for now 
              D_GPIO_DIR_IN            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },      
    {81, 81 , ACTION_UPDATE_ALL          // I2C1 Pins.. WARNING: 0xC implies bit 10 (reserved is set)
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_7            |  // let it floating for now 
              D_GPIO_DIR_IN            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },      

    // -----------------------------------------------------------------------
    // MIPI gpios 
    // -----------------------------------------------------------------------    
   {114, 114, ACTION_UPDATE_ALL  // cam1 mclk
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_0             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },          
   {13, 13, ACTION_UPDATE_ALL  // cam2 mclk
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_4             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },          
   {64, 64, ACTION_UPDATE_ALL  // LCD1 mclk
            ,
              PIN_LEVEL_NA
            ,
              D_GPIO_MODE_0             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },          
    
   {127, 127, ACTION_UPDATE_ALL      //RESX
            ,
              PIN_LEVEL_HIGH                 // do no reset
            ,
              D_GPIO_MODE_7             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },          
   {113, 113, ACTION_UPDATE_ALL      //RESX
            ,
              PIN_LEVEL_HIGH                 // do no reset
            ,
              D_GPIO_MODE_7             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },          
    {63, 63, ACTION_UPDATE_ALL      //RESX
            ,
              PIN_LEVEL_HIGH                 // do no reset
            ,
              D_GPIO_MODE_7             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },          
    {98, 98, ACTION_UPDATE_ALL      //msel 
            ,
              PIN_LEVEL_LOW                 // cs2 rx
            ,
              D_GPIO_MODE_7             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },          
    {112, 112, ACTION_UPDATE_ALL      //msel 
            ,
              PIN_LEVEL_LOW                 // cs2 rx
            ,
              D_GPIO_MODE_7             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },          
    {126, 126, ACTION_UPDATE_ALL      //msel 
            ,
              PIN_LEVEL_LOW                 // cs2 rx
            ,
              D_GPIO_MODE_7             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },          
    {141, 141, ACTION_UPDATE_ALL      // CS
            ,
              PIN_LEVEL_HIGH                 // not selected 
            ,
              D_GPIO_MODE_7             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },          
    {3, 3, ACTION_UPDATE_ALL      // CS
            ,
              PIN_LEVEL_HIGH                 // not selected 
            ,
              D_GPIO_MODE_7             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },          
    {26, 26, ACTION_UPDATE_ALL      // CS
            ,
              PIN_LEVEL_HIGH                 // not selected 
            ,
              D_GPIO_MODE_7             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS
    },          
    // -----------------------------------------------------------------------
    // LEDS Configuration 
    // -----------------------------------------------------------------------
     {77, 80, ACTION_UPDATE_ALL      // LEDS
            ,
              PIN_LEVEL_LOW                 // off
            ,
              D_GPIO_MODE_7             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS2_5
    },    
     {74, 74, ACTION_UPDATE_ALL      // LEDS
            ,
              PIN_LEVEL_LOW                 // off
            ,
              D_GPIO_MODE_7             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS2_5
    },    
    // -----------------------------------------------------------------------
    // CAM2 Configuration 
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
    // CAM1 Configuration 
    // -----------------------------------------------------------------------    
    {115, 117, ACTION_UPDATE_ALL      // pclk, hs, vs
            ,
              PIN_LEVEL_NA                 
            ,
              D_GPIO_MODE_0             |  
              D_GPIO_DIR_IN             | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS1_8CAM
    },    
    {118, 125, ACTION_UPDATE_ALL      // d0 - d7 
            ,
              PIN_LEVEL_NA                 
            ,
              D_GPIO_MODE_0             |  
              D_GPIO_DIR_IN             | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS1_8CAM
    },    
    {128, 129, ACTION_UPDATE_ALL      //d8 d9
            ,
              PIN_LEVEL_NA                 
            ,
              D_GPIO_MODE_0             |  
              D_GPIO_DIR_IN             | 
              LOCAL_D_GPIO_DEFAULTS
            , 
              LOCAL_D_GPIO_PAD_DEFAULTS1_8CAM
    },    
    // -----------------------------------------------------------------------
    // LCD2 Configuration 
    // -----------------------------------------------------------------------
    {101, 102, ACTION_UPDATE_ALL      //vs hs
            ,
              PIN_LEVEL_NA                 
            ,
              D_GPIO_MODE_0             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , D_GPIO_PAD_NO_PULL       |  
              D_GPIO_PAD_DRIVE_8mA     |  // 8 mA
              D_GPIO_PAD_VOLT_2V5      |  
              D_GPIO_PAD_SLEW_FAST     |  
              D_GPIO_PAD_SCHMITT_OFF   |  
              D_GPIO_PAD_RECEIVER_ON   |  
              D_GPIO_PAD_BIAS_2V5      |  
              D_GPIO_PAD_LOCALCTRL_OFF |  
              D_GPIO_PAD_LOCALDATA_LO  |  
              D_GPIO_PAD_LOCAL_PIN_OUT
              
    },    
    {100, 100, ACTION_UPDATE_ALL      //pclk
            ,
              PIN_LEVEL_NA                 
            ,
              D_GPIO_MODE_0            |  
              D_GPIO_DIR_OUT           | 
              D_GPIO_DATA_INV_ON       |  // inv on 
              D_GPIO_WAKEUP_OFF        |  
              D_GPIO_IRQ_SRC_NONE
            , D_GPIO_PAD_NO_PULL       |  
              D_GPIO_PAD_DRIVE_8mA     |  // 8mA
              D_GPIO_PAD_VOLT_2V5      |  
              D_GPIO_PAD_SLEW_FAST     |  
              D_GPIO_PAD_SCHMITT_OFF   |  
              D_GPIO_PAD_RECEIVER_ON   |  
              D_GPIO_PAD_BIAS_2V5      |  
              D_GPIO_PAD_LOCALCTRL_OFF |  
              D_GPIO_PAD_LOCALDATA_LO  |  
              D_GPIO_PAD_LOCAL_PIN_OUT
              
    },    
     {104, 111, ACTION_UPDATE_ALL      //data
            ,
              PIN_LEVEL_NA                 
            ,
              D_GPIO_MODE_0             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , D_GPIO_PAD_NO_PULL       |  
              D_GPIO_PAD_DRIVE_4mA     |    // 4mA
              D_GPIO_PAD_VOLT_2V5      |  
              D_GPIO_PAD_SLEW_FAST     |  
              D_GPIO_PAD_SCHMITT_OFF   |  
              D_GPIO_PAD_RECEIVER_ON   |  
              D_GPIO_PAD_BIAS_2V5      |  
              D_GPIO_PAD_LOCALCTRL_OFF |  
              D_GPIO_PAD_LOCALDATA_LO  |  
              D_GPIO_PAD_LOCAL_PIN_OUT
              
    },       
    // -----------------------------------------------------------------------
    // LCD1 Configuration 
    // -----------------------------------------------------------------------    
   {24, 25,  ACTION_UPDATE_ALL      //vs hs
            ,
              PIN_LEVEL_NA                 
            ,
              D_GPIO_MODE_0             |  
              D_GPIO_DIR_OUT            | 
              LOCAL_D_GPIO_DEFAULTS
            , D_GPIO_PAD_NO_PULL       |  
              D_GPIO_PAD_DRIVE_2mA     |  
              D_GPIO_PAD_VOLT_1V8      |  
              D_GPIO_PAD_SLEW_FAST     |  
              D_GPIO_PAD_SCHMITT_OFF   |  
              D_GPIO_PAD_RECEIVER_ON   |  
              D_GPIO_PAD_BIAS_2V5      |  
              D_GPIO_PAD_LOCALCTRL_OFF |  
              D_GPIO_PAD_LOCALDATA_LO  |  
              D_GPIO_PAD_LOCAL_PIN_OUT
              
    },        
    
  {27, 43,  ACTION_UPDATE_ALL      //de, data
            ,
              PIN_LEVEL_NA                 
            ,
              D_GPIO_MODE_0             |  
              D_GPIO_DIR_IN             | 
              LOCAL_D_GPIO_DEFAULTS
            , D_GPIO_PAD_NO_PULL       |  
              D_GPIO_PAD_DRIVE_2mA     |  
              D_GPIO_PAD_VOLT_1V8      |  
              D_GPIO_PAD_SLEW_FAST     |  
              D_GPIO_PAD_SCHMITT_OFF   |  
              D_GPIO_PAD_RECEIVER_ON   |  
              D_GPIO_PAD_BIAS_2V5      |  
              D_GPIO_PAD_LOCALCTRL_OFF |  
              D_GPIO_PAD_LOCALDATA_LO  |  
              D_GPIO_PAD_LOCAL_PIN_OUT
              
    },            
  {47, 48,  ACTION_UPDATE_ALL      //data
            ,
              PIN_LEVEL_NA                 
            ,
              D_GPIO_MODE_0             |  
              D_GPIO_DIR_IN             | 
              LOCAL_D_GPIO_DEFAULTS
            , D_GPIO_PAD_NO_PULL       |  
              D_GPIO_PAD_DRIVE_2mA     |  
              D_GPIO_PAD_VOLT_1V8      |  
              D_GPIO_PAD_SLEW_FAST     |  
              D_GPIO_PAD_SCHMITT_OFF   |  
              D_GPIO_PAD_RECEIVER_ON   |  
              D_GPIO_PAD_BIAS_2V5      |  
              D_GPIO_PAD_LOCALCTRL_OFF |  
              D_GPIO_PAD_LOCALDATA_LO  |  
              D_GPIO_PAD_LOCAL_PIN_OUT
              
    },                
  {44, 45,  ACTION_UPDATE_ALL      //data
            ,
              PIN_LEVEL_NA                 
            ,
              D_GPIO_MODE_2             |  
              D_GPIO_DIR_IN             | 
              LOCAL_D_GPIO_DEFAULTS
            , D_GPIO_PAD_NO_PULL       |  
              D_GPIO_PAD_DRIVE_2mA     |  
              D_GPIO_PAD_VOLT_1V8      |  
              D_GPIO_PAD_SLEW_FAST     |  
              D_GPIO_PAD_SCHMITT_OFF   |  
              D_GPIO_PAD_RECEIVER_ON   |  
              D_GPIO_PAD_BIAS_2V5      |  
              D_GPIO_PAD_LOCALCTRL_OFF |  
              D_GPIO_PAD_LOCALDATA_LO  |  
              D_GPIO_PAD_LOCAL_PIN_OUT
              
    },                
  {59, 62,  ACTION_UPDATE_ALL      //data
            ,
              PIN_LEVEL_NA                 
            ,
              D_GPIO_MODE_4             |  
              D_GPIO_DIR_IN             | 
              LOCAL_D_GPIO_DEFAULTS
            , D_GPIO_PAD_NO_PULL       |  
              D_GPIO_PAD_DRIVE_2mA     |  
              D_GPIO_PAD_VOLT_1V8      |  
              D_GPIO_PAD_SLEW_FAST     |  
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

#endif // BRD_MV0155_GPIO_DEFAULTS_H_
