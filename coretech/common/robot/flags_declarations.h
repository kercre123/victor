/**
File: flags_declarations.h
Author: Peter Barnum
Created: 2013

Different classes and functions need several variable-argument parameters and flags. These flag classes help separate interface and implementation.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_FLAGS_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_FLAGS_DECLARATIONS_H_

#include <float.h>
#include "coretech/common/shared/types.h"
#include "coretech/common/robot/config.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Flags
    {
      // #pragma mark --- Declarations ---

      class Buffer
      {
      public:
        // zeroAllocatedMemory - On initialization, set all values in the buffer to zero
        // useBoundaryFillPatterns - Buffers can have fill patterns at the edges of each piece
        // isFullyAllocated - If true, the input raw buffer is full of data, and the object should have usedSize == totalSize.
        Buffer();
        Buffer(bool zeroAllocatedMemory, bool useBoundaryFillPatterns, bool isFullyAllocated);

        Buffer& set_zeroAllocatedMemory(bool value);
        bool get_zeroAllocatedMemory() const;

        Buffer& set_useBoundaryFillPatterns(bool value);
        bool get_useBoundaryFillPatterns() const;

        Buffer& set_isFullyAllocated(bool value);
        bool get_isFullyAllocated() const;

        Buffer& set_rawFlags(u32 rawFlags);
        u32 get_rawFlags() const;

      protected:
        enum Flags
        {
          USE_BOUNDARY_FILL_PATTERNS = 1,
          ZERO_ALLOCATED_MEMORY = (1<<1),
          IS_FULLY_ALLOCATED = (1<<2)
        };

        u32 flags;
      };

      template<typename Type> class TypeCharacteristics
      {
      public:
        const static bool isBasicType = false;
        const static bool isInteger = false;
        const static bool isSigned = false;
        const static bool isFloat = false;
        const static bool isString = false;
      };

      // Similar to limits.h, which Myriad doesn't have
      template<typename Type> class numeric_limits
      {
      public:
        static inline Type epsilon() { return 0; }
      };

      // #pragma mark --- Declaration Specializations ---
      template<> class TypeCharacteristics<bool>
      {
      public:
        const static bool isBasicType = true;
        const static bool isInteger = true;
        const static bool isSigned = false;
        const static bool isFloat = false;
        const static bool isString = false;
      };

      template<> class TypeCharacteristics<u8>
      {
      public:
        const static bool isBasicType = true;
        const static bool isInteger = true;
        const static bool isSigned = false;
        const static bool isFloat = false;
        const static bool isString = false;
      };

      template<> class TypeCharacteristics<s8>
      {
      public:
        const static bool isBasicType = true;
        const static bool isInteger = true;
        const static bool isSigned = true;
        const static bool isFloat = false;
        const static bool isString = false;
      };

      template<> class TypeCharacteristics<u16>
      {
      public:
        const static bool isBasicType = true;
        const static bool isInteger = true;
        const static bool isSigned = false;
        const static bool isFloat = false;
        const static bool isString = false;
      };

      template<> class TypeCharacteristics<s16>
      {
      public:
        const static bool isBasicType = true;
        const static bool isInteger = true;
        const static bool isSigned = true;
        const static bool isFloat = false;
        const static bool isString = false;
      };

      template<> class TypeCharacteristics<u32>
      {
      public:
        const static bool isBasicType = true;
        const static bool isInteger = true;
        const static bool isSigned = false;
        const static bool isFloat = false;
        const static bool isString = false;
      };

      template<> class TypeCharacteristics<s32>
      {
      public:
        const static bool isBasicType = true;
        const static bool isInteger = true;
        const static bool isSigned = true;
        const static bool isFloat = false;
        const static bool isString = false;
      };

      template<> class TypeCharacteristics<u64>
      {
      public:
        const static bool isBasicType = true;
        const static bool isInteger = true;
        const static bool isSigned = false;
        const static bool isFloat = false;
        const static bool isString = false;
      };

      template<> class TypeCharacteristics<s64>
      {
      public:
        const static bool isBasicType = true;
        const static bool isInteger = true;
        const static bool isSigned = true;
        const static bool isFloat = false;
        const static bool isString = false;
      };

      template<> class TypeCharacteristics<f32>
      {
      public:
        const static bool isBasicType = true;
        const static bool isInteger = false;
        const static bool isSigned = true;
        const static bool isFloat = true;
        const static bool isString = false;
      };

      template<> class TypeCharacteristics<f64>
      {
      public:
        const static bool isBasicType = true;
        const static bool isInteger = false;
        const static bool isSigned = true;
        const static bool isFloat = true;
        const static bool isString = false;
      };

      template<> class TypeCharacteristics<char *>
      {
      public:
        const static bool isBasicType = false;
        const static bool isInteger = false;
        const static bool isSigned = false;
        const static bool isFloat = false;
        const static bool isString = true;
      };

      template<> class TypeCharacteristics<const char *>
      {
      public:
        const static bool isBasicType = false;
        const static bool isInteger = false;
        const static bool isSigned = false;
        const static bool isFloat = false;
        const static bool isString = true;
      };

      template<> class numeric_limits<f32>
      {
      public:
        static inline f32 epsilon() { return FLT_EPSILON; }
      };

      template<> class numeric_limits<f64>
      {
      public:
        static inline f64 epsilon() { return DBL_EPSILON; }
      };
    } // namespace Flags
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_FLAGS_DECLARATIONS_H_
