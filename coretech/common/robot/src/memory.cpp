/**
File: memory.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/memory.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/utilities.h"

namespace Anki
{
  namespace Embedded
  {
    MemoryStack::MemoryStack(void *buffer, s32 bufferLength, Flags::Buffer flags)
      : buffer(buffer), totalBytes(bufferLength), usedBytes(0), usedBytesBeforeLastAllocation(0), lastAllocatedMemory(NULL), flags(flags)
    {
      AnkiAssert(flags.get_useBoundaryFillPatterns());

      static s32 maxId = 0;

      this->id = maxId;
      maxId++;

      AnkiConditionalError(buffer, "Anki.MemoryStack.MemoryStack", "Buffer must be allocated");
      AnkiConditionalError(bufferLength <= 0x3FFFFFFF, "Anki.MemoryStack.MemoryStack", "Maximum size of a MemoryStack is 2^30 - 1");
      AnkiConditionalError(MEMORY_ALIGNMENT == 16, "Anki.MemoryStack.MemoryStack", "Currently, only MEMORY_ALIGNMENT == 16 is supported");
    }

    MemoryStack::MemoryStack(const MemoryStack& ms)
      : buffer(ms.buffer), totalBytes(ms.totalBytes), usedBytes(ms.usedBytes), usedBytesBeforeLastAllocation(ms.usedBytesBeforeLastAllocation), lastAllocatedMemory(ms.lastAllocatedMemory), id(ms.id), flags(ms.flags)
    {
      AnkiConditionalWarn(ms.buffer, "Anki.MemoryStack.MemoryStack", "Buffer must be allocated");
      AnkiConditionalWarn(ms.totalBytes <= 0x3FFFFFFF, "Anki.MemoryStack.MemoryStack", "Maximum size of a MemoryStack is 2^30 - 1");
      AnkiConditionalWarn(MEMORY_ALIGNMENT == 16, "Anki.MemoryStack.MemoryStack", "Currently, only MEMORY_ALIGNMENT == 16 is supported");
      AnkiConditionalWarn(ms.totalBytes >= ms.usedBytes, "Anki.MemoryStack.MemoryStack", "Buffer is using more bytes than it has. Try running IsValid() to test for memory corruption.");
    }

    void* MemoryStack::Allocate(s32 numBytesRequested, s32 *numBytesAllocated)
    {
      if(numBytesAllocated)
        *numBytesAllocated = 0;

      AnkiConditionalErrorAndReturnValue(numBytesRequested > 0, NULL, "Anki.MemoryStack.Allocate", "numBytesRequested > 0");
      AnkiConditionalErrorAndReturnValue(numBytesRequested <= 0x3FFFFFFF, NULL, "Anki.MemoryStack.Allocate", "numBytesRequested <= 0x3FFFFFFF");

      char * const bufferNextFree = static_cast<char*>(buffer) + usedBytes;

      // Get the pointer locations for header, data, and footer
      // The header doesn't have to be aligned, but it should be right before the start of the segmentMemory
      // The footer is aligned for free, because the allocated memory starts aligned, and has an aligned stride
      char * const segmentMemory = reinterpret_cast<char*>( RoundUp<size_t>(reinterpret_cast<size_t>(bufferNextFree)+HEADER_LENGTH, MEMORY_ALIGNMENT) );
      u32  * const segmentHeader = reinterpret_cast<u32*> ( reinterpret_cast<size_t>(segmentMemory) - HEADER_LENGTH );
      const u32 numBytesRequestedRounded = RoundUp<u32>(numBytesRequested, MEMORY_ALIGNMENT);
      u32  * const segmentFooter = reinterpret_cast<u32*> (segmentMemory+numBytesRequestedRounded);

      const s32 requestedBytes = static_cast<s32>( reinterpret_cast<size_t>(segmentFooter) + FOOTER_LENGTH - reinterpret_cast<size_t>(bufferNextFree) );

      AnkiConditionalErrorAndReturnValue((usedBytes+requestedBytes) <= totalBytes, NULL, "Anki.MemoryStack.Allocate", "Ran out of scratch space");

      // Is this possible?
      AnkiConditionalErrorAndReturnValue(static_cast<u32>(reinterpret_cast<size_t>(segmentFooter) - reinterpret_cast<size_t>(segmentMemory)) == numBytesRequestedRounded,
        NULL, "Anki.MemoryStack.Allocate", "Odd error");

      // Next, add the header for this block
      segmentHeader[0] = numBytesRequestedRounded;
      segmentHeader[1] = FILL_PATTERN_START;
      segmentFooter[0] = FILL_PATTERN_END;

      // For Reallocate()
      usedBytesBeforeLastAllocation = usedBytes;
      lastAllocatedMemory = segmentMemory;

      usedBytes += requestedBytes;

      if(numBytesAllocated) {
        *numBytesAllocated = numBytesRequestedRounded;
      }

      if(flags.get_zeroAllocatedMemory())
        memset(segmentMemory, 0, numBytesRequestedRounded);

      return segmentMemory;
    }

    void* MemoryStack::Reallocate(void* memoryLocation, s32 numBytesRequested, s32 *numBytesAllocated)
    {
      if(numBytesAllocated)
        *numBytesAllocated = 0;

      AnkiConditionalErrorAndReturnValue(memoryLocation == lastAllocatedMemory, NULL, "Anki.MemoryStack.Reallocate", "The requested memory is not at the end of the stack");

      // Don't clear the reallocated memory
      const bool clearMemory = this->flags.get_zeroAllocatedMemory();
      this->flags.set_zeroAllocatedMemory(false);

      this->usedBytes = usedBytesBeforeLastAllocation;

      void *segmentMemory = Allocate(numBytesRequested, numBytesAllocated);

      this->flags.set_zeroAllocatedMemory(clearMemory);

      return segmentMemory;
    }

    bool MemoryStack::IsValid() const
    {
      AnkiConditionalErrorAndReturnValue(buffer != NULL, false, "Anki.MemoryStack.IsValid", "buffer is not allocated");

      AnkiConditionalWarnAndReturnValue(usedBytes <= totalBytes, false, "Anki.MemoryStack.IsValid", "usedBytes is greater than totalBytes");
      AnkiConditionalWarnAndReturnValue(usedBytes >= 0 && totalBytes >= 0, false, "Anki.MemoryStack.IsValid", "usedBytes or totalBytes is less than zero");

      if(usedBytes == 0)
        return true;

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS
      const size_t LOOP_MAX = 1000000;
      const char * const bufferCharStar = reinterpret_cast<const char*>(buffer);
      const size_t bufferSizeT = reinterpret_cast<size_t>(buffer);

      s32 index = static_cast<s32>( RoundUp<size_t>(bufferSizeT+HEADER_LENGTH, MEMORY_ALIGNMENT) - HEADER_LENGTH - bufferSizeT );

      for(size_t i=0; (i<MIN(LOOP_MAX,totalBytes)) && (index<usedBytes); i++) {
        index = static_cast<s32>( RoundUp<size_t>(bufferSizeT+index+HEADER_LENGTH, MEMORY_ALIGNMENT) - HEADER_LENGTH - bufferSizeT );

        // A segment's size should only be multiples of MEMORY_ALIGNMENT, but check to be sure
        const s32 segmentLength = reinterpret_cast<const u32*>(bufferCharStar+index)[0];
        const s32 roundedSegmentLength = RoundUp<s32>(segmentLength, MEMORY_ALIGNMENT);
        AnkiConditionalWarnAndReturnValue(segmentLength == roundedSegmentLength, false, "Anki.MemoryStack.IsValid", "The segmentLength is not a multiple of MEMORY_ALIGNMENT");

        // Check if the segment end is beyond the end of the buffer (NOTE: this is not conservative enough, though errors should be caught later)
        AnkiConditionalWarnAndReturnValue(segmentLength <= (usedBytes-index-HEADER_LENGTH-FOOTER_LENGTH), false, "Anki.MemoryStack.IsValid", "The segment end is beyond the end of the buffer");

        const u32 segmentHeader = reinterpret_cast<const u32*>(bufferCharStar+index)[1];

        AnkiConditionalWarnAndReturnValue(segmentHeader == FILL_PATTERN_START, false, "Anki.MemoryStack.IsValid", "segmentHeader == FILL_PATTERN_START");

        const u32 segmentFooter = reinterpret_cast<const u32*>(bufferCharStar+index+HEADER_LENGTH+segmentLength)[0];

        AnkiConditionalWarnAndReturnValue(segmentFooter == FILL_PATTERN_END, false, "Anki.MemoryStack.IsValid", "segmentFooter == FILL_PATTERN_END");

        index += HEADER_LENGTH + segmentLength + FOOTER_LENGTH;
      }

      if(index == usedBytes) {
        return true;
      } else if(index == LOOP_MAX){
        AnkiError("Anki.MemoryStack.IsValid", "Infinite while loop");
        return false;
      } else {
        AnkiError("Anki.MemoryStack.IsValid", "Loop exited at an incorrect position, probably due to corruption");
        return false;
      }
#endif // #if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS
      return true;
    }

    s32 MemoryStack::ComputeLargestPossibleAllocation() const
    {
      const size_t bufferNextFree = reinterpret_cast<size_t>(buffer) + usedBytes;
      const size_t bufferNextFreePlusHeaderAndAlignment = RoundUp<size_t>(bufferNextFree+HEADER_LENGTH, MEMORY_ALIGNMENT);
      const size_t bufferEnd = reinterpret_cast<size_t>(buffer) + totalBytes;

      // Make sure the next start point isn't past the end of the buffer (done in this way because of unsigned arithmetic)
      if( (bufferNextFreePlusHeaderAndAlignment+FOOTER_LENGTH+MEMORY_ALIGNMENT) > bufferEnd )
        return 0;

      // The RoundDown handles the requirement for all memory blocks to be multiples of MEMORY_ALIGNMENT
      const s32 maxFreeSpace = static_cast<s32>( RoundDown<size_t>(bufferEnd - bufferNextFreePlusHeaderAndAlignment - FOOTER_LENGTH, MEMORY_ALIGNMENT) );

      return maxFreeSpace;
    }

    Result MemoryStack::Print() const
    {
      const s32 maxAllocationBytes = ComputeLargestPossibleAllocation();
      printf("(id:%d totalBytes:%d usedBytes:%d maxAllocationBytes:%d bufferLocation:%d) ", id, totalBytes, usedBytes, maxAllocationBytes, buffer);
      return RESULT_OK;
    }

    s32 MemoryStack::get_totalBytes() const
    {
      return totalBytes;
    }

    s32 MemoryStack::get_usedBytes() const
    {
      return usedBytes;
    }

    void* MemoryStack::get_buffer()
    {
      return buffer;
    }

    const void* MemoryStack::get_buffer() const
    {
      return buffer;
    }

    s32 MemoryStack::get_id() const
    {
      return id;
    }

    Flags::Buffer MemoryStack::get_flags() const
    {
      return flags;
    }

    MemoryStackConstIterator::MemoryStackConstIterator(const MemoryStack &memory)
      : memory(memory)
    {
      const size_t bufferSizeT = reinterpret_cast<size_t>(memory.buffer);

      this->index = static_cast<s32>( RoundUp<size_t>(bufferSizeT+MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH - bufferSizeT );
    }

    bool MemoryStackConstIterator::HasNext() const
    {
      if(this->index < memory.get_usedBytes())
        return true;
      else
        return false;
    }

    const void * MemoryStackConstIterator::GetNext(s32 &segmentLength)
    {
      segmentLength = 0;

      if(!this->HasNext()) {
        return NULL;
      }

      const char * const bufferCharStar = reinterpret_cast<const char*>(memory.buffer);
      const size_t bufferSizeT = reinterpret_cast<size_t>(memory.buffer);

      // Get the start of the next valid segment
      this->index = static_cast<s32>( RoundUp<size_t>(bufferSizeT+this->index+MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH - bufferSizeT );

      // A segment's size should only be multiples of MEMORY_ALIGNMENT, but check to be sure
      segmentLength = reinterpret_cast<const u32*>(bufferCharStar+this->index)[0];
      const s32 roundedSegmentLength = RoundUp<s32>(segmentLength, MEMORY_ALIGNMENT);

      AnkiConditionalErrorAndReturnValue(segmentLength == roundedSegmentLength, NULL, "Anki.MemoryStackConstIterator.GetNext", "The segmentLength is not a multiple of MEMORY_ALIGNMENT");

      // Check if the segment end is beyond the end of the buffer (NOTE: this is not conservative enough, though errors should be caught later)
      AnkiConditionalErrorAndReturnValue(segmentLength <= (memory.usedBytes-this->index-MemoryStack::HEADER_LENGTH-MemoryStack::FOOTER_LENGTH), NULL, "Anki.MemoryStackConstIterator.GetNext", "The segment end is beyond the end of the buffer");

      const u32 segmentHeader = reinterpret_cast<const u32*>(bufferCharStar+this->index)[1];

      AnkiConditionalErrorAndReturnValue(segmentHeader == MemoryStack::FILL_PATTERN_START, NULL, "Anki.MemoryStackConstIterator.GetNext", "segmentHeader == FILL_PATTERN_START");

      const u32 segmentFooter = reinterpret_cast<const u32*>(bufferCharStar+this->index+MemoryStack::HEADER_LENGTH+segmentLength)[0];

      AnkiConditionalErrorAndReturnValue(segmentFooter == MemoryStack::FILL_PATTERN_END, NULL, "Anki.MemoryStackConstIterator.GetNext", "segmentFooter == FILL_PATTERN_END");

      const void * segmentToReturn = reinterpret_cast<const void*>(bufferCharStar + this->index + MemoryStack::HEADER_LENGTH);

      this->index += MemoryStack::HEADER_LENGTH + segmentLength + MemoryStack::FOOTER_LENGTH;

      return segmentToReturn;
    }

    MemoryStackIterator::MemoryStackIterator(MemoryStack &memory)
      : MemoryStackConstIterator(memory)
    {
    }

    void * MemoryStackIterator::GetNext(s32 &segmentLength)
    {
      // To avoid code duplication, we'll use the const version of GetNext(), though our MemoryStack is not const

      const void * segment = MemoryStackConstIterator::GetNext(segmentLength);

      return const_cast<void*>(segment);
    }
  } // namespace Embedded
} // namespace Anki