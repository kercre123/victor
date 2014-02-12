/**
File: serialize.h
Author: Peter Barnum
Created: 2013

Definitions for serialize_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_H_
#define _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_H_

#include "anki/common/robot/serialize_declarations.h"
#include "anki/common/robot/flags.h"
#include "anki/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Definitions ---

    template<typename Type> Result SerializedBuffer::EncodeBasicType(u32 &code)
    {
      code = 0;

      if(!Flags::TypeCharacteristics<Type>::isBasicType) {
        return RESULT_FAIL;
      }

      if(Flags::TypeCharacteristics<Type>::isInteger) {
        code |= 0xFF;
      }

      if(Flags::TypeCharacteristics<Type>::isSigned) {
        code |= 0xFF00;
      }

      if(Flags::TypeCharacteristics<Type>::isFloat) {
        code |= 0xFF0000;
      }

      code |= sizeof(Type) << 24;

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::PushBack(Type &value)
    {
      u32 code = 0;

      if(SerializedBuffer::EncodeBasicType<Type>(code) != RESULT_OK)
        return RESULT_FAIL;

      void * segment = PushBack(NULL, 4+sizeof(Type));
    }

    template<typename Type> Result SerializedBuffer::PushBack(Array<Type> &value)
    {
    }

#pragma mark --- Specializations ---
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_H_
