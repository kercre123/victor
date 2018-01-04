/**
File: arraySlices_declarations.h
Author: Peter Barnum
Created: 2013

An array slice is a sub-array of an Array object.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_ARRAYSLICES_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_ARRAYSLICES_DECLARATIONS_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark --- Class Declarations ---

    template<typename Type> class ConstArraySlice;
    template<typename Type> class ArraySlice;
    template<typename Type> class ConstArraySliceExpression;

    // An ArraySlice is a simple indexing wrapper on top of an Array. The slice of an Array could be
    // a sub-rectangle of an array and/or skip every n-th element.
    //
    // For example, Array(0,3,-1,1,2,4) is the same as Matlab's array(1:3:end, 2:2:5).
    // (The Array indexing starts from zero vs Matlab's one, hence the different numbers).
    //
    // TODO: support non-int indexes
    // TODO: is there a better way of doing this than a completely different class, different only
    //       by const?
    template<typename Type> class ConstArraySlice
    {
    public:
      ConstArraySlice();

      // Directly convert an array to an ArraySlice, so all Arrays can be used as input
      ConstArraySlice(const Array<Type> &array);

      // It's probably easier to call array.operator() than this constructor directly
      ConstArraySlice(const Array<Type> &array, const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice);

      // ArraySlice Transpose doesn't modify the data, it just sets a flag
      ConstArraySliceExpression<Type> Transpose() const;

      bool IsValid() const;

      const LinearSequence<s32>& get_ySlice() const;

      const LinearSequence<s32>& get_xSlice() const;

      // Get the raw Array from the Slice. This is mainly useful for interfacing with functions that
      // don't support the full ArraySlice type, and should be used with caution.
      const Array<Type>& get_array() const;

    protected:
      LinearSequence<s32> ySlice;
      LinearSequence<s32> xSlice;

      Array<Type> array;

      // For speed, this is a direct pointer to the Array's protected data
      const Type * constArrayData;
    }; // template<typename Type> class ArraySlice

    // A non-const version of ConstArraySlice, see ConstArraySlice for details
    //
    // WARNING: A "const ArraySlice" doesn't have a const Array. Only ConstArraySlice has a const
    //          Array. This allows for implicit conversion to non-const function parameters.
    template<typename Type> class ArraySlice : public ConstArraySlice<Type>
    {
    public:
      ArraySlice();

      // Directly convert an array to an ArraySlice, so all Arrays can be used as input
      // The Array parameter is not a reference, to allow for implicit conversion
      ArraySlice(Array<Type> array);

      // It's probably easier to call array.operator() than this constructor directly
      // The Array parameter is not a reference, to allow for implicit conversion
      ArraySlice(Array<Type> array, const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice);

      // If automaticTranspose==true, then you can set a MxN slice with a NxM input.
      // Matlab allows this for vectors, though this method will also work for
      // arbitrary-sized arrays.
      s32 Set(const ConstArraySliceExpression<Type> &input, bool automaticTranspose=true);

      // Explicitly evaluate the input LinearSequence into this ArraySlice
      s32 Set(const LinearSequence<Type> &input);

      // Set all values of this slice to the given value.
      //
      // For example, "array(0,-1,1,4).Set(5);" is the same as
      // Matlab's "array(1:end, 2:5) = 5;"
      s32 Set(const Type value);

      // Copy values to this ArraySlice.
      // numValues must be equal to the number of values in this slice
      // Returns the number of values set
      s32 Set(const Type * const values, const s32 numValues);

      // Read in the input, then cast it to this object's type
      //
      // WARNING:
      // This should be kept explicit, to prevent accidental casting between different datatypes.
      template<typename InType> s32 SetCast(const ConstArraySliceExpression<Type> &input, bool automaticTranspose);
      //template<typename InType> s32 SetCast(const InType * const values, const s32 numValues); // TODO: implement

      // Get the raw Array from the Slice. This is mainly useful for interfacing with functions that
      // don't support the full ArraySlice type, and should be used with caution.
      Array<Type>& get_array();

    protected:

      // For speed, this is a direct pointer to the Array's protected data
      Type * arrayData;
    }; // template<typename Type> class ArraySlice

    // An ConstArraySliceExpression is like a ConstArraySlice, but can also be transposed
    // It may have other abilities in the future, but will probably always be const
    template<typename Type> class ConstArraySliceExpression : public ConstArraySlice<Type>
    {
    public:
      ConstArraySliceExpression();

      ConstArraySliceExpression(const Array<Type> input, bool isTransposed=false);

      ConstArraySliceExpression(const ArraySlice<Type> &input, bool isTransposed=false);

      ConstArraySliceExpression(const ConstArraySlice<Type> &input, bool isTransposed=false);

      // ArraySlice Transpose doesn't modify the data, it just sets a flag
      // This object isn't modified, but the returned object is.
      ConstArraySliceExpression<Type> Transpose() const;

      bool get_isTransposed() const;

    protected:
      bool isTransposed;
    };

    // To simplify the creation of kernels using an ArraySlice, and to aid the compiler optimizer,
    // an ArraySliceLimits can be initialized at the beginning of the function, then used as the
    // limits for the inner loops.

    // The suffix of in# and out# refer to the number of input and output matrices.
    // If output == 0, then the output is a scalar.

    template<typename Type> class ArraySliceSimpleLimits
    {
    public:
      Type xStart;
      Type xIncrement;
      s32  xSize;

      Type yStart;
      Type yIncrement;
      s32  ySize;

      ArraySliceSimpleLimits(const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice);
    };

    // In1 and out0 is a special, ultra-simple case, for one matrix input and a scalar output
    template<typename Type> class ArraySliceLimits_in1_out0
    {
    public:
      // Was this ArraySliceLimits initialized?
      bool isValid;

      ArraySliceSimpleLimits<Type> rawIn1Limits;

      ArraySliceLimits_in1_out0(const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice);
    };

    // One input, one output
    template<typename Type> class ArraySliceLimits_in1_out1
    {
    public:
      // Was this ArraySliceLimits initialized?
      bool isValid;

      // Can a simple (non-transposed) iteration be performed?
      bool isSimpleIteration;

      // These are the current values for the coordinates in the input and output images
      s32 out1Y;
      s32 out1X;
      s32 in1Y;
      s32 in1X;

      // The loops will be based on these iterators (these should match with the output's and inputs' sizes)
      s32 ySize;
      s32 xSize;

      // Depending on whether ths input is transposed or not, either its X or Y coordinate should be
      // incremented every iteration of the inner loop
      s32 out1_xInnerIncrement;
      s32 in1_xInnerIncrement;
      s32 in1_yInnerIncrement;

      ArraySliceLimits_in1_out1(
        const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice, bool in1_isTransposed,
        const LinearSequence<Type> &out1_ySlice, const LinearSequence<Type> &out1_xSlice);

      // This should be called at the top of the y-iteration loop, before the x-iteration loop. This will update the out# and in# values for X and Y.
      inline void OuterIncrementTop();

      // This should be called at the botom of the y-iteration loop, after the x-iteration loop. This will update the out# and in# values for X and Y.
      inline void OuterIncrementBottom();

    protected:
      ArraySliceSimpleLimits<Type> rawOut1Limits;

      ArraySliceSimpleLimits<Type> rawIn1Limits;
      bool in1_isTransposed;
    };

    // Two inputs, one output
    template<typename Type> class ArraySliceLimits_in2_out1
    {
    public:
      // Was this ArraySliceLimits initialized?
      bool isValid;

      // Can a simple (non-transposed) iteration be performed?
      bool isSimpleIteration;

      // These are the current values for the coordinates in the input and output images
      s32 out1Y;
      s32 out1X;
      s32 in1Y;
      s32 in1X;
      s32 in2Y;
      s32 in2X;

      // The loops will be based on these iterators (these should match with the output's and inputs' sizes)
      s32 ySize;
      s32 xSize;

      // Depending on whether ths input is transposed or not, either its X or Y coordinate should be
      // incremented every iteration of the inner loop
      s32 out1_xInnerIncrement;
      s32 in1_xInnerIncrement;
      s32 in1_yInnerIncrement;
      s32 in2_xInnerIncrement;
      s32 in2_yInnerIncrement;

      ArraySliceLimits_in2_out1(
        const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice, bool in1_isTransposed,
        const LinearSequence<Type> &in2_ySlice, const LinearSequence<Type> &in2_xSlice, bool in2_isTransposed,
        const LinearSequence<Type> &out1_ySlice, const LinearSequence<Type> &out1_xSlice);

      // This should be called at the top of the y-iteration loop, before the x-iteration loop. This will update the out# and in# values for X and Y.
      inline void OuterIncrementTop();

      // This should be called at the botom of the y-iteration loop, after the x-iteration loop. This will update the out# and in# values for X and Y.
      inline void OuterIncrementBottom();

    protected:
      ArraySliceSimpleLimits<Type> rawOut1Limits;

      ArraySliceSimpleLimits<Type> rawIn1Limits;
      bool in1_isTransposed;

      ArraySliceSimpleLimits<Type> rawIn2Limits;
      bool in2_isTransposed;
    };
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_ARRAYSLICES_DECLARATIONS_H_
