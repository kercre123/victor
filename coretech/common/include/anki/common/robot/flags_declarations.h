#ifndef _ANKICORETECHEMBEDDED_COMMON_FLAGS_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_FLAGS_DECLARATIONS_H_

#include "anki/common/robot/config.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Flags
    {
#pragma mark --- Declarations ---

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

      template<typename Type> class TypeCharacteristics
      {
      public:
        const static bool isBasicType = false;
        const static bool isInteger = false;
        const static bool isSigned = false;
        const static bool isFloat = false;
      };
    } // namespace Flags
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_FLAGS_DECLARATIONS_H_