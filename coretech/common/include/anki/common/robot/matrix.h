/**
File: matrix.h
Author: Peter Barnum
Created: 2013

Definitions of matrix_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_
#define _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_

#include "anki/common/robot/matrix_declarations.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/arraySlices.h"
#include "anki/common/robot/trig_fast.h"
#include "anki/common/robot/benchmarking.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Matrix
    {
      template<typename Type> void InsertionSort_sortAscendingDimension0(Array<Type> &arr, const s32 trueMinIndex, const s32 trueMaxIndex);
      template<typename Type> void InsertionSort_sortDescendingDimension0(Array<Type> &arr, const s32 trueMinIndex, const s32 trueMaxIndex);
      template<typename Type> void InsertionSort_sortAscendingDimension1(Array<Type> &arr, const s32 trueMinIndex, const s32 trueMaxIndex);
      template<typename Type> void InsertionSort_sortDescendingDimension1(Array<Type> &arr, const s32 trueMinIndex, const s32 trueMaxIndex);

      template<typename Type> void InsertionSort_sortAscendingDimension0(Array<Type> &arr, Array<s32> &indexes, const s32 trueMinIndex, const s32 trueMaxIndex);
      template<typename Type> void InsertionSort_sortDescendingDimension0(Array<Type> &arr, Array<s32> &indexes, const s32 trueMinIndex, const s32 trueMaxIndex);
      template<typename Type> void InsertionSort_sortAscendingDimension1(Array<Type> &arr, Array<s32> &indexes, const s32 trueMinIndex, const s32 trueMaxIndex);
      template<typename Type> void InsertionSort_sortDescendingDimension1(Array<Type> &arr, Array<s32> &indexes, const s32 trueMinIndex, const s32 trueMaxIndex);

      template<typename Type> Type Min(const ConstArraySliceExpression<Type> &mat)
      {
        const Array<Type> &array = mat.get_array();

        AnkiConditionalErrorAndReturnValue(array.IsValid(),
          0, "Matrix::Min", "Array<Type> is not valid");

        const ArraySliceLimits_in1_out0<s32> limits(mat.get_ySlice(), mat.get_xSlice());

        AnkiConditionalErrorAndReturnValue(limits.isValid,
          0, "Matrix::Min", "Limits is not valid");

        Type minValue = *array.Pointer(limits.rawIn1Limits.yStart, limits.rawIn1Limits.xStart);
        for(s32 y=limits.rawIn1Limits.yStart; y<=limits.rawIn1Limits.yEnd; y+=limits.rawIn1Limits.yIncrement) {
          const Type * restrict pMat = array.Pointer(y, 0);
          for(s32 x=limits.rawIn1Limits.xStart; x<=limits.rawIn1Limits.xEnd; x+=limits.rawIn1Limits.xIncrement) {
            minValue = MIN(minValue, pMat[x]);
          }
        }

        return minValue;
      } // template<typename Type> Type Min(const ConstArraySliceExpression<Type> &mat)

      template<typename Type> Type Max(const ConstArraySliceExpression<Type> &mat)
      {
        const Array<Type> &array = mat.get_array();

        AnkiConditionalErrorAndReturnValue(array.IsValid(),
          0, "Matrix::Max", "Array<Type> is not valid");

        const ArraySliceLimits_in1_out0<s32> limits(mat.get_ySlice(), mat.get_xSlice());

        AnkiConditionalErrorAndReturnValue(limits.isValid,
          0, "Matrix::Max", "Limits is not valid");

        Type maxValue = *array.Pointer(limits.rawIn1Limits.yStart, limits.rawIn1Limits.xStart);
        for(s32 y=limits.rawIn1Limits.yStart; y<=limits.rawIn1Limits.yEnd; y+=limits.rawIn1Limits.yIncrement) {
          const Type * restrict pMat = array.Pointer(y, 0);
          for(s32 x=limits.rawIn1Limits.xStart; x<=limits.rawIn1Limits.xEnd; x+=limits.rawIn1Limits.xIncrement) {
            maxValue = MAX(maxValue, pMat[x]);
          }
        }

        return maxValue;
      } // template<typename Type> Type Max(const ConstArraySliceExpression<Type> &mat)

      template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(const ConstArraySliceExpression<Array_Type> &mat)
      {
        const Array<Array_Type> &array = mat.get_array();

        AnkiConditionalErrorAndReturnValue(array.IsValid(),
          0, "Matrix::Sum", "Array<Type> is not valid");

        const ArraySliceLimits_in1_out0<s32> limits(mat.get_ySlice(), mat.get_xSlice());

        AnkiConditionalErrorAndReturnValue(limits.isValid,
          0, "Matrix::Sum", "Limits is not valid");

        Accumulator_Type sum = 0;
        for(s32 y=limits.rawIn1Limits.yStart; y<=limits.rawIn1Limits.yEnd; y+=limits.rawIn1Limits.yIncrement) {
          const Array_Type * restrict pMat = array.Pointer(y, 0);
          for(s32 x=limits.rawIn1Limits.xStart; x<=limits.rawIn1Limits.xEnd; x+=limits.rawIn1Limits.xIncrement) {
            sum += pMat[x];
          }
        }

        return sum;
      } // template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(const Array<Array_Type> &image)

      template<typename Array_Type, typename Accumulator_Type> Array_Type Mean(const ConstArraySliceExpression<Array_Type> &mat)
      {
        const Accumulator_Type sum = Sum<Array_Type,Accumulator_Type>(mat);
        const Accumulator_Type numElements = static_cast<Accumulator_Type>(mat.get_ySlice().get_size() * mat.get_xSlice().get_size());
        const Array_Type mean = static_cast<Array_Type>(sum / numElements);

        return mean;
      } // template<typename Array_Type, typename Accumulator_Type> Array_Type Mean(const ConstArraySliceExpression<Array_Type> &mat)

      template<typename Array_Type, typename Accumulator_Type> Result MeanAndVar(const ConstArraySliceExpression<Array_Type> &mat,
        Accumulator_Type& mean, Accumulator_Type& var)
      {
        const Array<Array_Type> &array = mat.get_array();

        AnkiConditionalErrorAndReturnValue(array.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "Matrix::MeanAndVar", "Array<Type> is not valid");

        const ArraySliceLimits_in1_out0<s32> limits(mat.get_ySlice(), mat.get_xSlice());

        AnkiConditionalErrorAndReturnValue(limits.isValid,
          RESULT_FAIL_INVALID_OBJECT, "Matrix::MeanAndVar", "Limits is not valid");

        Accumulator_Type sum = 0;
        Accumulator_Type sumSq = 0;
        for(s32 y=limits.rawIn1Limits.yStart; y<=limits.rawIn1Limits.yEnd; y+=limits.rawIn1Limits.yIncrement) {
          const Array_Type * restrict pMat = array.Pointer(y, 0);
          for(s32 x=limits.rawIn1Limits.xStart; x<=limits.rawIn1Limits.xEnd; x+=limits.rawIn1Limits.xIncrement) {
            const Accumulator_Type val = static_cast<Accumulator_Type>(pMat[x]);
            sum   += val;
            sumSq += val*val;
          }
        }

        const Accumulator_Type numElements = static_cast<Accumulator_Type>(mat.get_ySlice().get_size() * mat.get_xSlice().get_size());

        mean = sum / numElements;                  // mean = E[x]
        var  = (sumSq / numElements) - (mean*mean);  // var  = E[x^2] - E[x]^2

        return RESULT_OK;
      } // template<typename Array_Type, typename Accumulator_Type> Accumulator_Type Sum(const Array<Array_Type> &image)

      template<typename InType, typename IntermediateType, typename OutType> Result Add(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::Add<InType, IntermediateType, OutType>, OutType>(in1, in2, out);
      } // template<typename Type> Result Add(const ConstArraySliceExpression<Type> &in1, const ConstArraySliceExpression<Type> &in2, const ArraySlice<Type> &out)

      template<typename InType, typename IntermediateType, typename OutType> Result Add(const ConstArraySliceExpression<InType> &in1, const InType value2, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::Add<InType, IntermediateType, OutType>, OutType>(in1, value2, out);
      }

      template<typename InType, typename IntermediateType, typename OutType> Result Add(const InType value1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::Add<InType, IntermediateType, OutType>, OutType>(value1, in2, out);
      }

      template<typename InType, typename IntermediateType, typename OutType> Result Subtract(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::Subtract<InType, IntermediateType, OutType>, OutType>(in1, in2, out);
      } // template<typename Type> Result Subtract(const ConstArraySliceExpression<Type> &in1, const ConstArraySliceExpression<Type> &in2, const ArraySlice<Type> &out)

      template<typename InType, typename IntermediateType, typename OutType> Result Subtract(const ConstArraySliceExpression<InType> &in1, const InType value2, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::Subtract<InType, IntermediateType, OutType>, OutType>(in1, value2, out);
      }

      template<typename InType, typename IntermediateType, typename OutType> Result Subtract(const InType value1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::Subtract<InType, IntermediateType, OutType>, OutType>(value1, in2, out);
      }

      template<typename InType, typename IntermediateType, typename OutType> Result DotMultiply(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::DotMultiply<InType, IntermediateType, OutType>, OutType>(in1, in2, out);
      } // template<typename Type> Result DotMultiply(const ConstArraySliceExpression<Type> &in1, const ConstArraySliceExpression<Type> &in2, const ArraySlice<Type> &out)

      template<typename InType, typename IntermediateType, typename OutType> Result DotMultiply(const ConstArraySliceExpression<InType> &in1, const InType value2, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::DotMultiply<InType, IntermediateType, OutType>, OutType>(in1, value2, out);
      }

      template<typename InType, typename IntermediateType, typename OutType> Result DotMultiply(const InType value1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::DotMultiply<InType, IntermediateType, OutType>, OutType>(value1, in2, out);
      }

      template<typename InType, typename IntermediateType, typename OutType> Result DotDivide(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::DotDivide<InType, IntermediateType, OutType>, OutType>(in1, in2, out);
      } // template<typename Type> Result DotDivide(const ConstArraySliceExpression<Type> &in1, const ConstArraySliceExpression<Type> &in2, const ArraySlice<Type> &out)

      template<typename InType, typename IntermediateType, typename OutType> Result DotDivide(const ConstArraySliceExpression<InType> &in1, const InType value2, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::DotDivide<InType, IntermediateType, OutType>, OutType>(in1, value2, out);
      }

      template<typename InType, typename IntermediateType, typename OutType> Result DotDivide(const InType value1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::DotDivide<InType, IntermediateType, OutType>, OutType>(value1, in2, out);
      }

      template<typename InType, typename IntermediateType, typename OutType> Result Exp(const ConstArraySliceExpression<InType> &in, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::Exp<InType, IntermediateType, OutType>, OutType>(in, in, out);
      } // template<typename Type> Result Exp(const ConstArraySliceExpression<InType> &in, ArraySlice<OutType> out)

      template<typename InType, typename IntermediateType, typename OutType> Result Sqrt(const ConstArraySliceExpression<InType> &in, ArraySlice<OutType> out)
      {
        return Elementwise::ApplyOperation<InType, Elementwise::Sqrt<InType, IntermediateType, OutType>, OutType>(in, in, out);
      }

      template<typename InType, typename OutType> Result Multiply(const Array<InType> &in1, const Array<InType> &in2, Array<OutType> &out)
      {
        const s32 in1Height = in1.get_size(0);
        const s32 in1Width = in1.get_size(1);

        const s32 in2Height = in2.get_size(0);
        const s32 in2Width = in2.get_size(1);
        const s32 in2Stride = in2.get_stride();

        AnkiConditionalErrorAndReturnValue(in1Width == in2Height,
          RESULT_FAIL_INVALID_SIZE, "Multiply", "Input matrices are incompatible sizes");

        AnkiConditionalErrorAndReturnValue(out.get_size(0) == in1Height,
          RESULT_FAIL_INVALID_SIZE, "Multiply", "Input and Output matrices are incompatible sizes");

        AnkiConditionalErrorAndReturnValue(out.get_size(1) == in2Width,
          RESULT_FAIL_INVALID_SIZE, "Multiply", "Input and Output matrices are incompatible sizes");

        for(s32 y1=0; y1<in1Height; y1++) {
          const InType * restrict pIn1 = in1.Pointer(y1, 0);
          OutType * restrict pOut = out.Pointer(y1, 0);

          for(s32 x2=0; x2<in2Width; x2++) {
            const u8 * restrict pIn2 = reinterpret_cast<const u8*>(in2.Pointer(0, x2));

            OutType accumulator = 0;

            s32 y2;
            for(y2=0; y2<in2Height-3; y2+=4) {
              const InType in1_0 = pIn1[y2];
              const InType in1_1 = pIn1[y2+1];
              const InType in1_2 = pIn1[y2+2];
              const InType in1_3 = pIn1[y2+3];

              const InType in2_0 = *reinterpret_cast<const InType*>(pIn2);
              const InType in2_1 = *reinterpret_cast<const InType*>(pIn2 + in2Stride);
              const InType in2_2 = *reinterpret_cast<const InType*>(pIn2 + 2*in2Stride);
              const InType in2_3 = *reinterpret_cast<const InType*>(pIn2 + 3*in2Stride);

              accumulator +=
                in1_0 * in2_0 +
                in1_1 * in2_1 +
                in1_2 * in2_2 +
                in1_3 * in2_3;

              pIn2 += 4*in2Stride;
            }

            for(; y2<in2Height; y2++) {
              accumulator += pIn1[y2] * (*reinterpret_cast<const InType*>(pIn2));

              pIn2 += in2Stride;
            }

            pOut[x2] = accumulator;
          }
        }

        return RESULT_OK;
      } // template<typename InType, typename OutType> Result Multiply(const Array<InType> &in1, const Array<InType> &in2, Array<OutType> &out)

      template<typename InType, typename OutType> NO_INLINE Result MultiplyTranspose(const Array<InType> &in1, const Array<InType> &in2Transposed, Array<OutType> &out)
      {
        const s32 in1Height = in1.get_size(0);
        const s32 in1Width = in1.get_size(1);

        const s32 in2TransposedHeight = in2Transposed.get_size(0);
        const s32 in2TransposedWidth = in2Transposed.get_size(1);

        AnkiConditionalErrorAndReturnValue(in1Width == in2TransposedWidth,
          RESULT_FAIL_INVALID_SIZE, "Multiply", "Input matrices are incompatible sizes");

        AnkiConditionalErrorAndReturnValue(out.get_size(0) == in1Height,
          RESULT_FAIL_INVALID_SIZE, "Multiply", "Input and Output matrices are incompatible sizes");

        AnkiConditionalErrorAndReturnValue(out.get_size(1) == in2TransposedHeight,
          RESULT_FAIL_INVALID_SIZE, "Multiply", "Input and Output matrices are incompatible sizes");

        for(s32 y1=0; y1<in1Height; y1++)
        {
          const InType * restrict pIn1 = in1.Pointer(y1, 0);

          for(s32 y2=0; y2<in2TransposedHeight; y2++) {
            const InType * restrict pIn2 = in2Transposed.Pointer(y2, 0);

            OutType accumulator = 0;

            s32 x;
            for(x=0; x<in2TransposedWidth-3; x+=4) {
              const InType in1_0 = pIn1[x];
              const InType in1_1 = pIn1[x+1];
              const InType in1_2 = pIn1[x+2];
              const InType in1_3 = pIn1[x+3];

              const InType in2_0 = pIn2[x];
              const InType in2_1 = pIn2[x+1];
              const InType in2_2 = pIn2[x+2];
              const InType in2_3 = pIn2[x+3];

              accumulator +=
                in1_0 * in2_0 +
                in1_1 * in2_1 +
                in1_2 * in2_2 +
                in1_3 * in2_3;
            }

            for(; x<in2TransposedWidth; x++) {
              accumulator += pIn1[x] * pIn2[x];
            }

            *out.Pointer(y1, y2) = accumulator;
          }
        }

        return RESULT_OK;
      } // template<typename InType, typename OutType> Result MultiplyTranspose(const Array<InType> &in1, const Array<InType> &in2Transposed, Array<OutType> &out)

      template<typename Type> Result SolveLeastSquaresWithCholesky(
        Array<Type> &A_L,       //!< Input A Matrix and Output lower-triangular L matrix
        Array<Type> &Bt_Xt,     //!< Input B-transpose matrix and Output X-transpose solution
        bool realCholesky,      //!< A real Cholesky is slower to compute, and not required if only the X solution is required
        bool &numericalFailure  //!< If true, the solver failed because of numerical instability
        )
      {
        const s32 matrixHeight = A_L.get_size(0);
        const s32 numSamples = Bt_Xt.get_size(0);

        numericalFailure = false;

        AnkiConditionalErrorAndReturnValue(A_L.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "CholeskyDecomposition", "A_L is not valid");

        AnkiConditionalErrorAndReturnValue(Bt_Xt.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "CholeskyDecomposition", "Bt_Xt is not valid");

        AnkiConditionalErrorAndReturnValue(matrixHeight == A_L.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "CholeskyDecomposition", "A_L is not square");

        AnkiConditionalErrorAndReturnValue(Bt_Xt.get_size(1) == matrixHeight,
          RESULT_FAIL_INVALID_SIZE, "CholeskyDecomposition", "Xt and Bt are the wrong sizes");

        // TODO: check if symmetric and positive-definite

        const Type minStableValue = Anki::Embedded::Flags::numeric_limits<Type>::epsilon();

        for(s32 i = 0; i < matrixHeight; i++) {
          // First, compute the non-diagonal values
          // This uses the results from the diagonal inverse computation from previous iterations of i
          Type * restrict pAL_yi = A_L.Pointer(i, 0);

          for(s32 j = 0; j < i; j++) {
            Type * restrict pAL_yj = A_L.Pointer(j, 0);

            Type sum = pAL_yi[j];
            for(s32 k = 0; k < j; k++) {
              const Type value1 = pAL_yi[k];
              const Type value2 = pAL_yj[k];
              sum -= value1*value2;
            }

            pAL_yi[j] = sum*pAL_yj[j];
          } // for(s32 j = 0; j < i; j++)

          // Second, compute the inverse of the diagonal
          {
            Type sum = pAL_yi[i];
            for(s32 k = 0; k < i; k++) {
              const Type value = pAL_yi[k];
              sum -= value*value;
            }

            if(sum < minStableValue) {
              numericalFailure = true;
              return RESULT_OK;
            }

            // TODO: change this f32 square root to f64 if Type==f64
            const Type sumRoot = static_cast<Type>(sqrtf(static_cast<f32>(sum)));
            pAL_yi[i] = static_cast<Type>(1) / sumRoot;
          }
        } // for(s32 i = 0; i < m; i++)

        // Solve L*y = b via forward substitution
        for(s32 i = 0; i < matrixHeight; i++) {
          const Type * restrict pAL_yi = A_L.Pointer(i, 0);
          //Type * restrict pBX_yi = Bt_Xt.Pointer(i, 0);

          for(s32 j = 0; j < numSamples; j++) {
            Type * restrict pBX_yj = Bt_Xt.Pointer(j, 0);

            Type sum = pBX_yj[i];
            for(s32 k = 0; k < i; k++) {
              const Type value1 = pAL_yi[k];
              const Type value2 = pBX_yj[k];
              sum -= value1*value2;
            }

            pBX_yj[i] = sum*pAL_yi[i];
          }
        }

        // Solve L'*X = Y via back substitution
        for(s32 i = matrixHeight-1; i >= 0; i--) {
          const Type * restrict pAL_yi = A_L.Pointer(i, 0);
          //Type * restrict pBX_yi = Bt_Xt.Pointer(i, 0);

          for(s32 j = 0; j < numSamples; j++) {
            Type * restrict pBX_yj = Bt_Xt.Pointer(j, 0);

            Type sum = pBX_yj[i];
            for(s32 k = matrixHeight-1; k > i; k-- ) {
              const Type value1 = A_L[k][i];
              const Type value2 = pBX_yj[k];
              sum -= value1*value2;
            }

            pBX_yj[i] = sum*pAL_yi[i];
          }
        }

        if(realCholesky) {
          // Invert the diagonal values of L, and set upper triangular to zero
          for(s32 i = 0; i < matrixHeight; i++) {
            Type * restrict pAL_yi = A_L.Pointer(i, 0);

            pAL_yi[i] = static_cast<Type>(1) / pAL_yi[i];

            for(s32 j = i+1; j < matrixHeight; j++) {
              pAL_yi[j] = 0;
            }
          }
        }

        return RESULT_OK;
      } // SolveLeastSquaresWithCholesky()

      template<typename Type> NO_INLINE Result EstimateHomography(
        const FixedLengthList<Point<Type> > &originalPoints,    //!< Four points in the original coordinate system
        const FixedLengthList<Point<Type> > &transformedPoints, //!< Four points in the transformed coordinate system
        Array<Type> &homography, //!< A 3x3 transformation matrix
        MemoryStack scratch //!< Scratch memory
        )
      {
        //BeginBenchmark("EstimateHomography_init");

        const s32 numPoints = originalPoints.get_size();

        AnkiConditionalErrorAndReturnValue(originalPoints.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "EstimateHomography", "originalPoints is not valid");

        AnkiConditionalErrorAndReturnValue(transformedPoints.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "EstimateHomography", "transformedPoints is not valid");

        AnkiConditionalErrorAndReturnValue(homography.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "EstimateHomography", "homography is not valid");

        AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "EstimateHomography", "scratch is not valid");

        AnkiConditionalErrorAndReturnValue(transformedPoints.get_size() == numPoints && numPoints >= 4,
          RESULT_FAIL_INVALID_SIZE, "EstimateHomography", "originalPoints and transformedPoints must be the same size, and have at least four points apiece.");

        AnkiConditionalErrorAndReturnValue(homography.get_size(0) == 3 && homography.get_size(1) == 3,
          RESULT_FAIL_INVALID_SIZE, "EstimateHomography", "homography must be 3x3");

        Array<Type> A(8, 2*numPoints, scratch);
        Array<Type> bt(1, 2*numPoints, scratch);

        const Point<Type> * const pOriginalPoints = originalPoints.Pointer(0);
        const Point<Type> * const pTransformedPoints = transformedPoints.Pointer(0);

        Type * restrict pBt = bt.Pointer(0,0);

        //EndBenchmark("EstimateHomography_init");

        //BeginBenchmark("EstimateHomography_a&b");

        for(s32 i=0; i<numPoints; i++) {
          Type * restrict A_y1 = A.Pointer(2*i, 0);
          Type * restrict A_y2 = A.Pointer(2*i + 1, 0);

          const Type xi = pOriginalPoints[i].x;
          const Type yi = pOriginalPoints[i].y;

          const Type xp = pTransformedPoints[i].x;
          const Type yp = pTransformedPoints[i].y;

          A_y1[0] = 0;  A_y1[1] = 0;  A_y1[2] = 0; A_y1[3] = -xi; A_y1[4] = -yi; A_y1[5] = -1; A_y1[6] = xi*yp;  A_y1[7] = yi*yp;
          A_y2[0] = xi; A_y2[1] = yi; A_y2[2] = 1; A_y2[3] = 0;   A_y2[4] = 0;   A_y2[5] = 0;  A_y2[6] = -xi*xp; A_y2[7] = -yi*xp;

          pBt[2*i] = -yp;
          pBt[2*i + 1] = xp;
        }

        //EndBenchmark("EstimateHomography_a&b");

        //BeginBenchmark("EstimateHomography_At");

        Array<Type> At(2*numPoints, 8, scratch);

        Matrix::Transpose(A, At);

        //EndBenchmark("EstimateHomography_At");

        //BeginBenchmark("EstimateHomography_AtA");

        Array<Type> AtA(8, 8, scratch, Flags::Buffer(false,false,false));
        Array<Type> Atb(8, 1, scratch, Flags::Buffer(false,false,false));

        Matrix::Multiply(At, A, AtA);

        //EndBenchmark("EstimateHomography_AtA");

        //BeginBenchmark("EstimateHomography_Atb");

        Matrix::MultiplyTranspose(At, bt, Atb);

        //EndBenchmark("EstimateHomography_Atb");

        //BeginBenchmark("EstimateHomography_transposeAtb");

        Array<Type> Atbt(1, 8, scratch);

        Matrix::Transpose(Atb, Atbt);

        //EndBenchmark("EstimateHomography_transposeAtb");

        //BeginBenchmark("EstimateHomography_cholesky");

        bool numericalFailure;

        const Result choleskyResult = SolveLeastSquaresWithCholesky(AtA, Atbt, false, numericalFailure);

        AnkiConditionalErrorAndReturnValue(choleskyResult == RESULT_OK,
          choleskyResult, "EstimateHomography", "SolveLeastSquaresWithCholesky failed");

        if(numericalFailure){
          AnkiWarn("EstimateHomography", "numericalFailure");
          return RESULT_OK;
        }

        Type * restrict pAtbt = Atbt.Pointer(0,0);

        homography[0][0] = pAtbt[0]; homography[0][1] = pAtbt[1]; homography[0][2] = pAtbt[2];
        homography[1][0] = pAtbt[3]; homography[1][1] = pAtbt[4]; homography[1][2] = pAtbt[5];
        homography[2][0] = pAtbt[6]; homography[2][1] = pAtbt[7]; homography[2][2] = static_cast<Type>(1);

        //EndBenchmark("EstimateHomography_cholesky");

        return RESULT_OK;
      } // EstimateHomography()

      template<typename InType, typename OutType> Result Reshape(const bool isColumnMajor, const Array<InType> &in, Array<OutType> &out)
      {
        const s32 inHeight = in.get_size(0);
        const s32 inWidth = in.get_size(1);

        const s32 outHeight = out.get_size(0);
        const s32 outWidth = out.get_size(1);

        AnkiConditionalErrorAndReturnValue((inHeight*inWidth) == (outHeight*outWidth),
          RESULT_FAIL_INVALID_SIZE, "Reshape", "Input and Output matrices are incompatible sizes");

        s32 inIndexY = 0;
        s32 inIndexX = 0;

        if(isColumnMajor) {
          for(s32 y = 0; y < outHeight; y++)
          {
            OutType * const pOut = out.Pointer(y,0);

            for(s32 x = 0; x < outWidth; x++) {
              const InType curIn = *in.Pointer(inIndexY,inIndexX);

              pOut[x] = static_cast<OutType>(curIn);

              inIndexY++;
              if(inIndexY >= inHeight) {
                inIndexY = 0;
                inIndexX++;
              }
            }
          }
        } else { // if(isColumnMajor)
          for(s32 y = 0; y < outHeight; y++)
          {
            OutType * const pOut = out.Pointer(y,0);

            for(s32 x = 0; x < outWidth; x++) {
              const InType curIn = *in.Pointer(inIndexY,inIndexX);

              pOut[x] = static_cast<OutType>(curIn);

              inIndexX++;
              if(inIndexX >= inWidth) {
                inIndexX = 0;
                inIndexY++;
              }
            }
          }
        } // if(isColumnMajor) ... else

        return RESULT_OK;
      } // Reshape()

      template<typename InType, typename OutType> Array<OutType> Reshape(const bool isColumnMajor, const Array<InType> &in, const s32 newHeight, const s32 newWidth, MemoryStack &memory)
      {
        Array<OutType> out(newHeight, newWidth, memory);

        Reshape<InType, OutType>(isColumnMajor, in, out);

        return out;
      }

      template<typename InType, typename OutType> Result Vectorize(const bool isColumnMajor, const Array<InType> &in, Array<OutType> &out)
      {
        AnkiConditionalErrorAndReturnValue(out.get_size(0) == 1,
          RESULT_FAIL_INVALID_SIZE, "Vectorize", "Output is not 1xN");

        return Reshape<InType, OutType>(isColumnMajor, in, out);
      }

      template<typename InType, typename OutType> Array<OutType> Vectorize(const bool isColumnMajor, const Array<InType> &in, MemoryStack &memory)
      {
        const s32 inHeight = in.get_size(0);
        const s32 inWidth = in.get_size(1);

        Array<OutType> out(1, inHeight*inWidth, memory);

        Vectorize<InType, OutType>(isColumnMajor, in, out);

        return out;
      }

      template<typename InType, typename OutType> Result Transpose(const Array<InType> &in, Array<OutType> &out)
      {
        const s32 inHeight = in.get_size(0);
        const s32 inWidth = in.get_size(1);

        const s32 outStride = out.get_stride();

        AnkiConditionalErrorAndReturnValue(in.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "Transpose", "in is not valid");

        AnkiConditionalErrorAndReturnValue(out.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "Transpose", "out is not valid");

        AnkiConditionalErrorAndReturnValue(out.get_size(0) == inWidth && out.get_size(1) == inHeight,
          RESULT_FAIL_INVALID_SIZE, "Transpose", "out is not the correct size");

        AnkiConditionalErrorAndReturnValue(in.get_rawDataPointer() != out.get_rawDataPointer(),
          RESULT_FAIL_ALIASED_MEMORY, "Transpose", "in and out cannot be the same array");

        for(s32 yIn=0; yIn<inHeight; yIn++) {
          const InType * restrict pIn = in.Pointer(yIn, 0);
          u8 * restrict pOut = reinterpret_cast<u8*>(out.Pointer(0,yIn));

          s32 xIn;
          s32 outOffset0 = 0;

          for(xIn=0; xIn<inWidth-1; xIn+=2) {
            const InType in0 = pIn[xIn];
            const InType in1 = pIn[xIn+1];

            const s32 outOffset1 = outOffset0 + outStride;

            *reinterpret_cast<OutType*>(pOut + outOffset0) = static_cast<OutType>(in0);
            *reinterpret_cast<OutType*>(pOut + outOffset1) = static_cast<OutType>(in1);

            outOffset0 += 2*outStride;
          }

          for(; xIn<inWidth; xIn++) {
            *out.Pointer(xIn,yIn) = static_cast<OutType>(pIn[xIn]);
          }
        }

        return RESULT_OK;
      } // Transpose()

      template<typename InType, typename OutType> Result Rotate90(const Array<InType> &in, Array<OutType> &out)
      {
        const s32 arrWidth = in.get_size(1);

        AnkiConditionalErrorAndReturnValue(in.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "Rotate90", "in is not valid");

        AnkiConditionalErrorAndReturnValue(out.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "Rotate90", "out is not valid");

        AnkiConditionalErrorAndReturnValue(in.get_size(0) == in.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "Rotate90", "in and out must be square");

        AnkiConditionalErrorAndReturnValue(out.get_size(0) == arrWidth && out.get_size(1) == arrWidth,
          RESULT_FAIL_INVALID_SIZE, "Rotate90", "in and out must be square");

        AnkiConditionalErrorAndReturnValue(in.get_rawDataPointer() != out.get_rawDataPointer(),
          RESULT_FAIL_ALIASED_MEMORY, "Rotate90", "in and out cannot be the same array");

        const s32 outStride = out.get_stride();

        for(s32 yIn=0; yIn<arrWidth; yIn++) {
          const InType * restrict pIn = in.Pointer(yIn, 0);
          u8 * restrict pOut = reinterpret_cast<u8*>(out.Pointer(0, arrWidth-yIn-1));

          for(s32 xIn=0; xIn<arrWidth; xIn++) {
            (reinterpret_cast<OutType *>(pOut))[0] = static_cast<OutType>(pIn[xIn]);

            pOut += outStride;
          }
        }

        return RESULT_OK;
      } // Rotate90()

      template<typename InType, typename OutType> Result Rotate180(const Array<InType> &in, Array<OutType> &out)
      {
        const s32 arrWidth = in.get_size(1);

        AnkiConditionalErrorAndReturnValue(in.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "Rotate180", "in is not valid");

        AnkiConditionalErrorAndReturnValue(out.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "Rotate180", "out is not valid");

        AnkiConditionalErrorAndReturnValue(in.get_size(0) == in.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "Rotate180", "in and out must be square");

        AnkiConditionalErrorAndReturnValue(out.get_size(0) == arrWidth && out.get_size(1) == arrWidth,
          RESULT_FAIL_INVALID_SIZE, "Rotate180", "in and out must be square");

        AnkiConditionalErrorAndReturnValue(in.get_rawDataPointer() != out.get_rawDataPointer(),
          RESULT_FAIL_ALIASED_MEMORY, "Rotate180", "in and out cannot be the same array");

        for(s32 yIn=0; yIn<arrWidth; yIn++) {
          const InType * restrict pIn = in.Pointer(yIn, 0);
          OutType * restrict pOut = out.Pointer(arrWidth-yIn-1, 0);

          for(s32 xIn=0; xIn<arrWidth; xIn++) {
            pOut[arrWidth-xIn-1] = static_cast<OutType>(pIn[xIn]);
          }
        }

        return RESULT_OK;
      } // Rotate180()

      template<typename InType, typename OutType> Result Rotate270(const Array<InType> &in, Array<OutType> &out)
      {
        const s32 arrWidth = in.get_size(1);

        AnkiConditionalErrorAndReturnValue(in.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "Rotate270", "in is not valid");

        AnkiConditionalErrorAndReturnValue(out.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "Rotate270", "out is not valid");

        AnkiConditionalErrorAndReturnValue(in.get_size(0) == in.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "Rotate270", "in and out must be square");

        AnkiConditionalErrorAndReturnValue(out.get_size(0) == arrWidth && out.get_size(1) == arrWidth,
          RESULT_FAIL_INVALID_SIZE, "Rotate270", "in and out must be square");

        AnkiConditionalErrorAndReturnValue(in.get_rawDataPointer() != out.get_rawDataPointer(),
          RESULT_FAIL_ALIASED_MEMORY, "Rotate270", "in and out cannot be the same array");

        const s32 outStride = out.get_stride();

        for(s32 yIn=0; yIn<arrWidth; yIn++) {
          const InType * restrict pIn = in.Pointer(yIn, 0);
          u8 * restrict pOut = reinterpret_cast<u8*>(out.Pointer(arrWidth-1, yIn));

          for(s32 xIn=0; xIn<arrWidth; xIn++) {
            (reinterpret_cast<OutType *>(pOut))[0] = static_cast<OutType>(pIn[xIn]);

            pOut -= outStride;
          }
        }

        return RESULT_OK;
      } // Rotate270()

      template<typename Type> void InsertionSort_sortAscendingDimension0(Array<Type> &arr, const s32 trueMinIndex, const s32 trueMaxIndex)
      {
        const s32 arrWidth = arr.get_size(1);

        for(s32 x=0; x<arrWidth; x++) {
          for(s32 y=(trueMinIndex+1); y<=trueMaxIndex; y++) {
            const Type valueToInsert = arr[y][x];

            s32 holePosition = y;

            while(holePosition > trueMinIndex && valueToInsert < arr[holePosition-1][x]) {
              arr[holePosition][x] = arr[holePosition-1][x];
              holePosition--;
            }

            arr[holePosition][x] = valueToInsert;
          }
        } // for(s32 x=0; x<arrWidth; x++)
      } // InsertionSort_sortAscendingDimension0()

      template<typename Type> void InsertionSort_sortDescendingDimension0(Array<Type> &arr, const s32 trueMinIndex, const s32 trueMaxIndex)
      {
        const s32 arrWidth = arr.get_size(1);

        for(s32 x=0; x<arrWidth; x++) {
          for(s32 y=(trueMinIndex+1); y<=trueMaxIndex; y++) {
            const Type valueToInsert = arr[y][x];

            s32 holePosition = y;

            while(holePosition > trueMinIndex && valueToInsert > arr[holePosition-1][x]) {
              arr[holePosition][x] = arr[holePosition-1][x];
              holePosition--;
            }

            arr[holePosition][x] = valueToInsert;
          }
        } // for(s32 x=0; x<arrWidth; x++)
      } // InsertionSort_sortDescendingDimension0

      template<typename Type> void InsertionSort_sortAscendingDimension1(Array<Type> &arr, const s32 trueMinIndex, const s32 trueMaxIndex)
      {
        const s32 arrHeight = arr.get_size(0);

        for(s32 y=0; y<arrHeight; y++) {
          Type * const pArr = arr[y];

          for(s32 x=(trueMinIndex+1); x<=trueMaxIndex; x++) {
            const Type valueToInsert = pArr[x];

            s32 holePosition = x;

            while(holePosition > trueMinIndex && valueToInsert < pArr[holePosition-1]) {
              pArr[holePosition] = pArr[holePosition-1];
              holePosition--;
            }

            pArr[holePosition] = valueToInsert;
          }
        } // for(s32 x=0; x<arrWidth; x++)
      } // InsertionSort_sortAscendingDimension1()

      template<typename Type> void InsertionSort_sortDescendingDimension1(Array<Type> &arr, const s32 trueMinIndex, const s32 trueMaxIndex)
      {
        const s32 arrHeight = arr.get_size(0);

        for(s32 y=0; y<arrHeight; y++) {
          Type * const pArr = arr[y];

          for(s32 x=(trueMinIndex+1); x<=trueMaxIndex; x++) {
            const Type valueToInsert = pArr[x];

            s32 holePosition = x;

            while(holePosition > trueMinIndex && valueToInsert > pArr[holePosition-1]) {
              pArr[holePosition] = pArr[holePosition-1];
              holePosition--;
            }

            pArr[holePosition] = valueToInsert;
          }
        } // for(s32 x=0; x<arrWidth; x++)
      } // InsertionSort_sortAscendingDimension1()

      template<typename Type> void InsertionSort_sortAscendingDimension0(Array<Type> &arr, Array<s32> &indexes, const s32 trueMinIndex, const s32 trueMaxIndex)
      {
        const s32 arrWidth = arr.get_size(1);

        for(s32 x=0; x<arrWidth; x++) {
          indexes[0][x] = 0;

          for(s32 y=(trueMinIndex+1); y<=trueMaxIndex; y++) {
            const Type valueToInsert = arr[y][x];

            s32 holePosition = y;

            while(holePosition > trueMinIndex && valueToInsert < arr[holePosition-1][x]) {
              arr[holePosition][x] = arr[holePosition-1][x];
              indexes[holePosition][x] = indexes[holePosition-1][x];
              holePosition--;
            }

            arr[holePosition][x] = valueToInsert;
            indexes[holePosition][x] = y;
          }
        } // for(s32 x=0; x<arrWidth; x++)
      } // InsertionSort_sortAscendingDimension0()

      template<typename Type> void InsertionSort_sortDescendingDimension0(Array<Type> &arr, Array<s32> &indexes, const s32 trueMinIndex, const s32 trueMaxIndex)
      {
        const s32 arrWidth = arr.get_size(1);

        for(s32 x=0; x<arrWidth; x++) {
          indexes[0][x] = 0;

          for(s32 y=(trueMinIndex+1); y<=trueMaxIndex; y++) {
            const Type valueToInsert = arr[y][x];

            s32 holePosition = y;

            while(holePosition > trueMinIndex && valueToInsert > arr[holePosition-1][x]) {
              arr[holePosition][x] = arr[holePosition-1][x];
              indexes[holePosition][x] = indexes[holePosition-1][x];
              holePosition--;
            }

            arr[holePosition][x] = valueToInsert;
            indexes[holePosition][x] = y;
          }
        } // for(s32 x=0; x<arrWidth; x++)
      } // InsertionSort_sortDescendingDimension0()

      template<typename Type> void InsertionSort_sortAscendingDimension1(Array<Type> &arr, Array<s32> &indexes, const s32 trueMinIndex, const s32 trueMaxIndex)
      {
        const s32 arrHeight = arr.get_size(0);

        for(s32 y=0; y<arrHeight; y++) {
          Type * const pArr = arr[y];
          s32 * const pIndexes = indexes[y];

          pIndexes[0] = 0;

          for(s32 x=(trueMinIndex+1); x<=trueMaxIndex; x++) {
            const Type valueToInsert = pArr[x];

            s32 holePosition = x;

            while(holePosition > trueMinIndex && valueToInsert < pArr[holePosition-1]) {
              pArr[holePosition] = pArr[holePosition-1];
              pIndexes[holePosition] = pIndexes[holePosition-1];
              holePosition--;
            }

            pArr[holePosition] = valueToInsert;
            pIndexes[holePosition] = x;
          }
        } // for(s32 x=0; x<arrWidth; x++)
      } // InsertionSort_sortAscendingDimension1()

      template<typename Type> void InsertionSort_sortDescendingDimension1(Array<Type> &arr, Array<s32> &indexes, const s32 trueMinIndex, const s32 trueMaxIndex)
      {
        const s32 arrHeight = arr.get_size(0);

        for(s32 y=0; y<arrHeight; y++) {
          Type * const pArr = arr[y];
          s32 * const pIndexes = indexes[y];

          pIndexes[0] = 0;

          for(s32 x=(trueMinIndex+1); x<=trueMaxIndex; x++) {
            const Type valueToInsert = pArr[x];

            s32 holePosition = x;

            while(holePosition > trueMinIndex && valueToInsert > pArr[holePosition-1]) {
              pArr[holePosition] = pArr[holePosition-1];
              pIndexes[holePosition] = pIndexes[holePosition-1];
              holePosition--;
            }

            pArr[holePosition] = valueToInsert;
            pIndexes[holePosition] = x;
          }
        } // for(s32 x=0; x<arrWidth; x++)
      } // InsertionSort_sortDescendingDimension1()

      template<typename Type> Result InsertionSort(Array<Type> &arr, const s32 sortWhichDimension, const bool sortAscending, const s32 minIndex, const s32 maxIndex)
      {
        const s32 arrHeight = arr.get_size(0);
        const s32 arrWidth = arr.get_size(1);

        AnkiConditionalErrorAndReturnValue(arr.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "Sort", "Input array is invalid");

        AnkiConditionalErrorAndReturnValue(sortWhichDimension==0 || sortWhichDimension==1,
          RESULT_FAIL_INVALID_PARAMETER, "Sort", "sortWhichDimension must be zero or one");

        const s32 trueMinIndex = CLIP(minIndex, 0, arr.get_size(sortWhichDimension) - 1);
        const s32 trueMaxIndex = CLIP(maxIndex, 0, arr.get_size(sortWhichDimension) - 1);

        if(sortWhichDimension == 0) {
          // TODO: This columnwise sorting could be sped up, with smarter array indexing.
          if(sortAscending) {
            InsertionSort_sortAscendingDimension0(arr, trueMinIndex, trueMaxIndex);
          } else { // if(sortAscending)
            InsertionSort_sortDescendingDimension0(arr, trueMinIndex, trueMaxIndex);
          } // if(sortAscending) ... else
        } else { // sortWhichDimension == 1
          if(sortAscending) {
            InsertionSort_sortAscendingDimension1(arr, trueMinIndex, trueMaxIndex);
          } else { // if(sortAscending)
            InsertionSort_sortDescendingDimension1(arr, trueMinIndex, trueMaxIndex);
          } // if(sortAscending) ... else
        } // if(sortWhichDimension == 0) ... else

        return RESULT_OK;
      } // InsertionSort()

      template<typename Type> Result InsertionSort(Array<Type> &arr, Array<s32> &indexes, const s32 sortWhichDimension, const bool sortAscending, const s32 minIndex, const s32 maxIndex)
      {
        const s32 arrHeight = arr.get_size(0);
        const s32 arrWidth = arr.get_size(1);

        AnkiConditionalErrorAndReturnValue(arr.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "Sort", "Input array is invalid");

        AnkiConditionalErrorAndReturnValue(indexes.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "Sort", "indexes array is invalid");

        AnkiConditionalErrorAndReturnValue(sortWhichDimension==0 || sortWhichDimension==1,
          RESULT_FAIL_INVALID_PARAMETER, "Sort", "sortWhichDimension must be zero or one");

        AnkiConditionalErrorAndReturnValue(indexes.get_size(0) == arrHeight && indexes.get_size(1) == arrWidth,
          RESULT_FAIL_INVALID_SIZE, "Sort", "indexes must be the same size as arr");

        const s32 trueMinIndex = CLIP(minIndex, 0, arr.get_size(sortWhichDimension) - 1);
        const s32 trueMaxIndex = CLIP(maxIndex, 0, arr.get_size(sortWhichDimension) - 1);

        if(sortWhichDimension == 0) {
          // TODO: This columnwise sorting could be sped up, with smarter array indexing.
          if(sortAscending) {
            InsertionSort_sortAscendingDimension0(arr, indexes, trueMinIndex, trueMaxIndex);
          } else { // if(sortAscending)
            InsertionSort_sortDescendingDimension0(arr, indexes, trueMinIndex, trueMaxIndex);
          } // if(sortAscending) ... else
        } else { // sortWhichDimension == 1
          if(sortAscending) {
            InsertionSort_sortAscendingDimension1(arr, indexes, trueMinIndex, trueMaxIndex);
          } else { // if(sortAscending)
            InsertionSort_sortDescendingDimension1(arr, indexes, trueMinIndex, trueMaxIndex);
          } // if(sortAscending) ... else
        } // if(sortWhichDimension == 0) ... else

        return RESULT_OK;
      } // InsertionSort()

      template<typename Type> Result MakeSymmetric(Type &arr, bool lowerToUpper)
      {
        AnkiConditionalErrorAndReturnValue(arr.get_size(0) == arr.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "MakeSymmetric", "Input array must be square");

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

      namespace Elementwise
      {
        template<typename InType, typename Operator, typename OutType> Result ApplyOperation(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
        {
          const Array<InType> &in1Array = in1.get_array();
          const Array<InType> &in2Array = in2.get_array();
          Array<OutType> &out1Array = out.get_array();

          AnkiConditionalErrorAndReturnValue(in1Array.IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "Matrix::Elementwise::ApplyOperation", "Invalid array in1");

          AnkiConditionalErrorAndReturnValue(in2Array.IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "Matrix::Elementwise::ApplyOperation", "Invalid array in2");

          AnkiConditionalErrorAndReturnValue(out1Array.IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "Matrix::Elementwise::ApplyOperation", "Invalid array out");

          ArraySliceLimits_in2_out1<s32> limits(
            in1.get_ySlice(), in1.get_xSlice(), in1.get_isTransposed(),
            in2.get_ySlice(), in2.get_xSlice(), in2.get_isTransposed(),
            out.get_ySlice(), out.get_xSlice());

          AnkiConditionalErrorAndReturnValue(limits.isValid,
            RESULT_FAIL_INVALID_OBJECT, "Matrix::Elementwise::ApplyOperation", "Limits is not valid");

          if(limits.isSimpleIteration) {
            // If the input isn't transposed, we will do the maximally efficient loop iteration

            for(s32 y=0; y<limits.ySize; y++) {
              const InType * const pIn1 = in1Array.Pointer(limits.in1Y, 0);
              const InType * const pIn2 = in2Array.Pointer(limits.in2Y, 0);
              OutType * const pOut1 = out1Array.Pointer(limits.out1Y, 0);

              limits.OuterIncrementTop();

              for(s32 x=0; x<limits.xSize; x++) {
                pOut1[limits.out1X] = Operator::BinaryElementwiseOperation(pIn1[limits.in1X], pIn2[limits.in2X]);

                limits.in1X += limits.in1_xInnerIncrement;
                limits.in2X += limits.in2_xInnerIncrement;
                limits.out1X += limits.out1_xInnerIncrement;
              }

              limits.OuterIncrementBottom();
            }
          } else { // if(limits.isSimpleIteration)
            // If either input is transposed is allowed, then we will do an inefficent loop iteration

            for(s32 y=0; y<limits.ySize; y++) {
              OutType * const pOut1 = out1Array.Pointer(limits.out1Y, 0);

              limits.OuterIncrementTop();

              for(s32 x=0; x<limits.xSize; x++) {
                const InType valIn1 = *in1Array.Pointer(limits.in1Y, limits.in1X);
                const InType valIn2 = *in2Array.Pointer(limits.in2Y, limits.in2X);

                pOut1[limits.out1X] = Operator::BinaryElementwiseOperation(valIn1, valIn2);

                limits.in1X += limits.in1_xInnerIncrement;
                limits.in1Y += limits.in1_yInnerIncrement;
                limits.in2X += limits.in2_xInnerIncrement;
                limits.in2Y += limits.in2_yInnerIncrement;
                limits.out1X += limits.out1_xInnerIncrement;
              }

              limits.OuterIncrementBottom();
            }
          } //   if(limits.isSimpleIteration)  ... else

          return RESULT_OK;
        } // template<typename InType, typename Operator, typename OutType> Result ApplyOperation(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)

        template<typename InType, typename Operator, typename OutType> Result ApplyOperation(const ConstArraySliceExpression<InType> &in1, const InType value2, ArraySlice<OutType> out)
        {
          const Array<InType> &in1Array = in1.get_array();
          Array<OutType> &out1Array = out.get_array();

          AnkiConditionalErrorAndReturnValue(in1Array.IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "Matrix::Elementwise::ApplyOperation", "Invalid array in1");

          AnkiConditionalErrorAndReturnValue(out1Array.IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "Matrix::Elementwise::ApplyOperation", "Invalid array out");

          ArraySliceLimits_in1_out1<s32> limits(
            in1.get_ySlice(), in1.get_xSlice(), in1.get_isTransposed(),
            out.get_ySlice(), out.get_xSlice());

          AnkiConditionalErrorAndReturnValue(limits.isValid,
            RESULT_FAIL_INVALID_OBJECT, "Matrix::Elementwise::ApplyOperation", "Limits is not valid");

          if(limits.isSimpleIteration) {
            // If the input isn't transposed, we will do the maximally efficient loop iteration

            for(s32 y=0; y<limits.ySize; y++) {
              const InType * const pIn1 = in1Array.Pointer(limits.in1Y, 0);
              OutType * const pOut1 = out1Array.Pointer(limits.out1Y, 0);

              limits.OuterIncrementTop();

              for(s32 x=0; x<limits.xSize; x++) {
                pOut1[limits.out1X] = Operator::BinaryElementwiseOperation(pIn1[limits.in1X], value2);

                limits.in1X += limits.in1_xInnerIncrement;
                limits.out1X += limits.out1_xInnerIncrement;
              }

              limits.OuterIncrementBottom();
            }
          } else { // if(limits.isSimpleIteration)
            // If either input is transposed is allowed, then we will do an inefficent loop iteration

            for(s32 y=0; y<limits.ySize; y++) {
              OutType * const pOut1 = out1Array.Pointer(limits.out1Y, 0);

              limits.OuterIncrementTop();

              for(s32 x=0; x<limits.xSize; x++) {
                const InType valIn1 = *in1Array.Pointer(limits.in1Y, limits.in1X);

                pOut1[limits.out1X] = Operator::BinaryElementwiseOperation(valIn1, value2);

                limits.in1X += limits.in1_xInnerIncrement;
                limits.in1Y += limits.in1_yInnerIncrement;
                limits.out1X += limits.out1_xInnerIncrement;
              }

              limits.OuterIncrementBottom();
            }
          } //   if(limits.isSimpleIteration)  ... else

          return RESULT_OK;
        } // template<typename InType, typename Operator, typename OutType> Result ApplyOperation(const ConstArraySliceExpression<InType> &in1, const InType value2, ArraySlice<OutType> out)

        template<typename InType, typename Operator, typename OutType> Result ApplyOperation(const InType value1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
        {
          const Array<InType> &in2Array = in2.get_array();
          Array<OutType> &out1Array = out.get_array();

          AnkiConditionalErrorAndReturnValue(in2Array.IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "Matrix::Elementwise::ApplyOperation", "Invalid array in2");

          AnkiConditionalErrorAndReturnValue(out1Array.IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "Matrix::Elementwise::ApplyOperation", "Invalid array out");

          ArraySliceLimits_in1_out1<s32> limits(
            in2.get_ySlice(), in2.get_xSlice(), in2.get_isTransposed(),
            out.get_ySlice(), out.get_xSlice());

          AnkiConditionalErrorAndReturnValue(limits.isValid,
            RESULT_FAIL_INVALID_OBJECT, "Matrix::Elementwise::ApplyOperation", "Limits is not valid");

          if(limits.isSimpleIteration) {
            // If the input isn't transposed, we will do the maximally efficient loop iteration

            for(s32 y=0; y<limits.ySize; y++) {
              const InType * const pIn2 = in2Array.Pointer(limits.in1Y, 0);
              OutType * const pOut1 = out1Array.Pointer(limits.out1Y, 0);

              limits.OuterIncrementTop();

              for(s32 x=0; x<limits.xSize; x++) {
                pOut1[limits.out1X] = Operator::BinaryElementwiseOperation(value1, pIn2[limits.in1X]);

                limits.in1X += limits.in1_xInnerIncrement;
                limits.out1X += limits.out1_xInnerIncrement;
              }

              limits.OuterIncrementBottom();
            }
          } else { // if(limits.isSimpleIteration)
            // If either input is transposed, then we will do an inefficent loop iteration

            for(s32 y=0; y<limits.ySize; y++) {
              OutType * const pOut1 = out1Array.Pointer(limits.out1Y, 0);

              limits.OuterIncrementTop();

              for(s32 x=0; x<limits.xSize; x++) {
                const InType valIn2 = *in2Array.Pointer(limits.in1Y, limits.in1X);

                pOut1[limits.out1X] = Operator::BinaryElementwiseOperation(value1, valIn2);

                limits.in1X += limits.in1_xInnerIncrement;
                limits.in1Y += limits.in1_yInnerIncrement;
                limits.out1X += limits.out1_xInnerIncrement;
              }

              limits.OuterIncrementBottom();
            }
          } //   if(limits.isSimpleIteration)  ... else

          return RESULT_OK;
        } // template<typename InType, typename Operator, typename OutType> Result ApplyOperation(const InType value1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out)
      } // namespace Elementwise
    } // namespace Matrix
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_
