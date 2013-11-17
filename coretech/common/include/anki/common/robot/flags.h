#ifndef _ANKICORETECHEMBEDDED_COMMON_FLAGS_H_
#define _ANKICORETECHEMBEDDED_COMMON_FLAGS_H_

#include "anki/common/robot/config.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Flags
    {
#pragma mark --- Definitions ---
      class Buffer
      {
      public:
        Buffer();
        Buffer(bool zeroAllocatedMemory, bool useBoundaryFillPatterns);

        void set_zeroAllocatedMemory(bool value);
        bool get_zeroAllocatedMemory() const;

        void set_useBoundaryFillPatterns(bool value);
        bool get_useBoundaryFillPatterns() const;

        u32 get_rawFlags() const;

      protected:
        enum Flags
        {
          USE_BOUNDARY_FILL_PATTERNS = 1,
          ZERO_ALLOCATED_MEMORY = (1<<1)
        };

        u32 flags;
      };

      // This set of flags is computed automatically at compile time
      template<typename Type> class TypeCharacteristics
      {
      public:
        enum Characteristics { IS_FLOAT = 0, IS_INT = 0, IS_SIGNED = 0, IS_BASIC_TYPE = 0 };
      };

#pragma mark --- Implementations ---

#pragma mark --- Specializations ---

      template<> class TypeCharacteristics<bool>
      {
      public:
        enum Characteristics { IS_FLOAT = 0, IS_INT = 1, IS_SIGNED = 0, IS_BASIC_TYPE = 1 };
      };

      template<> class TypeCharacteristics<s8>
      {
      public:
        enum Characteristics { IS_FLOAT = 0, IS_INT = 1, IS_SIGNED = 1, IS_BASIC_TYPE = 1 };
      };

      template<> class TypeCharacteristics<u8>
      {
      public:
        enum Characteristics { IS_FLOAT = 0, IS_INT = 1, IS_SIGNED = 0, IS_BASIC_TYPE = 1 };
      };

      template<> class TypeCharacteristics<s16>
      {
      public:
        enum Characteristics { IS_FLOAT = 0, IS_INT = 1, IS_SIGNED = 1, IS_BASIC_TYPE = 1 };
      };

      template<> class TypeCharacteristics<u16>
      {
      public:
        enum Characteristics { IS_FLOAT = 0, IS_INT = 1, IS_SIGNED = 0, IS_BASIC_TYPE = 1 };
      };

      template<> class TypeCharacteristics<s32>
      {
      public:
        enum Characteristics { IS_FLOAT = 0, IS_INT = 1, IS_SIGNED = 1, IS_BASIC_TYPE = 1 };
      };

      template<> class TypeCharacteristics<u32>
      {
      public:
        enum Characteristics { IS_FLOAT = 0, IS_INT = 1, IS_SIGNED = 0, IS_BASIC_TYPE = 1 };
      };

      template<> class TypeCharacteristics<s64>
      {
      public:
        enum Characteristics { IS_FLOAT = 0, IS_INT = 1, IS_SIGNED = 1, IS_BASIC_TYPE = 1 };
      };

      template<> class TypeCharacteristics<u64>
      {
      public:
        enum Characteristics { IS_FLOAT = 0, IS_INT = 1, IS_SIGNED = 0, IS_BASIC_TYPE = 1 };
      };

      template<> class TypeCharacteristics<f32>
      {
      public:
        enum Characteristics { IS_FLOAT = 1, IS_INT = 0, IS_SIGNED = 1, IS_BASIC_TYPE = 1 };
      };

      template<> class TypeCharacteristics<f64>
      {
      public:
        enum Characteristics { IS_FLOAT = 1, IS_INT = 0, IS_SIGNED = 1, IS_BASIC_TYPE = 1 };
      };
    } // namespace Flags
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_FLAGS_H_