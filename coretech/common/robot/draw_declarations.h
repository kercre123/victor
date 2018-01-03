/**
File: draw_declarations.h
Author: Peter Barnum
Created: 2013

Simple functions to draw shapes on an Array

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_DRAW_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_DRAW_DECLARATIONS_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    // Draw a box with the two opposing corners point1 and point2
    // If lineWidth < 1, then the box will be solid. Otherwise, the inside will be filled with backgroundColor
    template<typename Type> Result DrawRectangle(Array<Type> &image, Point<s16> point1, Point<s16> point2, const s32 lineWidth, const Type lineColor, const Type backgroundColor);

    // Fill in the pixels inside quad. The center of the top-left pixel is (0.5,0.5).
    // If the quad is not convex, this function will fill a set of pixels such that: convexHull >= numPixelsFilled >= nonConvexFill
    template<typename Type> Result DrawFilledConvexQuadrilateral(Array<Type> &image, Quadrilateral<f32> quad, const Type color);

    // Create a test pattern image, full of different types of squares
    // Example:
    // Array<u8> testSquaresImage = AllocateArrayFromHeap<u8>(480,640);
    // DrawExampleSquaresImage(testSquaresImage);
    // testSquaresImage.Show("testSquaresImage", true);
    template<typename Type> Result DrawExampleSquaresImage(Array<Type> &image);

    // points is a list of point coordinates. DrawPoints draws them on image.
    template<typename Type> Result DrawPoints(const FixedLengthList<Point<Type> > &points, const u8 grayvalue, Array<u8> &image);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_DRAW_DECLARATIONS_H_
