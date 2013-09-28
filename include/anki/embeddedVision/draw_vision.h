#ifndef _ANKICORETECHEMBEDDED_VISION_DRAW_VISION_H_
#define _ANKICORETECHEMBEDDED_VISION_DRAW_VISION_H_

#include "anki/embeddedCommon/config.h"
#include "anki/embeddedCommon/array2d.h"
#include "anki/embeddedVision/connectedComponents.h"
#include "anki/embeddedVision/connectedComponents.h"

namespace Anki
{
  namespace Embedded
  {
    // Draw the components on the image
    // If minValue == maxValue == 0, no scaling will be performed
    template<typename Type> Result DrawComponents(Array<Type> &image, ConnectedComponents &components, Type minValue, Type maxValue);

#pragma mark --- Implementations ---

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

      for(s32 i=0; i<components.get_size(); i++) {
        const u16 id = components[i].id;

        if(id == 0)
          continue;

        const s16 y = components[i].y;
        const s16 xStart = components[i].xStart;
        const s16 xEnd = components[i].xEnd;

        Type lineColor;

        if(doScaling) {
          const double idD = static_cast<double>(id - 1);
          lineColor = static_cast<Type>(step*idD + minValueD);
        } else {
          lineColor = static_cast<Type>(id);
        }

        Type * restrict image_rowPointer = image.Pointer(y, 0);
        for(s16 x=xStart; x<=xEnd; x++) {
          image_rowPointer[x] = lineColor;
        }
      }

      return RESULT_OK;
    } // template<typename Type> Result DrawRectangle(Array<Type> &image, const Point<s16> point1, const Point<s16> point2, const s32 lineWidth, const Type lineColor, const Type backgroundColor)
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_DRAW_VISION_H_
