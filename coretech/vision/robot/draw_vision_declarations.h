/**
File: draw_vision_declarations.h
Author: Peter Barnum
Created: 2013

Vision-specific drawing functions, for drawing on Array objects.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_DRAW_VISION_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_VISION_DRAW_VISION_DECLARATIONS_H_

#include "coretech/common/robot/array2d_declarations.h"
#include "coretech/common/robot/draw_declarations.h"

#include "coretech/vision/robot/connectedComponents_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    // Draw the components on the image
    // If minValue == maxValue == 0, no scaling will be performed
    template<typename Type> Result DrawComponents(Array<Type> &image, ConnectedComponents &components, Type minValue, Type maxValue);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_DRAW_VISION_DECLARATIONS_H_
