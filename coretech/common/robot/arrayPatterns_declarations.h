/**
File: arrayPatterns_declarations.h
Author: Peter Barnum
Created: 2013

Various common array patterns, such as zeros(), ones(), or exp() for gaussians

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_ARRAY_PATTERNS_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_ARRAY_PATTERNS_DECLARATIONS_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    // Similar to Matlab's zeros()
    template<typename Type> Result Zeros(Array<Type> &out);
    template<typename Type> Array<Type> Zeros(const s32 arrayHeight, const s32 arrayWidth, MemoryStack &memory);

    // Similat to Matlab's ones()
    template<typename Type> Result Ones(Array<Type> &out);
    template<typename Type> Array<Type> Ones(const s32 arrayHeight, const s32 arrayWidth, MemoryStack &memory);

    // TODO: Rand
    //template<typename Type> Result Rand(Array<Type> &arr);
    //template<typename Type> Array<Type> Rand(const s32 arrayHeight, const s32 arrayWidth, MemoryStack &memory);

    // Similar to Matlab's eye()
    template<typename Type> Result Eye(Array<Type> &out);
    template<typename Type> Array<Type> Eye(const s32 arrayHeight, const s32 arrayWidth, MemoryStack &memory);

    // Similar to Matlab's exp(in)
    template<typename Type> Array<Type> Exp(const Array<Type> &in, MemoryStack &memory);
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_ARRAY_PATTERNS_DECLARATIONS_H_
