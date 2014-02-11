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

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d_declarations.h"

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

    // points is a list of point coordinates. DrawPoints draws them on image.
    template<typename Type> Result DrawPoints(const FixedLengthList<Point<Type> > &points, const u8 grayvalue, Array<u8> &image);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_DRAW_DECLARATIONS_H_