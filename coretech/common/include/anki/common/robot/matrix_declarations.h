/**
File: matrix_declarations.h
Author: Peter Barnum
Created: 2013

Various Matrix operations, such as matrix multiply and addition.

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

      // Return the mean of every element in the Array
      template<typename Array_Type, typename Accumulator_Type> Array_Type Mean(const ConstArraySliceExpression<Array_Type> &mat);

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

      // Perform the matrix multiplication "out = in1 * in2'"
      // Note that this is the naive O(n^3) Definition
      // MultiplyTranspose has better access patterns than Multiply for certain types of arrays, so could be a lot faster (and easier to accelerate)
      template<typename InType, typename OutType> Result MultiplyTranspose(const Array<InType> &in1, const Array<InType> &in2Transposed, Array<OutType> &out);

      //
      // Linear Algebra and Linear Solvers
      //

      // Compute the Cholesky-Banachiewicz decomposition, to return a lower-triangular matrix L such that A=L*L'
      template<typename Type> Result SolveLeastSquaresWithCholesky(
        Array<Type> &A_L,       //!< Input A Matrix and Output lower-triangular L matrix
        Array<Type> &Bt_Xt,     //!< Input B-transpose matrix and Output X-transpose solution
        bool realCholesky=false //!< A real Cholesky is slower to compute, and not required if only the X solution is required
        );

      // Compute the homography such that "transformedPoints = homography * originalPoints"
      //
      // Warning: This uses the inhomogeneous solution and the Cholesky decomposition, therefore it
      //          will be incorrect if H_33 is zero, which happens in certain cases of lines at
      //          inifinty. For more details, see Multiple View Geometry 2nd Edition, Example 4.1
      template<typename Type> Result EstimateHomography(
        const FixedLengthList<Point<Type> > &originalPoints,    //!< Four points in the original coordinate system
        const FixedLengthList<Point<Type> > &transformedPoints, //!< Four points in the transformed coordinate system
        Array<Type> &homography, //!< A 3x3 transformation matrix
        MemoryStack scratch //!< Scratch memory
        );

      //template<typename InType, typename IntermediateType, typename OutType> Result CholeskyDecomposition(
      //  const Array<InType> &A,                    //!< Input A Matrix
      //  Array<IntermediateType> &diagonalInverses, //!< Vector of the inverses of the diagonals of L
      //  Array<OutType> &L                          //!< Output lower-triangular L matrix
      //  );

      //template<typename InType, typename IntermediateType, typename OutType> Result SolveWithLowerTriangular(
      //  const Array<InType> &L,                          //!< Input lower-triangular L matrix (such as computed by CholeskyDecomposition)
      //  const Array<InType> &b,                          //!< Input b matrix
      //  const Array<IntermediateType> &diagonalInverses, //!< Vector of the inverses of the diagonals of L
      //  Array<OutType> &x                                //!< Output x solution
      //  );

      // Solves Ax = b
      // Specifically, it uses SVD to minimize ||Ax - b||
      // Note that the A, b, and x matrices are transposed (this is because for large numbers of samples, transposed inputs are liable to be faster)
      //Result SolveLeastSquaresWithSVD_f32(Array<f32> &At, const Array<f32> &bt, Array<f32> &xt, MemoryStack scratch);
      //Result SolveLeastSquaresWithSVD_f64(Array<f64> &At, const Array<f64> &bt, Array<f64> &xt, MemoryStack scratch);

      //
      // Matrix structure operations
      //

      // matlab equivalent: out = reshape(in, [M,N]);
      template<typename TypeIn, typename TypeOut> Result Reshape(const bool isColumnMajor, const Array<TypeIn> &in, Array<TypeOut> &out);
      template<typename TypeIn, typename TypeOut> Array<TypeOut> Reshape(const bool isColumnMajor, const Array<TypeIn> &in, const s32 newHeight, const s32 newWidth, MemoryStack &memory);

      // matlab equivalent: out = in(:);
      template<typename TypeIn, typename TypeOut> Result Vectorize(const bool isColumnMajor, const Array<TypeIn> &in, Array<TypeOut> &out);
      template<typename TypeIn, typename TypeOut> Array<TypeOut> Vectorize(const bool isColumnMajor, const Array<TypeIn> &in, MemoryStack &memory);

      // Perform an immediate matrix transpose (unlike the lazy transpose of ArraySlice)
      template<typename TypeIn, typename TypeOut> Result Transpose(const Array<TypeIn> &in, Array<TypeOut> &out);

      //
      // Misc matrix operations
      //

      // Works the same as the Matlab sort() for matrices.
      // Sort(X) sorts each column of X in ascending order.
      // NOTE: this currently uses insertion sort, so may be slow for large, badly-unsorted arrays
      template<typename Type> Result Sort(Array<Type> &arr, const s32 sortWhichDimension=0, const bool sortAscending=true);

      // indexes must be allocated, but will be overwritten by Sort()
      template<typename Type> Result Sort(Array<Type> &arr, Array<s32> &indexes, const s32 sortWhichDimension=0, const bool sortAscending=true);

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
          static inline OutType BinaryElementwiseOperation(const InType value1, const InType value2) {return static_cast<OutType>(expf(static_cast<IntermediateType>(value1)));}
        };

        template<typename InType, typename Operator, typename OutType> Result ApplyOperation(const ConstArraySliceExpression<InType> &in1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);
        template<typename InType, typename Operator, typename OutType> Result ApplyOperation(const ConstArraySliceExpression<InType> &in1, const InType value2, ArraySlice<OutType> out);
        template<typename InType, typename Operator, typename OutType> Result ApplyOperation(const InType value1, const ConstArraySliceExpression<InType> &in2, ArraySlice<OutType> out);
      } // namespace Elementwise
    } // namespace Matrix
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_MATRIX_DECLARATIONS_H_