///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @defgroup PatternGenerator Pattern Generator component.
/// @{
/// @brief     Pattern Generator Functions API.
///

#ifndef _PATTERN_GENERATOR_API_H_
#define _PATTERN_GENERATOR_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "PatternGeneratorApiDefines.h"
#include "swcFrameTypes.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------
// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

///
/// Create an horizontal pattern at address specified in param list
/// @param[in] frame  frame to make pattern
/// @param[in] interlaced  flag for interlaced frames
/// @return      0 FAIL, 1 SUCCESS
///
unsigned int CreateHorizontalColorBars (frameBuffer *frame, unsigned int interlaced);

///
/// Create an vertical pattern at address specified in param list
/// @param[in] frame  frame to make pattern
/// @param[in] interlaced  flag for interlaced frames
/// @return     0 FAIL, 1 SUCCESS
///
unsigned int CreateVerticalColorBars (frameBuffer *frame, unsigned int interlaced);

///
/// Create an specific horizontal pattern with 64 color stripes for
///           first half and 8 color stripes for second half
/// @param[in] frame  frame to make pattern
/// @return     0 FAIL, 1 SUCCESS
///
unsigned int CreateColorStripesPattern (frameBuffer *frame);

///
/// Create an specific vertical pattern with 17 grey stripes, from black to white
/// @param[in] frame  frame to make pattern
/// @return     0 FAIL, 1 SUCCESS
///
unsigned int CreateLinearGreyPattern (frameBuffer *frame);

///
/// Check if patterns are correct by comparing
/// @param[in] inputFrame   input frame to test
/// @param[in] outputFrame  output frame to test
/// @param[in] interlaced   flag for interlaced frames
/// @return    0 FAIL, 1 MATCH
///
unsigned int PatternCheck (frameBuffer *inputFrame, frameBuffer *outputFrame, unsigned int interlaced);
/// @}
#endif /* _PATTERN_GENERATOR_API_H_ */
