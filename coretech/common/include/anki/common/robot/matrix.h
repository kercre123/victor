#ifndef _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_
#define _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/arraySlices.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Matrix
    {
#pragma mark --- Definitions ---

      //
      // Simple matrix statistics
      //

      // Return the minimum element in this Array
      template<typename Type> Type Min(const ConstArraySliceExpression<Type> &mat);

      // Return the maximum element in this Array
      template<typename Type> Type Max(const ConstArraySliceExpression<Type> &mat);

      // Return the sum of every element in the Array
      template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(const ConstArraySliceExpression<Array_Type> &mat);

      //
      // Elementwise matrix operations
      //

      // Elementwise add two arrays. in1, in2, and out can be the same array
      template<typename InType, typename OutType> Result Add(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);

      // Elementwise subtract two arrays. in1, in2, and out can be the same array
      template<typename InType, typename OutType> Result Subtract(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);

      // Elementwise multiply two arrays. in1, in2, and out can be the same array
      template<typename InType, typename OutType> Result DotMultiply(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<OutType> &in2, ArraySlice<OutType> out);

      // Elementwise divide two arrays. in1, in2, and out can be the same array
      template<typename InType, typename OutType> Result DotDivide(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);

      //
      // Standard matrix operations
      //

      // Perform the matrix multiplication "out = in1 * in2"
      // Note that this is the naive O(n^3) implementation
      template<typename InType, typename OutType> Result Multiply(const Array<InType> &in1, const Array<InType> &in2, Array<OutType> &out);

      //
      // Misc matrix operations
      //

      // For a square array, either:
      // 1. When lowerToUpper==true,  copies the lower (left)  triangle to the upper (right) triangle
      // 2. When lowerToUpper==false, copies the upper (right) triangle to the lower (left)  triangle
      // Functionally the same as OpenCV completeSymm()
      template<typename Type> Result MakeSymmetric(Type &arr, bool lowerToUpper = false);

#pragma mark --- Implementations ---

      template<typename Type> Type Min(const ConstArraySliceExpression<Type> &mat)
      {
        const Array<Type> &array = mat.get_array();

        AnkiConditionalErrorAndReturnValue(array.IsValid(),
          0, "Matrix::Min", "Array<Type> is not valid");

        const ArraySliceLimits_in1_out0<s32> limits(mat.get_ySlice(), mat.get_xSlice());

        AnkiConditionalErrorAndReturnValue(limits.isValid,
          0, "Matrix::Min", "Limits is not valid");

        Type minValue = *array.Pointer(limits.yStart, limits.xStart);
        for(s32 y=limits.yStart; y<=limits.yEnd; y+=limits.yIncrement) {
          const Type * restrict pMat = array.Pointer(y, 0);
          for(s32 x=limits.xStart; x<=limits.xEnd; x+=limits.xIncrement) {
            minValue = MIN(minValue, pMat[x]);
          }
        }

        return minValue;
      }

      template<typename Type> Type Max(const ConstArraySliceExpression<Type> &mat)
      {
        const Array<Type> &array = mat.get_array();

        AnkiConditionalErrorAndReturnValue(array.IsValid(),
          0, "Matrix::Max", "Array<Type> is not valid");

        const ArraySliceLimits_in1_out0<s32> limits(mat.get_ySlice(), mat.get_xSlice());

        AnkiConditionalErrorAndReturnValue(limits.isValid,
          0, "Matrix::Max", "Limits is not valid");

        Type maxValue = *array.Pointer(limits.yStart, limits.xStart);
        for(s32 y=limits.yStart; y<=limits.yEnd; y+=limits.yIncrement) {
          const Type * restrict pMat = array.Pointer(y, 0);
          for(s32 x=limits.xStart; x<=limits.xEnd; x+=limits.xIncrement) {
            maxValue = MAX(maxValue, pMat[x]);
          }
        }

        return maxValue;
      }

      template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(const ConstArraySliceExpression<Array_Type> &mat)
      {
        const Array<Array_Type> &array = mat.get_array();

        AnkiConditionalErrorAndReturnValue(array.IsValid(),
          0, "Matrix::Sum", "Array<Type> is not valid");

        const ArraySliceLimits_in1_out0<s32> limits(mat.get_ySlice(), mat.get_xSlice());

        AnkiConditionalErrorAndReturnValue(limits.isValid,
          0, "Matrix::Sum", "Limits is not valid");

        Accumulator_Type sum = 0;
        for(s32 y=limits.yStart; y<=limits.yEnd; y+=limits.yIncrement) {
          const Array_Type * restrict pMat = array.Pointer(y, 0);
          for(s32 x=limits.xStart; x<=limits.xEnd; x+=limits.xIncrement) {
            sum += pMat[x];
          }
        }

        return sum;
      } // template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(const Array<Array_Type> &image)

      template<typename InType, typename OutType> Result Add(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
      {
        const Array<InType> &in1Array = in1.get_array();
        const Array<InType> &in2Array = in2.get_array();
        Array<OutType> &out1Array = out.get_array();

        AnkiConditionalErrorAndReturnValue(in1Array.IsValid(),
          RESULT_FAIL, "Matrix::Add", "Invalid array in1");

        AnkiConditionalErrorAndReturnValue(in2Array.IsValid(),
          RESULT_FAIL, "Matrix::Add", "Invalid array in2");

        AnkiConditionalErrorAndReturnValue(out1Array.IsValid(),
          RESULT_FAIL, "Matrix::Add", "Invalid array out");

        ArraySliceLimits_in2_out1<s32> limits(
          in1.get_ySlice(), in1.get_xSlice(), in1.get_isTransposed(),
          in2.get_ySlice(), in2.get_xSlice(), in2.get_isTransposed(),
          out.get_ySlice(), out.get_xSlice());

        AnkiConditionalErrorAndReturnValue(limits.isValid,
          RESULT_FAIL, "Matrix::Add", "Limits is not valid");

        if(limits.isSimpleIteration) {
          // If the input isn't transposed, we will do the maximally efficient loop iteration

          for(s32 y=0; y<limits.out1_ySize; y++) {
            const InType * const pIn1 = in1Array.Pointer(limits.in1Y, 0);
            const InType * const pIn2 = in2Array.Pointer(limits.in2Y, 0);
            OutType * const pOut1 = out1Array.Pointer(limits.out1Y, 0);

            limits.IncrementTop();

            for(s32 x=0; x<limits.out1_xSize; x++) {
              pOut1[limits.out1X] = pIn1[limits.in1X] + pIn2[limits.in2X];

              limits.in1X += limits.in1_xIncrement;
              limits.in2X += limits.in2_xIncrement;
              limits.out1X += limits.out1_xIncrement;
            }

            limits.IncrementBottom();
          }
        } else { // if(limits.isSimpleIteration)
          // If either input is transposed is allowed, then we will do an inefficent loop iteration

          for(s32 y=0; y<limits.out1_ySize; y++) {
            OutType * const pOut1 = out1Array.Pointer(limits.out1Y, 0);

            limits.IncrementTop();

            for(s32 x=0; x<limits.out1_xSize; x++) {
              const InType valIn1 = *in1Array.Pointer(limits.in1Y, limits.in1X);
              const InType valIn2 = *in2Array.Pointer(limits.in2Y, limits.in2X);

              pOut1[limits.out1X] = valIn1 + valIn2;

              limits.in1X += limits.in1InnerIncrementX;
              limits.in1Y += limits.in1InnerIncrementY;
              limits.in2X += limits.in2InnerIncrementX;
              limits.in2Y += limits.in2InnerIncrementY;
              limits.out1X += limits.out1_xIncrement;
            }

            limits.IncrementBottom();
          }
        } //   if(limits.isSimpleIteration)  ... else

        return RESULT_OK;
      } // template<typename Type> Result Add(const ConstArraySliceExpression<Type> &in1, const ConstArraySliceExpression<Type> &in2, const ArraySlice<Type> &out)

      template<typename Type> Result Subtract(const ConstArraySliceExpression<Type> &in1, const ConstArraySliceExpression<Type> &in2, const ArraySlice<Type> &out)
      {
      } // template<typename Type> Result Subtract(const ConstArraySliceExpression<Type> &in1, const ConstArraySliceExpression<Type> &in2, const ArraySlice<Type> &out)

      template<typename Type> Result DotMultiply(const ConstArraySliceExpression<Type> &in1, const ConstArraySliceExpression<Type> &in2, const ArraySlice<Type> &out)
      {
      } // template<typename Type> Result DotMultiply(const ConstArraySliceExpression<Type> &in1, const ConstArraySliceExpression<Type> &in2, const ArraySlice<Type> &out)

      template<typename Type> Result DotDivide(const ConstArraySliceExpression<Type> &in1, const ConstArraySliceExpression<Type> &in2, const ArraySlice<Type> &out)
      {
      } // template<typename Type> Result DotDivide(const ConstArraySliceExpression<Type> &in1, const ConstArraySliceExpression<Type> &in2, const ArraySlice<Type> &out)

      template<typename InType, typename OutType> Result Multiply(const Array<InType> &in1, const Array<InType> &in2, Array<OutType> &out)
      {
        const s32 in1Height = in1.get_size(0);
        const s32 in1Width = in1.get_size(1);

        const s32 in2Height = in2.get_size(0);
        const s32 in2Width = in2.get_size(1);

        AnkiConditionalErrorAndReturnValue(in1Width == in2Height,
          RESULT_FAIL, "Multiply", "Input matrices are incompatible sizes");

        AnkiConditionalErrorAndReturnValue(out.get_size(0) == in1Height,
          RESULT_FAIL, "Multiply", "Input and Output matrices are incompatible sizes");

        AnkiConditionalErrorAndReturnValue(out.get_size(1) == in2Width,
          RESULT_FAIL, "Multiply", "Input and Output matrices are incompatible sizes");

        for(s32 y1=0; y1<in1Height; y1++) {
          const InType * restrict pMat1 = in1.Pointer(y1, 0);
          OutType * restrict pMatOut = out.Pointer(y1, 0);

          for(s32 x2=0; x2<in2Width; x2++) {
            pMatOut[x2] = 0;

            for(s32 y2=0; y2<in2Height; y2++) {
              pMatOut[x2] += pMat1[y2] * (*in2.Pointer(y2, x2));
            }
          }
        }

        return RESULT_OK;
      } // template<typename InType, typename OutType> Result Multiply(const Array<InType> &in1, const Array<InType> &in2, Array<OutType> &out)

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
    } // namespace Matrix
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_