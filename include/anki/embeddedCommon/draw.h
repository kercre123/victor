#ifndef _ANKICORETECHEMBEDDED_COMMON_DRAW_H_
#define _ANKICORETECHEMBEDDED_COMMON_DRAW_H_

#include "anki/embeddedCommon/config.h"

namespace Anki
{
  namespace Embedded
  {
    // Draw a box with the two opposing corners point1 and point2
    // If lineWidth < 1, then the box will be solid. Otherwise, the inside will be filled with backgroundColor
    template<typename Type> Result DrawRectangle(Array<Type> &image, Point<s16> point1, Point<s16> point2, const s32 lineWidth, const Type lineColor, const Type backgroundColor)
    {
      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL, "DrawRectangle", "image is not valid");

      // Make point1 the upper left, and point2 the lower right
      if(point1.x > point2.x) Swap(point1.x, point2.x);
      if(point1.y > point2.y) Swap(point1.y, point2.y);

      Point<s16> point1inner;
      Point<s16> point2inner;

      if(lineWidth > 0) {
        point1inner.x = point1.x + static_cast<s16>(lineWidth);
        point1inner.y = point1.y + static_cast<s16>(lineWidth);
        point2inner.x = point2.x - static_cast<s16>(lineWidth);
        point2inner.y = point2.y - static_cast<s16>(lineWidth);

        point1inner.x = static_cast<s16>(CLIP(point1inner.x, 0, image.get_size(1)-1));
        point1inner.y = static_cast<s16>(CLIP(point1inner.y, 0, image.get_size(1)-1));
        point2inner.x = static_cast<s16>(CLIP(point2inner.x, 0, image.get_size(1)-1));
        point2inner.y = static_cast<s16>(CLIP(point2inner.y, 0, image.get_size(1)-1));
      }

      // Make the corners fit inside the image
      point1.x = static_cast<s16>(CLIP(point1.x, 0, image.get_size(1)-1));
      point1.y = static_cast<s16>(CLIP(point1.y, 0, image.get_size(1)-1));
      point2.x = static_cast<s16>(CLIP(point2.x, 0, image.get_size(1)-1));
      point2.y = static_cast<s16>(CLIP(point2.y, 0, image.get_size(1)-1));

      for(s16 y=point1.y; y<=point2.y; y++) {
        Type * restrict image_rowPointer = image.Pointer(y, 0);
        for(s16 x=point1.x; x<=point2.x; x++) {
          image_rowPointer[x] = lineColor;
        }
      }

      if(lineWidth > 0) {
        /*for(s16 y=point1.y+static_cast<s16>(lineWidth); y<=(point2.y-static_cast<s16>(lineWidth)); y++) {
        Type * restrict image_rowPointer = image.Pointer(y, 0);
        for(s16 x=point1.x+static_cast<s16>(lineWidth); x<=(point2.x-static_cast<s16>(lineWidth)); x++) {
        image_rowPointer[x] = backgroundColor;
        }
        }*/
        for(s16 y=point1inner.y; y<=point2inner.y; y++) {
          Type * restrict image_rowPointer = image.Pointer(y, 0);
          for(s16 x=point1inner.x; x<=point2inner.x; x++) {
            image_rowPointer[x] = backgroundColor;
          }
        }
      }

      return RESULT_OK;
    } // template<typename Type> Result DrawRectangle(Array<Type> &image, const Point<s16> point1, const Point<s16> point2, const s32 lineWidth, const Type lineColor, const Type backgroundColor)
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_DRAW_H_
