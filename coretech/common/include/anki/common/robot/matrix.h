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

      // Return the minimum element in this Array
      template<typename Type> Type Min(ConstArraySlice<Type> &mat);

      // Return the maximum element in this Array
      template<typename Type> Type Max(ConstArraySlice<Type> &mat);

      // For a square array, either:
      // 1. When lowerToUpper==true,  copies the lower (left)  triangle to the upper (right) triangle
      // 2. When lowerToUpper==false, copies the upper (right) triangle to the lower (left)  triangle
      // Functionally the same as OpenCV completeSymm()
      template<typename Type> Result MakeSymmetric(Type &arr, bool lowerToUpper = false);

      // Perform the matrix multiplication "matOut = mat1 * mat2"
      // Note that this is the naive O(n^3) implementation
      template<typename Type> Result Multiply(const Array<Type> &mat1, const Array<Type> &mat2, Array<Type> &matOut);

      // Return the sum of every element in the Array
      template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(ConstArraySlice<Array_Type> &mat);

#pragma mark --- Implementations ---

      template<typename Type> Type Min(ConstArraySlice<Type> &mat)
      {
        const Array<Type> &array = mat.get_array();

        AnkiConditionalErrorAndReturnValue(array.IsValid(),
          0, "Matrix::Min", "Array<Type> is not valid");

        const LinearSequence<s32>& xSlice = mat.get_xSlice();
        const LinearSequence<s32>& ySlice = mat.get_ySlice();

        const s32 xMin = xSlice.get_startValue();
        const s32 xIncrement = xSlice.get_increment();
        const s32 xMax = xSlice.get_endValue();

        const s32 yMin = ySlice.get_startValue();
        const s32 yIncrement = ySlice.get_increment();
        const s32 yMax = ySlice.get_endValue();

        Type minValue = *array.Pointer(yMin, xMin);
        for(s32 y=yMin; y<=yMax; y+=yIncrement) {
          const Type * const pMat = array.Pointer(y, 0);
          for(s32 x=xMin; x<=xMax; x+=xIncrement) {
            minValue = MIN(minValue, pMat[x]);
          }
        }

        return minValue;
      }

      template<typename Type> Type Max(ConstArraySlice<Type> &mat)
      {
        const Array<Type> &array = mat.get_array();

        AnkiConditionalErrorAndReturnValue(array.IsValid(),
          0, "Matrix::Max", "Array<Type> is not valid");

        const LinearSequence<s32>& xSlice = mat.get_xSlice();
        const LinearSequence<s32>& ySlice = mat.get_ySlice();

        const s32 xMin = xSlice.get_startValue();
        const s32 xIncrement = xSlice.get_increment();
        const s32 xMax = xSlice.get_endValue();

        const s32 yMin = ySlice.get_startValue();
        const s32 yIncrement = ySlice.get_increment();
        const s32 yMax = ySlice.get_endValue();

        Type maxValue = *array.Pointer(yMin, xMin);
        for(s32 y=yMin; y<=yMax; y+=yIncrement) {
          const Type * const pMat = array.Pointer(y, 0);
          for(s32 x=xMin; x<=xMax; x+=xIncrement) {
            maxValue = MAX(maxValue, pMat[x]);
          }
        }

        return maxValue;
      }

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

      template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(ConstArraySlice<Array_Type> &mat)
      {
        const Array<Array_Type> &array = mat.get_array();

        AnkiConditionalErrorAndReturnValue(array.IsValid(),
          0, "Matrix::Sum", "Array<Type> is not valid");

        const LinearSequence<s32>& xSlice = mat.get_xSlice();
        const LinearSequence<s32>& ySlice = mat.get_ySlice();

        const s32 xMin = xSlice.get_startValue();
        const s32 xIncrement = xSlice.get_increment();
        const s32 xMax = xSlice.get_endValue();

        const s32 yMin = ySlice.get_startValue();
        const s32 yIncrement = ySlice.get_increment();
        const s32 yMax = ySlice.get_endValue();

        Accumulator_Type sum = 0;
        for(s32 y=yMin; y<=yMax; y+=yIncrement) {
          const Array_Type * const pMat = array.Pointer(y, 0);
          for(s32 x=xMin; x<=xMax; x+=xIncrement) {
            sum += pMat[x];
          }
        }

        return sum;
      } // template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(const Array<Array_Type> &image)
    } // namespace Matrix
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_