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

#include "coretech/vision/robot/draw_vision_declarations.h"

#include "coretech/common/robot/array2d.h"
#include "coretech/common/robot/draw.h"

#include "coretech/vision/robot/connectedComponents.h"

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark --- Definitions ---

    template<typename Type> Result DrawComponents(Array<Type> &image, ConnectedComponents &components, Type minValue, Type maxValue)
    {
      AnkiConditionalErrorAndReturnValue(AreValid(image, components),
        RESULT_FAIL_INVALID_OBJECT, "DrawComponents", "Invalid objects");

      const f64 numComponentIds = components.get_maximumId();
      const f64 minValueD = static_cast<f64>(minValue);
      const f64 maxValueD = static_cast<f64>(maxValue);
      const f64 step = (numComponentIds==1) ? 0.0 : ((maxValueD - minValueD) / (numComponentIds-1));

      const bool doScaling = (minValue == maxValue) ? false : true;

      const s32 numComponents = components.get_size();

      const bool useU16 = components.get_useU16();
      const ConnectedComponentsTemplate<u16>* componentsU16 = components.get_componentsU16();
      const ConnectedComponentsTemplate<s32>* componentsS32 = components.get_componentsS32();

      for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
        s32 id;
        if(useU16) {
          id = (*(components.get_componentsU16()))[iComponent].id;
        } else {
          id = (*(components.get_componentsS32()))[iComponent].id;
        }

        if(id == 0)
          continue;

        s16 y;
        s16 xStart;
        s16 xEnd;

        if(useU16) {
          y = (*componentsU16)[iComponent].y;
          xStart = (*componentsU16)[iComponent].xStart;
          xEnd = (*componentsU16)[iComponent].xEnd;
        } else {
          y = (*componentsS32)[iComponent].y;
          xStart = (*componentsS32)[iComponent].xStart;
          xEnd = (*componentsS32)[iComponent].xEnd;
        }

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
