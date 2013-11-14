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

        const ArraySliceLimits<Type> matLimits(mat);

        Type minValue = *array.Pointer(matLimits.yStart, matLimits.xStart);
        for(s32 y=matLimits.yStart; y<=matLimits.yEnd; y+=matLimits.yIncrement) {
          const Type * restrict pMat = array.Pointer(y, 0);
          for(s32 x=matLimits.xStart; x<=matLimits.xEnd; x+=matLimits.xIncrement) {
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

        const ArraySliceLimits<Type> matLimits(mat);

        Type maxValue = *array.Pointer(matLimits.yStart, matLimits.xStart);
        for(s32 y=matLimits.yStart; y<=matLimits.yEnd; y+=matLimits.yIncrement) {
          const Type * restrict pMat = array.Pointer(y, 0);
          for(s32 x=matLimits.xStart; x<=matLimits.xEnd; x+=matLimits.xIncrement) {
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

        const ArraySliceLimits<Array_Type> matLimits(mat);

        Accumulator_Type sum = 0;
        for(s32 y=matLimits.yStart; y<=matLimits.yEnd; y+=matLimits.yIncrement) {
          const Array_Type * restrict pMat = array.Pointer(y, 0);
          for(s32 x=matLimits.xStart; x<=matLimits.xEnd; x+=matLimits.xIncrement) {
            sum += pMat[x];
          }
        }

        return sum;
      } // template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(const Array<Array_Type> &image)

      template<typename InType, typename OutType> Result Add(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
      {
        const Array<InType> &in1Array = in1.get_array();
        const Array<InType> &in2Array = in2.get_array();
        Array<OutType> &outArray = out.get_array();

        AnkiConditionalErrorAndReturnValue(in1Array.IsValid(),
          RESULT_FAIL, "ArraySlice<Type>::Set", "Invalid array in1");

        AnkiConditionalErrorAndReturnValue(in2Array.IsValid(),
          RESULT_FAIL, "ArraySlice<Type>::Set", "Invalid array in2");

        AnkiConditionalErrorAndReturnValue(outArray.IsValid(),
          RESULT_FAIL, "ArraySlice<Type>::Set", "Invalid array out");

        const ArraySliceLimits<InType> in1Limits(in1);
        const ArraySliceLimits<InType> in2Limits(in2);
        const ArraySliceLimits<OutType> outLimits(out);

        const bool sizesMatch = (in1Limits.xSize == in2Limits.xSize) && (in1Limits.xSize == outLimits.xSize) && (in1Limits.ySize == in2Limits.ySize) && (in1Limits.ySize == outLimits.ySize);
        if(sizesMatch) {
          // If the input isn't transposed, we will do the maximally efficient loop iteration

          s32 in1Y = in1Limits.yStart;
          s32 in2Y = in2Limits.yStart;
          s32 outY = outLimits.yStart;

          for(s32 y=0; y<outLimits.ySize; y++) {
            const InType * const pIn1 = in1Array.Pointer(in1Y, 0);
            const InType * const pIn2 = in2Array.Pointer(in2Y, 0);
            OutType * const pOut = outArray.Pointer(outY, 0);

            s32 in1X = in1Limits.xStart;
            s32 in2X = in2Limits.xStart;
            s32 outX = outLimits.xStart;

            for(s32 x=0; x<outLimits.xSize; x++) {
              pOut[outX] = pIn1[in1X] + pIn2[in2X];

              in1X += in1Limits.xIncrement;
              in2X += in2Limits.xIncrement;
              outX += outLimits.xIncrement;
            }

            in1Y += in1Limits.yIncrement;
            in2Y += in2Limits.yIncrement;
            outY += outLimits.yIncrement;
          }
        } else if(in1.get_isTransposed() || in2.get_isTransposed()) {
          // If the input is transposed or if automaticTransposing is allowed, then we will do an inefficent loop iteration
          // TODO: make fast if needed
          const bool in1Transposed = in1.get_isTransposed();
          const bool in2Transposed = in2.get_isTransposed();

          bool sizesMatch = false;

          s32 in1InnerIncrementY = 0;
          s32 in1InnerIncrementX = 0;
          s32 in2InnerIncrementY = 0;
          s32 in2InnerIncrementX = 0;

          if(in1Transposed && in2Transposed) {
            sizesMatch = (in1Limits.xSize == in2Limits.xSize) && (in1Limits.xSize == outLimits.ySize) && (in1Limits.ySize == in2Limits.ySize) && (in1Limits.ySize == outLimits.xSize);
            in1InnerIncrementY = in1Limits.yIncrement;
            in2InnerIncrementY = in2Limits.yIncrement;
          } else if(in1Transposed) {
            sizesMatch = (in1Limits.xSize == in2Limits.ySize) && (in1Limits.xSize == outLimits.ySize) && (in1Limits.ySize == in2Limits.xSize) && (in1Limits.ySize == outLimits.xSize);
            in1InnerIncrementY = in1Limits.yIncrement;
            in2InnerIncrementX = in2Limits.xIncrement;
          } else if(in2Transposed) {
            sizesMatch = (in1Limits.xSize == in2Limits.ySize) && (in1Limits.xSize == outLimits.xSize) && (in1Limits.ySize == in2Limits.xSize) && (in1Limits.ySize == outLimits.ySize);
            in1InnerIncrementX = in1Limits.xIncrement;
            in2InnerIncrementY = in2Limits.yIncrement;
          } else {
            assert(false); // should not be possible
          }

          if(!sizesMatch) {
            AnkiError("ArraySlice<Type>::Add", "Subscripted assignment dimension mismatch");
            return RESULT_FAIL;
          }

          s32 in1X = in1Limits.xStart;
          s32 in1Y = in1Limits.yStart;
          s32 in2X = in2Limits.xStart;
          s32 in2Y = in2Limits.yStart;

          s32 outY = outLimits.yStart;

          // TODO: replace the Pointer() call with an addition, if speed is a problem
          for(s32 y=0; y<outLimits.ySize; y++) {
            OutType * const pOut = outArray.Pointer(outY, 0);

            s32 outX = outLimits.xStart;

            if(in1Transposed) {
              in1Y = in1Limits.yStart;
            } else {
              in1X = in1Limits.xStart;
            }

            if(in2Transposed) {
              in2Y = in2Limits.yStart;
            } else {
              in2X = in2Limits.xStart;
            }

            for(s32 x=0; x<outLimits.xSize; x++) {
              const InType pIn1 = *in1Array.Pointer(in1Y, in1X);
              const InType pIn2 = *in2Array.Pointer(in2Y, in2X);

              pOut[outX] = pIn1 + pIn2;

              in1X += in1InnerIncrementX;
              in1Y += in1InnerIncrementY;
              in2X += in2InnerIncrementX;
              in2Y += in2InnerIncrementY;
              outX += outLimits.xIncrement;
            }

            outY += outLimits.yIncrement;

            if(in1Transposed) {
              in1X += in1Limits.xIncrement;
            } else {
              in1Y += in1Limits.yIncrement;
            }

            if(in2Transposed) {
              in2X += in2Limits.xIncrement;
            } else {
              in2Y += in2Limits.yIncrement;
            }
          }
        }

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