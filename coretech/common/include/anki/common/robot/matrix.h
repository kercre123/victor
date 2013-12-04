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

namespace Anki
{
  namespace Embedded
  {
    namespace Matrix
    {
#pragma mark --- Definitions ---

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
      }

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
      }

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

      template<typename InType, typename OutType> Result MultiplyTranspose(const Array<InType> &in1, const Array<InType> &in2Transposed, Array<OutType> &out)
      {
        const s32 in1Height = in1.get_size(0);
        const s32 in1Width = in1.get_size(1);

        const s32 in2TransposedHeight = in2Transposed.get_size(0);
        const s32 in2TransposedWidth = in2Transposed.get_size(1);

        AnkiConditionalErrorAndReturnValue(in1Width == in2TransposedWidth,
          RESULT_FAIL, "Multiply", "Input matrices are incompatible sizes");

        AnkiConditionalErrorAndReturnValue(out.get_size(0) == in1Height,
          RESULT_FAIL, "Multiply", "Input and Output matrices are incompatible sizes");

        AnkiConditionalErrorAndReturnValue(out.get_size(1) == in2TransposedHeight,
          RESULT_FAIL, "Multiply", "Input and Output matrices are incompatible sizes");

        for(s32 y1=0; y1<in1Height; y1++)
        {
          const InType * restrict pMat1 = in1.Pointer(y1, 0);

          for(s32 y2=0; y2<in2TransposedHeight; y2++) {
            const InType * restrict pMat2 = in2Transposed.Pointer(y2, 0);
            OutType * restrict pMatOut = out.Pointer(y1, y2);

            *pMatOut = 0;

            for(s32 x=0; x<in2TransposedWidth; x++) {
              *pMatOut += pMat1[x] * pMat2[x];
            }
          }
        }

        return RESULT_OK;
      } // template<typename InType, typename OutType> Result MultiplyTranspose(const Array<InType> &in1, const Array<InType> &in2Transposed, Array<OutType> &out)

      template<typename TypeIn, typename TypeOut> Result Reshape(const bool isColumnMajor, const Array<TypeIn> &in, Array<TypeOut> &out)
      {
        const s32 inHeight = in.get_size(0);
        const s32 inWidth = in.get_size(1);

        const s32 outHeight = out.get_size(0);
        const s32 outWidth = out.get_size(1);

        AnkiConditionalErrorAndReturnValue((inHeight*inWidth) == (outHeight*outWidth),
          RESULT_FAIL, "Reshape", "Input and Output matrices are incompatible sizes");

        s32 inIndexY = 0;
        s32 inIndexX = 0;

        if(isColumnMajor) {
          for(s32 y = 0; y < outHeight; y++)
          {
            TypeOut * const pOut = out.Pointer(y,0);

            for(s32 x = 0; x < outWidth; x++) {
              const TypeIn curIn = *in.Pointer(inIndexY,inIndexX);

              pOut[x] = static_cast<TypeOut>(curIn);

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
            TypeOut * const pOut = out.Pointer(y,0);

            for(s32 x = 0; x < outWidth; x++) {
              const TypeIn curIn = *in.Pointer(inIndexY,inIndexX);

              pOut[x] = static_cast<TypeOut>(curIn);

              inIndexX++;
              if(inIndexX >= inWidth) {
                inIndexX = 0;
                inIndexY++;
              }
            }
          }
        } // if(isColumnMajor) ... else

        return RESULT_OK;
      }

      template<typename TypeIn, typename TypeOut> Array<TypeOut> Reshape(const bool isColumnMajor, const Array<TypeIn> &in, const s32 newHeight, const s32 newWidth, MemoryStack &memory)
      {
        Array<TypeOut> out(newHeight, newWidth, memory);

        Reshape<TypeIn, TypeOut>(isColumnMajor, in, out);

        return out;
      }

      template<typename TypeIn, typename TypeOut> Result Vectorize(const bool isColumnMajor, const Array<TypeIn> &in, Array<TypeOut> &out)
      {
        AnkiConditionalErrorAndReturnValue(out.get_size(0) == 1,
          RESULT_FAIL, "Vectorize", "Output is not 1xN");

        return Reshape<TypeIn, TypeOut>(isColumnMajor, in, out);
      }

      template<typename TypeIn, typename TypeOut> Array<TypeOut> Vectorize(const bool isColumnMajor, const Array<TypeIn> &in, MemoryStack &memory)
      {
        const s32 inHeight = in.get_size(0);
        const s32 inWidth = in.get_size(1);

        Array<TypeOut> out(1, inHeight*inWidth, memory);

        Vectorize<TypeIn, TypeOut>(isColumnMajor, in, out);

        return out;
      }

      template<typename Type> Result Sort(Array<Type> &arr, const s32 sortWhichDimension, const bool sortAscending)
      {
        const s32 arrHeight = arr.get_size(0);
        const s32 arrWidth = arr.get_size(1);

        AnkiConditionalErrorAndReturnValue(arr.IsValid(),
          RESULT_FAIL, "Sort", "Input array is invalid");

        AnkiConditionalErrorAndReturnValue(sortWhichDimension==0 || sortWhichDimension==1,
          RESULT_FAIL, "Sort", "sortWhichDimension must be zero or one");

        if(sortWhichDimension == 0) {
          // TODO: This columnwise sorting could be sped up, with smarter array indexing.
          if(sortAscending) {
            for(s32 x=0; x<arrWidth; x++) {
              for(s32 y=1; y<arrHeight; y++) {
                const Type valueToInsert = arr[y][x];

                s32 holePosition = y;

                while(holePosition > 0 && valueToInsert < arr[holePosition-1][x]) {
                  arr[holePosition][x] = arr[holePosition-1][x];
                  holePosition--;
                }

                arr[holePosition][x] = valueToInsert;
              }
            } // for(s32 x=0; x<arrWidth; x++)
          } else { // if(sortAscending)
            for(s32 x=0; x<arrWidth; x++) {
              for(s32 y=1; y<arrHeight; y++) {
                const Type valueToInsert = arr[y][x];

                s32 holePosition = y;

                while(holePosition > 0 && valueToInsert > arr[holePosition-1][x]) {
                  arr[holePosition][x] = arr[holePosition-1][x];
                  holePosition--;
                }

                arr[holePosition][x] = valueToInsert;
              }
            } // for(s32 x=0; x<arrWidth; x++)
          } // if(sortAscending) ... else
        } else { // sortWhichDimension == 1
          if(sortAscending) {
            for(s32 y=0; y<arrHeight; y++) {
              Type * const pArr = arr[y];

              for(s32 x=1; x<arrWidth; x++) {
                const Type valueToInsert = pArr[x];

                s32 holePosition = x;

                while(holePosition > 0 && valueToInsert < pArr[holePosition-1]) {
                  pArr[holePosition] = pArr[holePosition-1];
                  holePosition--;
                }

                pArr[holePosition] = valueToInsert;
              }
            } // for(s32 x=0; x<arrWidth; x++)
          } else { // if(sortAscending)
            for(s32 y=0; y<arrHeight; y++) {
              Type * const pArr = arr[y];

              for(s32 x=1; x<arrWidth; x++) {
                const Type valueToInsert = pArr[x];

                s32 holePosition = x;

                while(holePosition > 0 && valueToInsert > pArr[holePosition-1]) {
                  pArr[holePosition] = pArr[holePosition-1];
                  holePosition--;
                }

                pArr[holePosition] = valueToInsert;
              }
            } // for(s32 x=0; x<arrWidth; x++)
          } // if(sortAscending) ... else
        } // if(sortWhichDimension == 0) ... else

        return RESULT_OK;
      }

      template<typename Type> Result Sort(Array<Type> &arr, Array<s32> &indexes, const s32 sortWhichDimension, const bool sortAscending)
      {
        const s32 arrHeight = arr.get_size(0);
        const s32 arrWidth = arr.get_size(1);

        AnkiConditionalErrorAndReturnValue(arr.IsValid(),
          RESULT_FAIL, "Sort", "Input array is invalid");

        AnkiConditionalErrorAndReturnValue(indexes.IsValid(),
          RESULT_FAIL, "Sort", "indexes array is invalid");

        AnkiConditionalErrorAndReturnValue(sortWhichDimension==0 || sortWhichDimension==1,
          RESULT_FAIL, "Sort", "sortWhichDimension must be zero or one");

        AnkiConditionalErrorAndReturnValue(indexes.get_size(0) == arrHeight && indexes.get_size(1) == arrWidth,
          RESULT_FAIL, "Sort", "indexes must be the same size as arr");

        if(sortWhichDimension == 0) {
          // TODO: This columnwise sorting could be sped up, with smarter array indexing.
          if(sortAscending) {
            for(s32 x=0; x<arrWidth; x++) {
              indexes[0][x] = 0;

              for(s32 y=1; y<arrHeight; y++) {
                const Type valueToInsert = arr[y][x];

                s32 holePosition = y;

                while(holePosition > 0 && valueToInsert < arr[holePosition-1][x]) {
                  arr[holePosition][x] = arr[holePosition-1][x];
                  indexes[holePosition][x] = indexes[holePosition-1][x];
                  holePosition--;
                }

                arr[holePosition][x] = valueToInsert;
                indexes[holePosition][x] = y;
              }
            } // for(s32 x=0; x<arrWidth; x++)
          } else { // if(sortAscending)
            for(s32 x=0; x<arrWidth; x++) {
              indexes[0][x] = 0;

              for(s32 y=1; y<arrHeight; y++) {
                const Type valueToInsert = arr[y][x];

                s32 holePosition = y;

                while(holePosition > 0 && valueToInsert > arr[holePosition-1][x]) {
                  arr[holePosition][x] = arr[holePosition-1][x];
                  indexes[holePosition][x] = indexes[holePosition-1][x];
                  holePosition--;
                }

                arr[holePosition][x] = valueToInsert;
                indexes[holePosition][x] = y;
              }
            } // for(s32 x=0; x<arrWidth; x++)
          } // if(sortAscending) ... else
        } else { // sortWhichDimension == 1
          if(sortAscending) {
            for(s32 y=0; y<arrHeight; y++) {
              Type * const pArr = arr[y];
              s32 * const pIndexes = indexes[y];

              pIndexes[0] = 0;

              for(s32 x=1; x<arrWidth; x++) {
                const Type valueToInsert = pArr[x];

                s32 holePosition = x;

                while(holePosition > 0 && valueToInsert < pArr[holePosition-1]) {
                  pArr[holePosition] = pArr[holePosition-1];
                  pIndexes[holePosition] = pIndexes[holePosition-1];
                  holePosition--;
                }

                pArr[holePosition] = valueToInsert;
                pIndexes[holePosition] = x;
              }
            } // for(s32 x=0; x<arrWidth; x++)
          } else { // if(sortAscending)
            for(s32 y=0; y<arrHeight; y++) {
              Type * const pArr = arr[y];
              s32 * const pIndexes = indexes[y];

              pIndexes[0] = 0;

              for(s32 x=1; x<arrWidth; x++) {
                const Type valueToInsert = pArr[x];

                s32 holePosition = x;

                while(holePosition > 0 && valueToInsert > pArr[holePosition-1]) {
                  pArr[holePosition] = pArr[holePosition-1];
                  pIndexes[holePosition] = pIndexes[holePosition-1];
                  holePosition--;
                }

                pArr[holePosition] = valueToInsert;
                pIndexes[holePosition] = x;
              }
            } // for(s32 x=0; x<arrWidth; x++)
          } // if(sortAscending) ... else
        } // if(sortWhichDimension == 0) ... else

        return RESULT_OK;
      }

      template<typename Type> Result MakeSymmetric(Type &arr, bool lowerToUpper)
      {
        AnkiConditionalErrorAndReturnValue(arr.get_size(0) == arr.get_size(1),
          RESULT_FAIL, "MakeSymmetric", "Input array must be square");

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
            RESULT_FAIL, "Matrix::Elementwise::ApplyOperation", "Invalid array in1");

          AnkiConditionalErrorAndReturnValue(in2Array.IsValid(),
            RESULT_FAIL, "Matrix::Elementwise::ApplyOperation", "Invalid array in2");

          AnkiConditionalErrorAndReturnValue(out1Array.IsValid(),
            RESULT_FAIL, "Matrix::Elementwise::ApplyOperation", "Invalid array out");

          ArraySliceLimits_in2_out1<s32> limits(
            in1.get_ySlice(), in1.get_xSlice(), in1.get_isTransposed(),
            in2.get_ySlice(), in2.get_xSlice(), in2.get_isTransposed(),
            out.get_ySlice(), out.get_xSlice());

          AnkiConditionalErrorAndReturnValue(limits.isValid,
            RESULT_FAIL, "Matrix::Elementwise::ApplyOperation", "Limits is not valid");

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
            RESULT_FAIL, "Matrix::Elementwise::ApplyOperation", "Invalid array in1");

          AnkiConditionalErrorAndReturnValue(out1Array.IsValid(),
            RESULT_FAIL, "Matrix::Elementwise::ApplyOperation", "Invalid array out");

          ArraySliceLimits_in1_out1<s32> limits(
            in1.get_ySlice(), in1.get_xSlice(), in1.get_isTransposed(),
            out.get_ySlice(), out.get_xSlice());

          AnkiConditionalErrorAndReturnValue(limits.isValid,
            RESULT_FAIL, "Matrix::Elementwise::ApplyOperation", "Limits is not valid");

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
            RESULT_FAIL, "Matrix::Elementwise::ApplyOperation", "Invalid array in2");

          AnkiConditionalErrorAndReturnValue(out1Array.IsValid(),
            RESULT_FAIL, "Matrix::Elementwise::ApplyOperation", "Invalid array out");

          ArraySliceLimits_in1_out1<s32> limits(
            in2.get_ySlice(), in2.get_xSlice(), in2.get_isTransposed(),
            out.get_ySlice(), out.get_xSlice());

          AnkiConditionalErrorAndReturnValue(limits.isValid,
            RESULT_FAIL, "Matrix::Elementwise::ApplyOperation", "Limits is not valid");

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
            // If either input is transposed is allowed, then we will do an inefficent loop iteration

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