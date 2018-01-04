/**
File: sequences_declarations.h
Author: Peter Barnum
Created: 2013

A Sequence is a mathematically-defined, ordered list. The sequence classes allow for operations on sequences, without requiring them to be explicitly evaluated.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_DECLARATIONS_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/flags_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class Array;
    template<typename Type> class FixedLengthList;
    template<typename Type> class ArraySlice;
    class MemoryStack;

    // #pragma mark --- Class Declarations ---
    template<typename Type> class Sequence
    {
    }; // class Sequence

    // A LinearSequence is like the result of a call to Matlab's linspace() It has a start, end, and
    // increment. It does not explicitly compute the values in the sequence, so does not require
    // much memory.
    //
    // WARNING:
    // The "end" of a LinearSequence is computed automatically, and is less-than-or-equal-to the
    // requested end.
    template<typename Type> class LinearSequence : public Sequence<Type>
    {
    public:

      LinearSequence();

      // Matlab equivalent: start:end
      LinearSequence(const Type start, const Type end);

      // Matlab equivalent: start:increment:end
      LinearSequence(const Type start, const Type increment, const Type end);

      // No Matlab equivalent
      // NOTE: end is unused. It is just present to prevent confusion with the other polymorphic constructors
      LinearSequence(const Type start, const Type increment, const Type end, const s32 size);

      // Explicitly evaluate each element of the sequence, and put the results in an Array.
      Array<Type> Evaluate(MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false,false)) const;
      Result Evaluate(ArraySlice<Type> out) const;

      Type get_start() const;

      // NOTE: The increment is meaningless for LinearSequences of size 0 or 1.
      Type get_increment() const;

      // Note: End it not computed, as it is tempting to use it as a loop condition, but it is not safe
      // Type get_end() const;

      // Matlab equivalent: length(start:increment:end)
      s32 get_size() const;

    protected:
      // For speed, FixedLengthList is allowed to access protected members, instead of having to
      // construct a new LinearSequence every time an element is popped or pushed
      template<typename FixedLengthListType> friend class FixedLengthList;

      s32 size;

      Type start;
      Type increment;

      static s32 computeSize(const Type start, const Type increment, const Type end);
    }; // class LinearSequence

    // IndexSequence creates the input for slicing an Array
    // If start or end is less than 0, it is equivalent to (end+value)
    template<typename Type> LinearSequence<Type> IndexSequence(Type start, Type end, s32 arraySize);
    template<typename Type> LinearSequence<Type> IndexSequence(Type start, Type increment, Type end, s32 arraySize);
    LinearSequence<s32> IndexSequence(s32 arraySize); // Internally, it sets start==0, end=arraySize-1, like the Matlab colon operator array(:,:)

    // Linspace only works correctly for f32 and f64. To prevent misusage, trying ints will give a linker error.
    template<typename Type> LinearSequence<Type> Linspace(const Type start, const Type end, const s32 size);

    // These do not link, as they are unsafe
    template<> LinearSequence<u8> Linspace(const u8 start, const u8 end, const s32 size);
    template<> LinearSequence<s8> Linspace(const s8 start, const s8 end, const s32 size);
    template<> LinearSequence<u16> Linspace(const u16 start, const u16 end, const s32 size);
    template<> LinearSequence<s16> Linspace(const s16 start, const s16 end, const s32 size);
    template<> LinearSequence<u32> Linspace(const u32 start, const u32 end, const s32 size);
    template<> LinearSequence<s32> Linspace(const s32 start, const s32 end, const s32 size);
    template<> LinearSequence<u64> Linspace(const u64 start, const u64 end, const s32 size);
    template<> LinearSequence<s64> Linspace(const s64 start, const s64 end, const s32 size);

    // TODO: Logspace
    //template<typename Type> class Logspace : public Sequence<Type>
    //{
    //public:
    //protected:
    //};

    // A Meshgrid is like the result of a call to Matlab's meshgrid(). It is made of two
    // LinearSequence objects, so does not require much memory.
    template<typename Type> class Meshgrid
    {
    public:
      Meshgrid();

      // Matlab equivalent: meshgrid(xGridVector, yGridVector)
      Meshgrid(const LinearSequence<Type> xGridVector, const LinearSequence<Type> yGridVector);

      // Allocate an Array, and evaluate this Meshgrid object
      //
      // If isOutColumnMajor==true, then the output vector will be column-major(like Matlab)
      // The first suffix X or Y is for the xGrid vs yGrid
      // The second suffix 1 or 2 is for 1D vs 2D output
      Array<Type> EvaluateX1(bool isOutColumnMajor, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false,false)) const;
      Array<Type> EvaluateX2(MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false,false)) const;
      Array<Type> EvaluateY1(bool isOutColumnMajor, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false,false)) const;
      Array<Type> EvaluateY2(MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false,false)) const;

      // Evaluate this Meshgrid object into a pre-allocated Array
      //
      // If isOutColumnMajor==true, then the output vector will be column-major(like Matlab)
      Result EvaluateX1(bool isOutColumnMajor, ArraySlice<Type> out) const;
      Result EvaluateX2(ArraySlice<Type> out) const;
      Result EvaluateY1(bool isOutColumnMajor, ArraySlice<Type> out) const;
      Result EvaluateY2(ArraySlice<Type> out) const;

      s32 get_numElements() const;

      inline const LinearSequence<Type>& get_xGridVector() const;

      inline const LinearSequence<Type>& get_yGridVector() const;

    protected:
      LinearSequence<Type> xGridVector;
      LinearSequence<Type> yGridVector;
    };
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_DECLARATIONS_H_
