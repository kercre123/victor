/**
File: draw.h
Author: Peter Barnum
Created: 2013

Definitions of draw_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_DRAW_H_
#define _ANKICORETECHEMBEDDED_COMMON_DRAW_H_

#include "coretech/common/robot/draw_declarations.h"
#include "coretech/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark

    template<typename Type> Result DrawRectangle(Array<Type> &image, Point<s16> point1, Point<s16> point2, const s32 lineWidth, const Type lineColor, const Type backgroundColor)
    {
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "DrawRectangle", "image is not valid");

      // Make point1 the upper left, and point2 the lower right
      if(point1.x > point2.x) Swap(point1.x, point2.x);
      if(point1.y > point2.y) Swap(point1.y, point2.y);

      Point<s16> point1inner;
      Point<s16> point2inner;

      s16 short_w = imageWidth-1;
      s16 short_h = imageHeight-1;
      if(lineWidth > 0) {
        point1inner.x = point1.x + static_cast<s16>(lineWidth);
        point1inner.y = point1.y + static_cast<s16>(lineWidth);
        point2inner.x = point2.x - static_cast<s16>(lineWidth);
        point2inner.y = point2.y - static_cast<s16>(lineWidth);

        point1inner.x = static_cast<s16>(CLIP(point1inner.x, 0, short_w));
        point1inner.y = static_cast<s16>(CLIP(point1inner.y, 0, short_h));
        point2inner.x = static_cast<s16>(CLIP(point2inner.x, 0, short_w));
        point2inner.y = static_cast<s16>(CLIP(point2inner.y, 0, short_h));
      }

      // Make the corners fit inside the image
      point1.x = static_cast<s16>(CLIP(point1.x, 0, short_w));
      point1.y = static_cast<s16>(CLIP(point1.y, 0, short_h));
      point2.x = static_cast<s16>(CLIP(point2.x, 0, short_w));
      point2.y = static_cast<s16>(CLIP(point2.y, 0, short_h));

      for(s32 y=point1.y; y<=point2.y; y++) {
        Type * restrict pImage = image.Pointer(y, 0);
        for(s32 x=point1.x; x<=point2.x; x++) {
          pImage[x] = lineColor;
        }
      }

      if(lineWidth > 0) {
        for(s32 y=point1inner.y; y<=point2inner.y; y++) {
          Type * restrict pImage = image.Pointer(y, 0);
          for(s32 x=point1inner.x; x<=point2inner.x; x++) {
            pImage[x] = backgroundColor;
          }
        }
      }

      return RESULT_OK;
    } // template<typename Type> Result DrawRectangle(Array<Type> &image, const Point<s16> point1, const Point<s16> point2, const s32 lineWidth, const Type lineColor, const Type backgroundColor)

    template<typename Type> Result DrawFilledConvexQuadrilateral(Array<Type> &image, Quadrilateral<f32> quad, const Type color)
    {
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "DrawFilledConvexQuadrilateral", "image is not valid");

      const Rectangle<f32> boundingRect = quad.ComputeBoundingRectangle<f32>();
      const Quadrilateral<f32> sortedQuad = quad.ComputeClockwiseCorners<f32>();

      const f32 rect_y0 = boundingRect.top;
      const f32 rect_y1 = boundingRect.bottom;

      // For circular indexing
      Point<f32> corners[5];
      for(s32 i=0; i<4; i++) {
        corners[i] = sortedQuad[i];
      }
      corners[4] = sortedQuad[0];

      const s32 minYS32 = MAX(0,             Round<s32>(ceilf(rect_y0 - 0.5f)));
      const s32 maxYS32 = MIN(imageHeight-1, Round<s32>(floorf(rect_y1 - 0.5f)));
      const f32 minYF32 = minYS32 + 0.5f;
      const f32 maxYF32 = maxYS32 + 0.5f;
      const LinearSequence<f32> ys(minYF32, maxYF32);
      const s32 numYs = ys.get_size();

      f32 y = ys.get_start();
      for(s32 iy=0; iy<numYs; iy++) {
        // Compute all intersections
        f32 minXF32 = FLT_MAX;
        f32 maxXF32 = FLT_MIN;
        for(s32 iCorner=0; iCorner<4; iCorner++) {
          if( (corners[iCorner].y < y && corners[iCorner+1].y >= y) || (corners[iCorner+1].y < y && corners[iCorner].y >= y) ) {
            const f32 dy = corners[iCorner+1].y - corners[iCorner].y;
            const f32 dx = corners[iCorner+1].x - corners[iCorner].x;

            const f32 alpha = (y - corners[iCorner].y) / dy;

            const f32 xIntercept = corners[iCorner].x + alpha * dx;

            minXF32 = MIN(minXF32, xIntercept);
            maxXF32 = MAX(maxXF32, xIntercept);
          }
        } // for(s32 iCorner=0; iCorner<4; iCorner++)

        const s32 minXS32 = MAX(0,            Round<s32>(floorf(minXF32+0.5f)));
        const s32 maxXS32 = MIN(imageWidth-1, Round<s32>(floorf(maxXF32-0.5f)));

        const s32 yS32 = minYS32 + iy;
        Type * restrict pImage = image.Pointer(yS32, 0);
        for(s32 x=minXS32; x<=maxXS32; x++) {
          pImage[x] = color;
        }

        y += 1.0f;
      } // for(s32 iy=0; iy<numYs; iy++)

      return RESULT_OK;
    } // template<typename Type> Result DrawFilledConvexQuadrilateral(Array<Type> &image, Quadrilateral<f32> quad, const Type color)

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

    template<typename Type> Result DrawPoints(const FixedLengthList<Point<Type> > &points, const u8 grayvalue, Array<u8> &image)
    {
      AnkiConditionalErrorAndReturnValue(AreValid(image, points),
        RESULT_FAIL_INVALID_OBJECT, "DrawPoints", "Invalid objects");

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      const s32 numPoints = points.get_size();

      const Point<Type> * pPoints = points.Pointer(0);

      for(s32 iPoint=0; iPoint<numPoints; iPoint++) {
        const s32 y = static_cast<s32>(pPoints[iPoint].y);
        const s32 x = static_cast<s32>(pPoints[iPoint].x);

        if(x >= 0 && x < imageWidth && y >= 0 && y < imageHeight) {
          image[y][x] = grayvalue;
        }
      }

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_DRAW_H_
