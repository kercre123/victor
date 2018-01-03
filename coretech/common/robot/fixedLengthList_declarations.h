/**
File: fixedLengthList_declarations.h
Author: Peter Barnum
Created: 2013

A FixedLengthList is like a std::vector, but has a fixed maximum size. This maximum is allocated at contruction.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_FIXED_LENGTH_LIST_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_FIXED_LENGTH_LIST_DECLARATIONS_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d_declarations.h"
#include "coretech/common/robot/arraySlices_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    class SerializedBuffer;

    // A FixedLengthList is a list with a fixed maximum size, which is allocated at construction.
    template<typename Type> class FixedLengthList : public ArraySlice<Type>
    {
    public:
      FixedLengthList();

      // Constructor for a FixedLengthList, pointing to user-allocated data.
      FixedLengthList(s32 maximumSize, void * data, s32 dataLength, const Flags::Buffer flags=Flags::Buffer(true,false,false));

      // Constructor for a FixedLengthList, pointing to user-allocated MemoryStack
      FixedLengthList(s32 maximumSize, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false,false));

      bool IsValid() const;

      // Resize will use MemoryStack::Reallocate() to change the FixedLengthList's size. It only works if this
      // FixedLengthList was the last thing allocated. The reallocated memory will not be cleared
      //
      // WARNING:
      // This will not update any references to the memory, you must update all references manually.
      Result Resize(s32 maximumSize, MemoryStack &memory);

      Result PushBack(const Type &value);

      // Will act as a normal pop, except when the list is empty. Then subsequent
      // calls will keep returning the first value in the list.
      Type PopBack();

      // Sets the size to zero, but does not modify any data. Equivalent to set_size(0)
      inline void Clear();

      // Does this ever need to be declared explicitly?
      //FixedLengthList& operator= (const FixedLengthList & rightHandSide);

      // Pointer to the data, at a given location
      inline Type* Pointer(const s32 index);
      inline const Type* Pointer(const s32 index) const;

      // Use this operator for normal C-style vector indexing. For example, "list[5] = 6;" will set
      // the element in the fifth row and first column to 6. This is the same as "*list.Pointer(5) =
      // 6;"
      //
      // NOTE:
      // Using this in a inner loop may be less efficient than using an explicit pointer with a
      // restrict keyword (Though the runtime cost isn't nearly as large as the [] operator for the
      // Array class). For speeding up performance-critical inner loops, use something like: "Type *
      // restrict pList = list.Pointer(0);" outside the inner loop, then index
      // pList in the inner loop.
      inline const Type& operator[](const s32 index) const;
      inline Type& operator[](const s32 index);

      // Print out the contents of this FixedLengthList
      Result Print(const char * const variableName = "FixedLengthList", const s32 minIndex = 0, const s32 maxIndex = 0x7FFFFFE) const;

      // Set every element in the Array to zero, including the stride padding, but not including the optional fill patterns (if they exist)
      // Returns the number of bytes set to zero
      inline s32 SetZero();

      // Read in the input, then cast it to this object's type
      //
      // WARNING:
      // This should be kept explicit, to prevent accidental casting between different datatypes.
      template<typename InType> s32 SetCast(const FixedLengthList<InType> &input, bool automaticTranspose=true);
      //template<typename InType> s32 SetCast(const InType * const values, const s32 numValues); // TODO: implement

      // The maximum size is set at object construction
      inline s32 get_maximumSize() const;

      // The current size changes as the FixedLengthList is used
      inline s32 get_size() const;

      // Attempt to set the size to newSize. Returns the value that was actually set.
      s32 set_size(s32 newSize);

    protected:
      // TODO: make less hacky
      friend class SerializedBuffer;
    }; // class FixedLengthList
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_FIXEDLENGTHLIST_DECLARATIONS_H_
