/**
File: matrix_declarations.h
Author: Peter Barnum
Created: 2013

Declarations of matrix.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_MATRIX_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_MATRIX_DECLARATIONS_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d_declarations.h"
#include "anki/common/robot/arraySlices_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Matrix
    {
#pragma mark --- Declarations ---

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
      template<typename InType, typename IntermediateType, typename OutType> Result Add(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);
      template<typename InType, typename IntermediateType, typename OutType> Result Add(const ConstArraySliceExpression<InType> &in1, const InType value2, ArraySlice<OutType> out);
      template<typename InType, typename IntermediateType, typename OutType> Result Add(const InType value1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);

      // Elementwise subtract two arrays. in1, in2, and out can be the same array
      template<typename InType, typename IntermediateType, typename OutType> Result Subtract(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);
      template<typename InType, typename IntermediateType, typename OutType> Result Subtract(const ConstArraySliceExpression<InType> &in1, const InType value2, ArraySlice<OutType> out);
      template<typename InType, typename IntermediateType, typename OutType> Result Subtract(const InType value1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);

      // Elementwise multiply two arrays. in1, in2, and out can be the same array
      template<typename InType, typename IntermediateType, typename OutType> Result DotMultiply(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);
      template<typename InType, typename IntermediateType, typename OutType> Result DotMultiply(const ConstArraySliceExpression<InType> &in1, const InType value2, ArraySlice<OutType> out);
      template<typename InType, typename IntermediateType, typename OutType> Result DotMultiply(const InType value1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);

      // Elementwise divide two arrays. in1, in2, and out can be the same array
      template<typename InType, typename IntermediateType, typename OutType> Result DotDivide(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);
      template<typename InType, typename IntermediateType, typename OutType> Result DotDivide(const ConstArraySliceExpression<InType> &in1, const InType value2, ArraySlice<OutType> out);
      template<typename InType, typename IntermediateType, typename OutType> Result DotDivide(const InType value1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);

      // Elementwise exponential on an array
      template<typename InType, typename IntermediateType, typename OutType> Result Exp(const ConstArraySliceExpression<InType> &in, ArraySlice<OutType> out);

      //
      // Standard matrix operations
      //

      // Perform the matrix multiplication "out = in1 * in2"
      // Note that this is the naive O(n^3) Definition
      template<typename InType, typename OutType> Result Multiply(const Array<InType> &in1, const Array<InType> &in2, Array<OutType> &out);

      //
      // Misc matrix operations
      //

      // For a square array, either:
      // 1. When lowerToUpper==true,  copies the lower (left)  triangle to the upper (right) triangle
      // 2. When lowerToUpper==false, copies the upper (right) triangle to the lower (left)  triangle
      // Functionally the same as OpenCV completeSymm()
      template<typename Type> Result MakeSymmetric(Type &arr, bool lowerToUpper = false);

      // There's probably no need to use these directly. Instead, use the normal Matrix:: operations, like Matrix::Add
      namespace Elementwise
      {
        template<typename InType, typename IntermediateType, typename OutType> class Add {
        public:
          static inline OutType BinaryElementwiseOperation(const InType value1, const InType value2) {return static_cast<OutType>(static_cast<IntermediateType>(value1) + static_cast<IntermediateType>(value2));}
        };

        template<typename InType, typename IntermediateType, typename OutType> class Subtract {
        public:
          static inline OutType BinaryElementwiseOperation(const InType value1, const InType value2) {return static_cast<OutType>(static_cast<IntermediateType>(value1) - static_cast<IntermediateType>(value2));}
        };

        template<typename InType, typename IntermediateType, typename OutType> class DotMultiply {
        public:
          static inline OutType BinaryElementwiseOperation(const InType value1, const InType value2) {return static_cast<OutType>(static_cast<IntermediateType>(value1) * static_cast<IntermediateType>(value2));}
        };

        template<typename InType, typename IntermediateType, typename OutType> class DotDivide {
        public:
          static inline OutType BinaryElementwiseOperation(const InType value1, const InType value2) {return static_cast<OutType>(static_cast<IntermediateType>(value1) / static_cast<IntermediateType>(value2));}
        };

        // Technically a unary operator, but we ignore the second parameter
        // TODO: if this is slow, make a unary version of ApplyOperation
        template<typename InType, typename IntermediateType, typename OutType> class Exp {
        public:
          static inline OutType BinaryElementwiseOperation(const InType value1, const InType value2) {return static_cast<OutType>(exp(static_cast<IntermediateType>(value1)));}
        };

        template<typename InType, typename Operator, typename OutType> Result ApplyOperation(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);
        template<typename InType, typename Operator, typename OutType> Result ApplyOperation(const ConstArraySliceExpression<InType> &in1, const InType value2, ArraySlice<OutType> out);
        template<typename InType, typename Operator, typename OutType> Result ApplyOperation(const InType value1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);
      }
    } // namespace Matrix
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_MATRIX_DECLARATIONS_H_