#ifndef _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_
#define _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Matrix
    {
#pragma mark --- Definitions ---
      // For a square array, either:
      // 1. When lowerToUpper==true,  copies the lower (left)  triangle to the upper (right) triangle
      // 2. When lowerToUpper==false, copies the upper (right) triangle to the lower (left)  triangle
      // Functionally the same as OpenCV completeSymm()
      template<typename Type> Result MakeSymmetric(Type &arr, bool lowerToUpper = false);

      // Perform the matrix multiplication "matOut = mat1 * mat2"
      // Note that this is the naive O(n^3) implementation
      template<typename Type> Result Multiply(const Array<Type> &mat1, const Array<Type> &mat2, Array<Type> &matOut);

      // Return the sum of every element in the Array
      template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(const Array<Array_Type> &image);

#pragma mark --- Implementations ---

      template<typename Type> Result MakeSymmetric(Type &arr, bool lowerToUpper)
      {
        AnkiConditionalErrorAndReturnValue(arr.get_size(0) == arr.get_size(1),
          RESULT_FAIL, "copyHalfArray", "Input array must be square");

        const s32 arrHeight = arr.get_size(0);
        for(s32 y = 0; y < arrHeight; y++)
        {
          const s32 x0 = lowerToUpper ? (y+1)     : 0;
          const s32 x1 = lowerToUpper ? arrHeight : y;

          for(s32 x = x0; x < x1; x++) {
            *arr.Pointer(y,x) = *arr.Pointer(x,y);
          }
        }

        return RESULT_OK;
      } // template<typename Type> Result MakeSymmetric(Type &arr, bool lowerToUpper)

      template<typename Type> Result Multiply(const Array<Type> &mat1, const Array<Type> &mat2, Array<Type> &matOut)
      {
        const s32 mat1Height = mat1.get_size(0);
        const s32 mat1Width = mat1.get_size(1);

        const s32 mat2Height = mat2.get_size(0);
        const s32 mat2Width = mat2.get_size(1);

        AnkiConditionalErrorAndReturnValue(mat1Width == mat2Height,
          RESULT_FAIL, "Multiply", "Input matrices are incompatible sizes");

        AnkiConditionalErrorAndReturnValue(matOut.get_size(0) == mat1Height,
          RESULT_FAIL, "Multiply", "Input and Output matrices are incompatible sizes");

        AnkiConditionalErrorAndReturnValue(matOut.get_size(1) == mat2Width,
          RESULT_FAIL, "Multiply", "Input and Output matrices are incompatible sizes");

        for(s32 y1=0; y1<mat1Height; y1++) {
          const Type * restrict pMat1 = mat1.Pointer(y1, 0);
          Type * restrict pMatOut = matOut.Pointer(y1, 0);

          for(s32 x2=0; x2<mat2Width; x2++) {
            pMatOut[x2] = 0;

            for(s32 y2=0; y2<mat2Height; y2++) {
              pMatOut[x2] += pMat1[y2] * (*mat2.Pointer(y2, x2));
            }
          }
        }

        return RESULT_OK;
      } // template<typename Array_Type, typename Type> Result Multiply(const Array_Type &mat1, const Array_Type &mat2, Array_Type &matOut)

      template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(const Array<Array_Type> &image)
      {
        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        Accumulator_Type sum = 0;
        for(s32 y=0; y<imageHeight; y++) {
          const Array_Type * const pImage = image.Pointer(y, 0);
          for(s32 x=0; x<imageWidth; x++) {
            sum += pImage[x];
          }
        }

        return sum;
      } // template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(const Array<Array_Type> &image)
    } // namespace Matrix
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_