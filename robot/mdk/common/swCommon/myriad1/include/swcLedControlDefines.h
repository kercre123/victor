///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by LED control module
/// 
#ifndef SWC_LED_CONTROL_DEF_H
#define SWC_LED_CONTROL_DEF_H 

// 1: Defines
// ----------------------------------------------------------------------------
// Function Error Values
#define SWC_LED_SUCCESS                     ( 0)
#define SWC_LED_INVALID                     (-1)
#define SWC_LED_NO_CFG                      (-2)

// Note: Irrespecitve of the LED pin polarity 0 always disables the LED in this module
#define SWC_LED_OFF                         (0 )
#define SWC_LED_ON                          (1 ) // Any non-zero value would do

// LED names (can optionally be used to make code more descriptive
#define SWC_LED_PWR                         (0 ) // By convention, IFF Power LED is controllable it is index 0!!

#define SWC_LED_1                           (1 )
#define SWC_LED_2                           (2 )
#define SWC_LED_3                           (3 )
#define SWC_LED_4                           (4 )
#define SWC_LED_5                           (5 )
#define SWC_LED_6                           (6 )
#define SWC_LED_7                           (7 )
#define SWC_LED_8                           (8 )
#define SWC_LED_9                           (9 )
#define SWC_LED_10                          (10)
#define SWC_LED_11                          (11)
#define SWC_LED_12                          (11)
#define SWC_LED_13                          (11)
#define SWC_LED_14                          (11)
#define SWC_LED_15                          (11)
                                                                 
// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

typedef struct
{
    u8      totalLeds;
    u8      pwrLedPresent;
    u8 *    ledArray;        // Note: By convention, Power LED is LED0 if controllable
} tyLedSystemConfig;

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // SWC_LED_CONTROL_DEF_H


