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
#ifndef BRD_MV0153_GPIO_DEFAULTS_H_
#define BRD_MV0153_GPIO_DEFAULTS_H_ 

// 1: Includes             
// ----------------------------------------------------------------------------

// 2: Defines
// ----------------------------------------------------------------------------

// 3: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 4: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

const drvGpioInitArrayType brdMv0153GpioCfgDefault =
{
    // -----------------------------------------------------------------------
    // I2C1 Configuration 
    // -----------------------------------------------------------------------
    {65, 66 , ACTION_UPDATE_ALL          // I2C1 Pins.. WARNING: 0xC implies bit 10 (reserved is set)
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
              D_GPIO_PAD_DRIVE_8mA     |  // 8mA  (DO we really need this?)
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
    // I2C2 Configuration 
    // DrvGpioModeRange(74, 75, 0x0c04);
    // 
    // -----------------------------------------------------------------------
    {74, 75 , ACTION_UPDATE_ALL      
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_4            |  // Mode 4 to select I2C Mode
              D_GPIO_DIR_IN            | 
              D_GPIO_DATA_INV_ON       |  // TODO: Why do we invert this bit?
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
    // CAM1 Configuration 
    // 
    // -----------------------------------------------------------------------
    {114,114, ACTION_UPDATE_ALL           // CAM1_MCLK
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
    {115,115, ACTION_UPDATE_ALL           // CAM1_PCLK 
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
    {116,116, ACTION_UPDATE_ALL           // CAM1_VSYNC 
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select CAM1
              D_GPIO_DIR_IN            | 
              D_GPIO_DATA_INV_ON       |  // Pin is inverted
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
    {117,117, ACTION_UPDATE_ALL           // CAM1_HSYNC 
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
    {118,125, ACTION_UPDATE_ALL           // CAM1 Data 0-7
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
    {113,113, ACTION_UPDATE_ALL           // CAM1 Strobe (manual mode) set as input for now as now used
            ,
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_IN            | 
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
    {16,16  , ACTION_UPDATE_ALL           // CAM1 Reset Control
            ,
              PIN_LEVEL_HIGH              // Default to not holding Camera in reset
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
    {91 ,91 , ACTION_UPDATE_ALL           // CAM1 Powerdown, Connected on MV0121, but not on later boards
            ,
              PIN_LEVEL_LOW               // Drive Pin Low to make sure camera not in power down by default
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
    {64 ,64 , ACTION_UPDATE_ALL           // CAM1 AFCTRL; Not actually used on any board yet
            ,
              PIN_LEVEL_LOW               
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_IN            |  // Set pin as input 
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
    // -----------------------------------------------------------------------
    // CAM2 Configuration 
    // 
    // -----------------------------------------------------------------------
    {13 ,13 , ACTION_UPDATE_ALL           // CAM2_MCLK
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
    {0  ,0  , ACTION_UPDATE_ALL           // CAM2_PCLK 
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
    {1  ,1  , ACTION_UPDATE_ALL           // CAM2_VSYNC 
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
    {2  , 2 , ACTION_UPDATE_ALL           // CAM2_HSYNC 
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
    {4  ,11 , ACTION_UPDATE_ALL           // CAM2 Data 0-7
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
    {14 ,15 , ACTION_UPDATE_ALL           // CAM2 Flash(strobe); Cam2 shutter (manual mode) set as input for now as now used
            ,                             // On 121, CAM2_Flash is connected to the strobe sensor output
              PIN_LEVEL_LOW
            ,
              D_GPIO_MODE_7            |  // Mode 7 to select GPIO
              D_GPIO_DIR_IN            | 
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
    {12,12  , ACTION_UPDATE_ALL           // CAM2 Reset Control
            ,
              PIN_LEVEL_HIGH              // Default to not holding camera in reset
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
    {24,24  , ACTION_UPDATE_ALL           // LCD1_VSYNC
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
    {25,25  , ACTION_UPDATE_ALL           // LCD1_PCLK
            ,
              PIN_LEVEL_LOW               // Default to driving low
            ,
              D_GPIO_MODE_0            |  // Mode 0 to select LCD  (Note: It will be reconfigured by ext PLL driver)
              D_GPIO_DIR_IN            |  
              D_GPIO_DATA_INV_ON       |  // Inverted
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE 
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
    },                       
    {26,26  , ACTION_UPDATE_ALL           // LCD1_HSYNC
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
    {27,27  , ACTION_UPDATE_ALL           // LCD1_DATAEN
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
    {28,43  , ACTION_UPDATE_ALL           // LCD1 D0 -> D15
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
    {47,48  , ACTION_UPDATE_ALL           // LCD1 D16 -> D17
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
    {44,45  , ACTION_UPDATE_ALL           // LCD1 D18 -> D19
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
    {59,62  , ACTION_UPDATE_ALL           // LCD1 D20 -> D23
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
    {23,23  , ACTION_UPDATE_ALL           // HDMI_RESET_N GPIO
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
    // Power Control Pin
    // WARNING: Need to configure the PWM for this to work
    // -----------------------------------------------------------------------
    {68,68  , ACTION_UPDATE_ALL           // HDMI_RESET_N GPIO
            ,
              PIN_LEVEL_LOW               // Default to holding in reset for now
            ,
              D_GPIO_MODE_1            |  // Mode 1 to select PWM_OUT_3
              D_GPIO_DIR_OUT           |  // Drive out
              D_GPIO_DATA_INV_OFF      |  
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE 
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
    },
    // -----------------------------------------------------------------------
    // Clock Select Pin
    // -----------------------------------------------------------------------
    {46,46  , ACTION_UPDATE_ALL           
            ,
              PIN_LEVEL_LOW               // Default to disable external PLL
            ,
              D_GPIO_MODE_7            |  // Mode 7 (Direct GPIO output)
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
    {69,72  , ACTION_UPDATE_ALL          // 
            ,
              PIN_LEVEL_LOW               // 
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
    // GPIO Buttons    (only buttons 1-4 are on GPIOS)
    // -----------------------------------------------------------------------
    {50,51  , ACTION_UPDATE_ALL           // Button 2,1
            ,                            
              PIN_LEVEL_LOW               // 
            ,
              D_GPIO_MODE_7            |  // Direct GPIO mode
              D_GPIO_DIR_IN            |  // Input
              D_GPIO_DATA_INV_OFF      |  
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE 
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
    },
    {83,84  , ACTION_UPDATE_ALL           // Button 4,3
            ,                            
              PIN_LEVEL_LOW               // 
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
    // SPI Control to LCD Connector Interface
    // -----------------------------------------------------------------------
    {18,21  , ACTION_UPDATE_ALL          // 
            ,
              PIN_LEVEL_LOW               // 
            ,
              D_GPIO_MODE_3            |  // Mode3 SPI3)
              D_GPIO_DIR_OUT           |  // Drive out
              D_GPIO_DATA_INV_OFF      |  
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE 
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
    },
    // -----------------------------------------------------------------------
    // PCB Revision Detection
    // -----------------------------------------------------------------------
    {56,57  , ACTION_UPDATE_ALL          // RevDet 0,1
            ,
              PIN_LEVEL_LOW               // 
            ,
              D_GPIO_MODE_7            |  // Mode7 GPIO)
              D_GPIO_DIR_IN            |  // Pins are input
              D_GPIO_DATA_INV_OFF      |  
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE 
            ,
              D_GPIO_PAD_PULL_UP       |  // Enable Weak pullups so we only need resistor to tie pin low
              D_GPIO_PAD_DRIVE_2mA     |  // if we want to flick a bit. bits default to 1 unless pulled down.
              D_GPIO_PAD_VOLT_2V5      | 
              D_GPIO_PAD_SLEW_SLOW     | 
              D_GPIO_PAD_SCHMITT_ON    |  // No harm to have schmitt trigger on
              D_GPIO_PAD_RECEIVER_ON   | 
              D_GPIO_PAD_BIAS_2V5      | 
              D_GPIO_PAD_LOCALCTRL_OFF | 
              D_GPIO_PAD_LOCALDATA_LO  | 
              D_GPIO_PAD_LOCAL_PIN_OUT   
    },
    {52,52  , ACTION_UPDATE_ALL          // RevDet 2
            ,
              PIN_LEVEL_LOW               // 
            ,
              D_GPIO_MODE_7            |  // Mode7 GPIO)
              D_GPIO_DIR_IN            |  // Pins are input
              D_GPIO_DATA_INV_OFF      |  
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE 
            ,
              D_GPIO_PAD_PULL_UP       |  // Enable Weak pullups so we only need resistor to tie pin low
              D_GPIO_PAD_DRIVE_2mA     |  // if we want to flick a bit. bits default to 1 unless pulled down.
              D_GPIO_PAD_VOLT_2V5      | 
              D_GPIO_PAD_SLEW_SLOW     | 
              D_GPIO_PAD_SCHMITT_ON    |  // No harm to have schmitt trigger on
              D_GPIO_PAD_RECEIVER_ON   | 
              D_GPIO_PAD_BIAS_2V5      | 
              D_GPIO_PAD_LOCALCTRL_OFF | 
              D_GPIO_PAD_LOCALDATA_LO  | 
              D_GPIO_PAD_LOCAL_PIN_OUT   
    },
    // -----------------------------------------------------------------------
    // InfraRed Sensor Pin
    // -----------------------------------------------------------------------
    {58,58  , ACTION_UPDATE_ALL           // Infrared Sensor Pin
            ,                            
              PIN_LEVEL_LOW               // 
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
    // CEC Pin (Configure as an input for now, SW will need to control this pins enable
    // -----------------------------------------------------------------------
    {76,76  , ACTION_UPDATE_ALL           // CEC Input Pin
            ,                            
              PIN_LEVEL_LOW               // 
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
    // LED Config
    // -----------------------------------------------------------------------
    {80,80  , ACTION_UPDATE_ALL           // 
            ,
              PIN_LEVEL_LOW               
            ,
              D_GPIO_MODE_4            |  
              D_GPIO_DIR_OUT           |  
              D_GPIO_DATA_INV_OFF      |  
              D_GPIO_WAKEUP_OFF        |
              D_GPIO_IRQ_SRC_NONE 
            ,
              D_GPIO_PAD_DEFAULTS         // Uses the default PAD configuration
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
