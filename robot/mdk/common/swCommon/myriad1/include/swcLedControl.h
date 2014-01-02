///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for common LED control module
/// 
/// 
/// 
/// 
/// 

#ifndef SWC_LED_CONTROL_H
#define SWC_LED_CONTROL_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include <swcLedControlDefines.h>

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Initialise the Led block with the configuration of the hardware platform
/// @param[in] boardLedConfig Led configuration
/// @return    0 on success, non-zero otherwise  
void swcLedInitialise(tyLedSystemConfig * boardLedConfig);
                        
/// Returns the total number of general purpose LEDs in the system
/// 
/// Power Led is not included
/// @return    0 if no LEDs available, or LED module not configured, otherwise numLeds
int swcLedGetNumLeds(void);

          
/// Turn on or off selected led in range 0-(totalLeds -1)
/// @param[in] ledNum Led Number
/// @param[in] enable 0 to disable LED, otherwise LED is enabled
/// @return    0 on success, non-zero otherwise  
int swcLedSet(int ledNum, int enable); 

/// Toggle the current LED
///
/// If LED is currently on, switch it off, and vice versa
/// @param[in] ledNum Led Number
/// @return    0 on success, non-zero otherwise  
int swcLedToggle(int ledNum);

/// Turn on or off selected range of LEDs 
///
/// swcLedSetRange(0,7,SWC_LED_OFF) // turns LEDS 0-7 off
/// @param[in] ledStart first LED in the range
/// @param[in] ledEnd last LED in the range
/// @param[in] enable 0 to disable LED, otherwise LED is enabled
/// @return    0 on success, non-zero otherwise  
int swcLedSetRange(int ledStart,int ledEnd, int enable);

/// Sets a binary value across a range of LEDS 
/// 
/// @note lsbLed may be > msbLed in which case the pattern uses reverse order 
///
/// @param[in] lsbLed is the LED which will be used for the LSB of the pattern value
/// @param[in] msbLed is the LED which will correspond to the highest bit within pattern (e.g. bit 5 if we have 6 leds)
/// @param[in] value bit field where 1 bits cause LED enable and 0 bits cause LED disable
/// @return    0 on success, non-zero otherwise  
int swcLedSetBinValue(int lsbLed,int msbLed, u32 value); 

/// Displays the classic Cylon Eye across a range of LEDs
/// @param[in] ledStart first Led in range
/// @param[in] ledEnd Last Led in range
/// @param[in] eyeWidth Number of bits wide to use for the pattern 
/// @param[in] loops Number of loops to display the pattern for
/// @param[in] delayMs Delay between each LED update in ms (0 will cause it to use system default)
/// @return    0 on success, non-zero otherwise  
int swcLedCylonEye(u32 ledStart,u32 ledEnd,u32 eyeWidth, int loops,u32 delayMs);

#endif // SWC_LED_CONTROL_H  



