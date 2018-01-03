/**
File: flags.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/robot/flags.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Flags
    {
      Buffer::Buffer()
        : flags(0)
      {
      }

      Buffer::Buffer(bool zeroAllocatedMemory, bool useBoundaryFillPatterns, bool isFullyAllocated)
        : flags(0)
      {
        this->set_zeroAllocatedMemory(zeroAllocatedMemory);
        this->set_useBoundaryFillPatterns(useBoundaryFillPatterns);
        this->set_isFullyAllocated(isFullyAllocated);
      }

      Buffer& Buffer::set_zeroAllocatedMemory(bool value)
      {
        this->flags &= ~Buffer::ZERO_ALLOCATED_MEMORY;

        if(value)
          this->flags |= Buffer::ZERO_ALLOCATED_MEMORY;

        return *this;
      }

      bool Buffer::get_zeroAllocatedMemory() const
      {
        return (this->flags & Buffer::ZERO_ALLOCATED_MEMORY) != 0;
      }

      Buffer& Buffer::set_useBoundaryFillPatterns(bool value)
      {
        this->flags &= ~Buffer::USE_BOUNDARY_FILL_PATTERNS;

        if(value)
          this->flags |= Buffer::USE_BOUNDARY_FILL_PATTERNS;

        return *this;
      }

      bool Buffer::get_useBoundaryFillPatterns() const
      {
        return (this->flags & Buffer::USE_BOUNDARY_FILL_PATTERNS) != 0;
      }

      Buffer& Buffer::set_isFullyAllocated(bool value)
      {
        this->flags &= ~Buffer::IS_FULLY_ALLOCATED;

        if(value)
          this->flags |= Buffer::IS_FULLY_ALLOCATED;

        return *this;
      }

      bool Buffer::get_isFullyAllocated() const
      {
        return (this->flags & Buffer::IS_FULLY_ALLOCATED) != 0;
      }

      Buffer& Buffer::set_rawFlags(u32 rawFlags)
      {
        this->flags = rawFlags;

        return *this;
      }

      u32 Buffer::get_rawFlags() const
      {
        return this->flags;
      }
    } // namespace Flags
  } // namespace Embedded
} // namespace Anki
