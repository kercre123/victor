/**
File: draw_vision.h
Author: Peter Barnum
Created: 2013

Definitions of draw_vision_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_DRAW_VISION_H_
#define _ANKICORETECHEMBEDDED_VISION_DRAW_VISION_H_

#include "anki/vision/robot/draw_vision_declarations.h"

#include "anki/common/robot/array2d.h"
#include "anki/common/robot/draw.h"

#include "anki/vision/robot/connectedComponents.h"

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Definitions ---

    template<typename Type> Result DrawComponents(Array<Type> &image, ConnectedComponents &components, Type minValue, Type maxValue)
    {
      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL, "DrawComponents", "image is not valid");

      AnkiConditionalErrorAndReturnValue(components.IsValid(),
        RESULT_FAIL, "DrawComponents", "components is not valid");

      const f64 numComponents = components.get_maximumId();
      const f64 minValueD = static_cast<f64>(minValue);
      const f64 maxValueD = static_cast<f64>(maxValue);
      const f64 step = (numComponents==1) ? 0.0 : ((maxValueD - minValueD) / (numComponents-1));

      const bool doScaling = (minValue == maxValue) ? false : true;

      for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
        const u16 id = components[iComponent].id;

        if(id == 0)
          continue;

        const s16 y = components[iComponent].y;
        const s16 xStart = components[iComponent].xStart;
        const s16 xEnd = components[iComponent].xEnd;

        Type lineColor;

        if(doScaling) {
          const double idD = static_cast<double>(id - 1);
          lineColor = static_cast<Type>(step*idD + minValueD);
        } else {
          lineColor = static_cast<Type>(id);
        }

        Type * restrict pImage = image.Pointer(y, 0);
        for(s16 x=xStart; x<=xEnd; x++) {
          pImage[x] = lineColor;
        }
      }

      return RESULT_OK;
    } // template<typename Type> Result DrawRectangle(Array<Type> &image, const Point<s16> point1, const Point<s16> point2, const s32 lineWidth, const Type lineColor, const Type backgroundColor)
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_DRAW_VISION_H_