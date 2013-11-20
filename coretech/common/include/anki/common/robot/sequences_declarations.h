/**
File: sequences_declarations.h
Author: Peter Barnum
Created: 2013

Declarations of sequences.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_DECLARATIONS_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/flags_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class Array;
    template<typename Type> class FixedLengthList;
    template<typename Type> class ArraySlice;
    class MemoryStack;

#pragma mark --- Class Declarations ---
    template<typename Type> class Sequence
    {
    }; // class Sequence

    template<typename Type> class LinearSequence : public Sequence<Type>
    {
    public:

      LinearSequence();

      // Matlab equivalent: start:end
      LinearSequence(const Type start, const Type end);

      // Matlab equivalent: start:increment:end
      LinearSequence(const Type start, const Type increment, const Type end);

      Array<Type> Evaluate(MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false)) const;
      Result Evaluate(ArraySlice<Type> out) const;

      Type get_start() const;

      Type get_increment() const;

      Type get_end() const;

      s32 get_size() const;

    protected:
      // For speed, FixedLengthList is allowed to access protected members, instead of having to
      // construct a new LinearSequence every time an element is popped or pushed
      template<typename FixedLengthListType> friend class FixedLengthList;

      Type start;
      Type increment;
      Type end;

      s32 size;

      static s32 computeSize(const Type start, const Type increment, const Type end);
    }; // class LinearSequence

    // IndexSequence creates the input for slicing an Array
    // If start or end is less than 0, it is equivalent to (end+value)
    template<typename Type> LinearSequence<Type> IndexSequence(Type start, Type end, s32 arraySize);
    template<typename Type> LinearSequence<Type> IndexSequence(Type start, Type increment, Type end, s32 arraySize);
    LinearSequence<s32> IndexSequence(s32 arraySize); // Internally, it sets start==0, end=arraySize-1, like the Matlab colon operator array(:,:)

    // Linspace only really works correctly for f32 and f64, because getting the correct start and
    // end may be impossible for integers.
    template<typename Type> LinearSequence<Type> Linspace(const Type start, const Type end, const s32 size);

    // TODO: Logspace
    //template<typename Type> class Logspace : public Sequence<Type>
    //{
    //public:
    //protected:
    //};

    template<typename Type> class Meshgrid
    {
    public:
      // Matlab equivalent: meshgrid(xGridVector, yGridVector)
      Meshgrid(const LinearSequence<Type> xGridVector, const LinearSequence<Type> yGridVector);

      // If xVector==true, evaluate xGridVector. If false, evaluate yGridVector
      // If isColumnMajor==true, then the output vector will be column-major(like Matlab)
      Array<Type> EvaluateX(bool isColumnMajor, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false)) const;
      Array<Type> EvaluateY(bool isColumnMajor, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false)) const;
      Result EvaluateX(bool isColumnMajor, ArraySlice<Type> out) const;
      Result EvaluateY(bool isColumnMajor, ArraySlice<Type> out) const;

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