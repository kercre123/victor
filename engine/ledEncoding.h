/** @file Simple macros / functions for converting wide LED blink parameters to encoded ones for the robot
 * @author Daniel Casner <daniel@anki.com>
 * @date January 5th 2016
 * @copyright Anki Inc. 2016
 */
#ifndef _COZMO_BASESTATION_LED_ENCODING_H_
#define _COZMO_BASESTATION_LED_ENCODING_H_

#include "clad/types/ledTypes.h"

/// Converts 32 bit RGBA color to 16 bit
#define ENCODED_COLOR(color)  (((color << 24) & (u32)Anki::Cozmo::LEDColor::LED_IR) ? (u16)Anki::Cozmo::LEDColorEncoded::LED_ENC_IR : 0)    | \
                             ((((color >> 8) & (u32)Anki::Cozmo::LEDColor::LED_RED)   >> (16 + 3)) << (u32)Anki::Cozmo::LEDColorEncodedShifts::LED_ENC_RED_SHIFT) | \
                             ((((color >> 8) & (u32)Anki::Cozmo::LEDColor::LED_GREEN) >> ( 8 + 3)) << (u32)Anki::Cozmo::LEDColorEncodedShifts::LED_ENC_GRN_SHIFT) | \
                             ((((color >> 8) & (u32)Anki::Cozmo::LEDColor::LED_BLUE)  >> ( 0 + 3)) << (u32)Anki::Cozmo::LEDColorEncodedShifts::LED_ENC_BLU_SHIFT)

/// Convert MS to LED FRAMES
#define MS_TO_LED_FRAMES(ms)  (ms == std::numeric_limits<u32>::max() ? std::numeric_limits<u8>::max() : (((ms)+29)/30))

#endif
