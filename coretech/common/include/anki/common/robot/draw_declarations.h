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
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_DRAW_DECLARATIONS_H_