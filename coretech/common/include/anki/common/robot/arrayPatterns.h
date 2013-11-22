/**
File: arrayPatterns.h
Author: Peter Barnum
Created: 2013

Various common array patterns, such as zeros(), ones(), or exp() for gaussians

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_ARRAY_PATTERNS_H_
#define _ANKICORETECHEMBEDDED_COMMON_ARRAY_PATTERNS_H_

#include "anki/common/robot/arrayPatterns_declarations.h"
#include "anki/common/robot/matrix.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> Result Zeros(Array<Type> &out)
    {
      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        RESULT_FAIL, "Zeros", "out is invalid");

      out.SetZero();

      return RESULT_OK;
    }

    template<typename Type> Array<Type> Zeros(const s32 arrayHeight, const s32 arrayWidth, MemoryStack &memory)
    {
      Array<Type> out(arrayHeight, arrayWidth, memory, Flags::Buffer(true, false));

      return out;
    }

    template<typename Type> Result Ones(Array<Type> &out)
    {
      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        RESULT_FAIL, "Ones", "out is invalid");

      out.Set(static_cast<Type>(1));

      return RESULT_OK;
    }

    template<typename Type> Array<Type> Ones(const s32 arrayHeight, const s32 arrayWidth, MemoryStack &memory)
    {
      Array<Type> out(arrayHeight, arrayWidth, memory, Flags::Buffer(false, false));

      Ones(out);

      return out;
    }

    template<typename Type> Result Eye(Array<Type> &out)
    {
      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        RESULT_FAIL, "Eye", "out is invalid");

      const s32 arrayHeight = out.get_size(0);

      out.SetZero();

      for(s32 y=0; y<arrayHeight; y++) {
        Type * const pOut = out.Pointer(y, 0);
        pOut[y] = static_cast<Type>(1);
      }

      return RESULT_OK;
    }

    template<typename Type> Array<Type> Eye(const s32 arrayHeight, const s32 arrayWidth, MemoryStack &memory)
    {
      Array<Type> out(arrayHeight, arrayWidth, memory, Flags::Buffer(false, false));

      Eye(out);

      return out;
    }

    /*template<typename Type> Result Exp(const Array<Type> &in, Array<Type> &out)
    {
    AnkiConditionalErrorAndReturnValue(in.IsValid(),
    RESULT_FAIL, "Exp", "in is invalid");

    AnkiConditionalErrorAndReturnValue(out.IsValid(),
    RESULT_FAIL, "Exp", "out is invalid");

    const s32 arrayHeight = out.get_size(0);
    const s32 arrayWidth = out.get_size(1);

    for(s32 y=0; y<arrayHeight; y++) {
    const Type * const pIn = in.Pointer(y, 0);
    Type * const pOut = out.Pointer(y, 0);

    for(s32 x=0; x<arrayWidth; x++) {
    pOut[x] = exp(pIn[x]);
    }
    }

    return RESULT_OK;
    }*/

    template<typename Type> Array<Type> Exp(const Array<Type> &in, MemoryStack &memory)
    {
      Array<Type> out(in.get_size(0), in.get_size(1), memory, Flags::Buffer(false, false));

      Matrix::Exp<Type,Type,Type>(in, out);

      return out;
    }
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_ARRAY_PATTERNS_H_
