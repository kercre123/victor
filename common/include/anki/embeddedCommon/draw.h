#ifndef _ANKICORETECHEMBEDDED_COMMON_DRAW_H_
#define _ANKICORETECHEMBEDDED_COMMON_DRAW_H_

#include "anki/embeddedCommon/config.h"
#include "anki/embeddedCommon/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    // Draw a box with the two opposing corners point1 and point2
    // If lineWidth < 1, then the box will be solid. Otherwise, the inside will be filled with backgroundColor
    template<typename Type> Result DrawRectangle(Array<Type> &image, Point<s16> point1, Point<s16> point2, const s32 lineWidth, const Type lineColor, const Type backgroundColor);

    // Create a test pattern image, full of different types of squares
    // Example:
    // Array<u8> testSquaresImage = AllocateArrayFromHeap<u8>(480,640);
    // DrawExampleSquaresImage(testSquaresImage);
    // testSquaresImage.Show("testSquaresImage", true);
    template<typename Type> Result DrawExampleSquaresImage(Array<Type> &image);

#pragma mark --- Implementations ---

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
        point1inner.y = static_cast<s16>(CLIP(point1inner.y, 0, image.get_size(0)-1));
        point2inner.x = static_cast<s16>(CLIP(point2inner.x, 0, image.get_size(1)-1));
        point2inner.y = static_cast<s16>(CLIP(point2inner.y, 0, image.get_size(0)-1));
      }

      // Make the corners fit inside the image
      point1.x = static_cast<s16>(CLIP(point1.x, 0, image.get_size(1)-1));
      point1.y = static_cast<s16>(CLIP(point1.y, 0, image.get_size(0)-1));
      point2.x = static_cast<s16>(CLIP(point2.x, 0, image.get_size(1)-1));
      point2.y = static_cast<s16>(CLIP(point2.y, 0, image.get_size(0)-1));

      for(s32 y=point1.y; y<=point2.y; y++) {
        Type * restrict image_rowPointer = image.Pointer(y, 0);
        for(s32 x=point1.x; x<=point2.x; x++) {
          image_rowPointer[x] = lineColor;
        }
      }

      if(lineWidth > 0) {
        for(s32 y=point1inner.y; y<=point2inner.y; y++) {
          Type * restrict image_rowPointer = image.Pointer(y, 0);
          for(s32 x=point1inner.x; x<=point2inner.x; x++) {
            image_rowPointer[x] = backgroundColor;
          }
        }
      }

      return RESULT_OK;
    } // template<typename Type> Result DrawRectangle(Array<Type> &image, const Point<s16> point1, const Point<s16> point2, const s32 lineWidth, const Type lineColor, const Type backgroundColor)

    template<typename Type> Result DrawExampleSquaresImage(Array<Type> &image)
    {
      const s32 boxWidth = 60;
      const Type lineColor = 0;
      const Type backgroundColor = 128;

      image.Set(128);

      s32 upperLeft[2];
      upperLeft[0] = 19;
      upperLeft[1] = 19;
      for(s32 i=1; i<=5; i++) {
        const Point<s16> point1 = Point<s16>(static_cast<s16>(upperLeft[1]+boxWidth*2*(i-1)), static_cast<s16>(upperLeft[0]));
        const Point<s16> point2 = Point<s16>(static_cast<s16>(upperLeft[1]+boxWidth+boxWidth*2*(i-1)), static_cast<s16>(upperLeft[0]+boxWidth));
        //    image = drawBox(image, upperLeft + [0, boxWidth*2*(i-1)], upperLeft + boxWidth + [0, boxWidth*2*(i-1)], i, lineColdor, backgroundColor);
        DrawRectangle<Type>(image, point1, point2, i, lineColor, backgroundColor);
      }

      //upperLeft = upperLeft + [2*boxWidth,0];
      upperLeft[0] += 2*boxWidth;
      for(s32 i=1; i<=5; i++) {
        const Point<s16> point1 = Point<s16>(static_cast<s16>(upperLeft[1]+boxWidth*2*(i-1)), static_cast<s16>(upperLeft[0]));
        const Point<s16> point2 = Point<s16>(static_cast<s16>(upperLeft[1]+boxWidth+boxWidth*2*(i-1)), static_cast<s16>(upperLeft[0]+boxWidth));
        //    image = drawBox(image, upperLeft + [0, boxWidth*2*(i-1)], upperLeft + boxWidth + [0, boxWidth*2*(i-1)], i+5, lineColor, backgroundColor);
        DrawRectangle<Type>(image, point1, point2,  i+5, lineColor, backgroundColor);
      }

      //upperLeft = upperLeft + [2*boxWidth,0];
      upperLeft[0] += 2*boxWidth;
      for(s32 i=1; i<=5; i++) {
        const Point<s16> point1 = Point<s16>(static_cast<s16>(upperLeft[1]+boxWidth*2*(i-1)), static_cast<s16>(upperLeft[0]));
        const Point<s16> point2 = Point<s16>(static_cast<s16>(upperLeft[1]+boxWidth+boxWidth*2*(i-1)), static_cast<s16>(upperLeft[0]+boxWidth));
        //    image = drawBox(image, upperLeft + [0, boxWidth*2*(i-1)], upperLeft + boxWidth + [0, boxWidth*2*(i-1)], i+10, lineColor, backgroundColor);
        DrawRectangle<Type>(image, point1, point2,  i+10, lineColor, backgroundColor);
      }

      //upperLeft = upperLeft + [2*boxWidth,0];
      upperLeft[0] += 2*boxWidth;
      //speckWidths = 1:22;
      const s32 maxSpeckWidth = 22;
      for(s32 i=1; i<=maxSpeckWidth; i++) {
        //    image = drawBox(image, upperLeft + [0, 2*sum(speckWidths(1:i))], upperLeft + [0, i + 2*sum(speckWidths(1:i))], -1, lineColor, backgroundColor);

        s32 sumVal = 0;
        for(s32 iSum=1; iSum<=i; iSum++) {
          sumVal += iSum;
        }

        const Point<s16> point1 = Point<s16>(static_cast<s16>(upperLeft[1]+2*sumVal), static_cast<s16>(upperLeft[0]));
        const Point<s16> point2 = Point<s16>(static_cast<s16>(upperLeft[1]+i+2*sumVal), static_cast<s16>(upperLeft[0]));

        DrawRectangle<Type>(image, point1, point2,  -1, lineColor, backgroundColor);
      }

      //upperLeft = upperLeft + [10,0];
      upperLeft[0] += 10;
      for(s32 i=1; i<=maxSpeckWidth; i++) {
        //    image = drawBox(image, upperLeft + [0, 2*sum(speckWidths(1:i))], upperLeft + [1, i + 2*sum(speckWidths(1:i))], -1, lineColor, backgroundColor);

        s32 sumVal = 0;
        for(s32 iSum=1; iSum<=i; iSum++) {
          sumVal += iSum;
        }

        const Point<s16> point1 = Point<s16>(static_cast<s16>(upperLeft[1]+2*sumVal), static_cast<s16>(upperLeft[0]));
        const Point<s16> point2 = Point<s16>(static_cast<s16>(upperLeft[1]+i+2*sumVal), static_cast<s16>(upperLeft[0]+1));

        DrawRectangle<Type>(image, point1, point2,  -1, lineColor, backgroundColor);
      }

      //upperLeft = upperLeft + [10,0];
      upperLeft[0] += 10;
      for(s32 i=1; i<=maxSpeckWidth; i++) {
        //    image = drawBox(image, upperLeft + [0, 2*sum(speckWidths(1:i))], upperLeft + [2, i + 2*sum(speckWidths(1:i))], -1, lineColor, backgroundColor);
        s32 sumVal = 0;
        for(s32 iSum=1; iSum<=i; iSum++) {
          sumVal += iSum;
        }

        const Point<s16> point1 = Point<s16>(static_cast<s16>(upperLeft[1]+2*sumVal), static_cast<s16>(upperLeft[0]));
        const Point<s16> point2 = Point<s16>(static_cast<s16>(upperLeft[1]+i+2*sumVal), static_cast<s16>(upperLeft[0]+2));

        DrawRectangle<Type>(image, point1, point2,  -1, lineColor, backgroundColor);
      }

      //upperLeft = upperLeft + [10,0];
      upperLeft[0] += 10;
      for(s32 i=1; i<=maxSpeckWidth; i++) {
        //    image = drawBox(image, upperLeft + [0, 2*sum(speckWidths(1:i))], upperLeft + [4, i + 2*sum(speckWidths(1:i))], -1, lineColor, backgroundColor);
        s32 sumVal = 0;
        for(s32 iSum=1; iSum<=i; iSum++) {
          sumVal += iSum;
        }

        const Point<s16> point1 = Point<s16>(static_cast<s16>(upperLeft[1]+2*sumVal), static_cast<s16>(upperLeft[0]));
        const Point<s16> point2 = Point<s16>(static_cast<s16>(upperLeft[1]+i+2*sumVal), static_cast<s16>(upperLeft[0]+4));

        DrawRectangle<Type>(image, point1, point2,  -1, lineColor, backgroundColor);
      }

      //upperLeft = upperLeft + [13,0];
      upperLeft[0] += 13;
      for(s32 i=1; i<=maxSpeckWidth; i++) {
        //    image = drawBox(image, upperLeft + [0, 2*sum(speckWidths(1:i))], upperLeft + [8, i + 2*sum(speckWidths(1:i))], -1, lineColor, backgroundColor);
        s32 sumVal = 0;
        for(s32 iSum=1; iSum<=i; iSum++) {
          sumVal += iSum;
        }

        const Point<s16> point1 = Point<s16>(static_cast<s16>(upperLeft[1]+2*sumVal), static_cast<s16>(upperLeft[0]));
        const Point<s16> point2 = Point<s16>(static_cast<s16>(upperLeft[1]+i+2*sumVal), static_cast<s16>(upperLeft[0]+8));

        DrawRectangle<Type>(image, point1, point2,  -1, lineColor, backgroundColor);
      }

      return RESULT_OK;
    } // template<typename Type> Result DrawExampleSquaresImage(Array<Type> &image)
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_DRAW_H_
