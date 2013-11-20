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

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class FixedLengthList;

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

      Type get_start() const;

      Type get_increment() const;

      Type get_end() const;

      s32 get_size() const;

    protected:
      // For speed, FixedLengthList is allowed to access protected members, instead of having to
      // construct a new LinearSequence every time an element is popped or pushed is changed
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

    // TODO: linspace
    //template<typename Type> LinearSequence<Type> Linspace(const Type start, const Type end, const s32 size);

    // TODO: Logspace
    //template<typename Type> class Logspace : public Sequence<Type>
    //{
    //public:
    //protected:
    //};

    // TODO: Meshgrid
    //template<typename Type> class Meshgrid
    //{
    //public:
    //  // Matlab equivalent:
    //  Meshgrid(LinearSequence<Type> xGridVector, LinearSequence<Type> yGridVector);
    //protected:
    //};
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_DECLARATIONS_H_