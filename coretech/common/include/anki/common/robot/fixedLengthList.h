/**
File: fixedLengthList.h
Author: Peter Barnum
Created: 2013

Definitions of fixedLenghtList_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_FIXED_LENGTH_LIST_H_
#define _ANKICORETECHEMBEDDED_COMMON_FIXED_LENGTH_LIST_H_

#include "anki/common/robot/fixedLengthList_declarations.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/arraySlices.h"

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark --- FixedLengthList Definitions ---

    template<typename Type> inline Type* FixedLengthList<Type>::Pointer(const s32 index)
    {
      return this->arrayData + index;
    }

    template<typename Type> inline const Type* FixedLengthList<Type>::Pointer(const s32 index) const
    {
      return this->arrayData + index;
    }

    template<typename Type> inline const Type& FixedLengthList<Type>::operator[](const s32 index) const
    {
      return *(this->arrayData + index);
    }

    template<typename Type> inline Type& FixedLengthList<Type>::operator[](const s32 index)
    {
      return *(this->arrayData + index);
    }

    template<typename Type> FixedLengthList<Type>::FixedLengthList()
      : ArraySlice<Type>()
    {
      this->arrayData = NULL;
      this->set_size(0);
    } // FixedLengthList<Type>::FixedLengthList()

    template<typename Type> FixedLengthList<Type>::FixedLengthList(s32 maximumSize, void * data, s32 dataLength, const Flags::Buffer flags)
      : ArraySlice<Type>(Array<Type>(1, maximumSize, data, dataLength, flags), LinearSequence<s32>(0,0), LinearSequence<s32>(0,0))
    {
      this->arrayData = this->array.Pointer(0,0);

      if(flags.get_isFullyAllocated()) {
        this->set_size(maximumSize);
      } else {
        this->set_size(0);
      }
    } // FixedLengthList<Type>::FixedLengthList(s32 maximumSize, void * data, s32 dataLength, const Flags::Buffer flags)

    template<typename Type> FixedLengthList<Type>::FixedLengthList(s32 maximumSize, MemoryStack &memory, const Flags::Buffer flags)
      : ArraySlice<Type>(Array<Type>(1, maximumSize, memory, flags), LinearSequence<s32>(0,0), LinearSequence<s32>(0,0))
    {
      this->arrayData = this->array.Pointer(0,0);

      if(flags.get_isFullyAllocated()) {
        this->set_size(maximumSize);
      } else {
        this->set_size(0);
      }
    } // FixedLengthList<Type>::FixedLengthList(s32 maximumSize, MemoryStack &memory, const Flags::Buffer flags)

    template<typename Type> bool FixedLengthList<Type>::IsValid() const
    {
      if(this->get_size() > this->get_maximumSize()) {
        return false;
      }

      return this->array.IsValid();
    } // bool FixedLengthList<Type>::IsValid() const

    template<typename Type> Result FixedLengthList<Type>::PushBack(const Type &value)
    {
      const s32 curSize = this->get_size();

      if(curSize >= this->get_maximumSize()) {
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      *this->Pointer(curSize) = value;

      this->xSlice.size = curSize+1;
      this->xSlice.end = curSize;

      return RESULT_OK;
    } // Result FixedLengthList<Type>::PushBack(const Type &value)

    template<typename Type> Type FixedLengthList<Type>::PopBack()
    {
      const s32 curSize = this->get_size();

      if(curSize == 0) {
        return *this->Pointer(0);
      }

      const Type value = *this->Pointer(curSize-1);
      this->xSlice.size = curSize-1;
      this->xSlice.end = curSize-2;

      return value;
    } // Type FixedLengthList<Type>::PopBack()

    template<typename Type> inline void FixedLengthList<Type>::Clear()
    {
      this->set_size(0);
    } // void FixedLengthList<Type>::Clear()

    template<typename Type> inline s32 FixedLengthList<Type>::SetZero()
    {
      return this->array.SetZero();
    }

    template<typename Type> inline s32 FixedLengthList<Type>::get_maximumSize() const
    {
      return this->array.get_size(1);
    } // s32 FixedLengthList<Type>::get_maximumSize() const

    template<typename Type> inline s32 FixedLengthList<Type>::get_size() const
    {
      return this->xSlice.size;
    } // s32 FixedLengthList<Type>::get_size() const

    // Attempt to set the size to newSize. Returns the value that was actually set.
    template<typename Type> s32 FixedLengthList<Type>::set_size(s32 newSize)
    {
      newSize = MIN(this->get_maximumSize(), MAX(0,newSize));

      this->xSlice.size = newSize;
      this->xSlice.end = newSize-1;

      return newSize;
    } // s32 FixedLengthList<Type>::set_size(s32 newSize)

    // Print out the contents of this FixedLengthList
    template<typename Type> Result FixedLengthList<Type>::Print(const char * const variableName, const s32 minIndex, const s32 maxIndex) const
    {
      return this->array.Print(variableName, 0, 0, MAX(0,minIndex), MIN(maxIndex, this->get_size()-1));
    } // Result FixedLengthList<Type>::Print(const char * const variableName, const s32 minIndex, const s32 maxIndex) const
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_FIXEDLENGTHLIST_H_
