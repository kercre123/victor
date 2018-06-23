/**
File: memory.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/
#include <string.h>
#include <assert.h>

#include "coretech/common/robot/memory.h"
#include "coretech/common/robot/errorHandling.h"
#include "coretech/common/robot/utilities.h"

#include "coretech/common/shared/utilities_shared.h"

//#define DISPLAY_USED_BYTES

namespace Anki
{
  namespace Embedded
  {
    MemoryStack::MemoryStack()
      : buffer(NULL)
    {
    }

    MemoryStack::MemoryStack(void *buffer, s32 bufferLength, Flags::Buffer flags)
      : buffer(buffer), totalBytes(bufferLength), usedBytes(0), usedBytesBeforeLastAllocation(0), lastAllocatedMemory(NULL), flags(flags)
    {
      AnkiAssert(flags.get_useBoundaryFillPatterns());

      if(flags.get_isFullyAllocated()) {
        AnkiConditionalErrorAndReturn((reinterpret_cast<size_t>(buffer)+MemoryStack::HEADER_LENGTH)%MEMORY_ALIGNMENT == 0,
          "MemoryStack::MemoryStack", "If fully allocated, the %dth byte of the buffer must be %d byte aligned", MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT);

        this->usedBytes = this->totalBytes;
      }

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

    void* MemoryStack::Allocate(const s32 numBytesRequested)
    {
      s32 numBytesAllocated = -1;
      return this->Allocate(numBytesRequested, numBytesAllocated);
    }

    void* MemoryStack::Allocate(s32 numBytesRequested, s32 &numBytesAllocated)
    {
      return this->Allocate(numBytesRequested, this->get_flags().get_zeroAllocatedMemory(), numBytesAllocated);
    }

    void* MemoryStack::Allocate(const s32 numBytesRequested, const bool zeroAllocatedMemory, s32 &numBytesAllocated)
    {
      numBytesAllocated = 0;

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

      AnkiConditionalErrorAndReturnValue((usedBytes+requestedBytes) <= totalBytes, NULL, "Anki.MemoryStack.Allocate",
                                         "Ran out of scratch space, requesting %d bytes, with %d of %d used",
                                         requestedBytes, usedBytes, totalBytes);

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

      numBytesAllocated = numBytesRequestedRounded;

      if(zeroAllocatedMemory)
        memset(segmentMemory, 0, numBytesRequestedRounded);

#ifdef DISPLAY_USED_BYTES
      CoreTechPrint("%d) Used %d bytes\n", id, usedBytes);
#endif

      return segmentMemory;
    }

    void* MemoryStack::Reallocate(void* memoryLocation, s32 numBytesRequested)
    {
      s32 numBytesAllocated = -1;
      return MemoryStack::Reallocate(memoryLocation, numBytesRequested, numBytesAllocated);
    }

    void* MemoryStack::Reallocate(void* memoryLocation, s32 numBytesRequested, s32 &numBytesAllocated)
    {
      numBytesAllocated = 0;

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
      CoreTechPrint("(id:%d totalBytes:%d usedBytes:%d maxAllocationBytes:%d bufferLocation:%p) ", id, totalBytes, usedBytes, maxAllocationBytes, buffer);
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

    void* MemoryStack::get_validBufferStart()
    {
      s32 firstValidIndex;
      return this->get_validBufferStart(firstValidIndex);
    }

    const void* MemoryStack::get_validBufferStart() const
    {
      s32 firstValidIndex;
      return this->get_validBufferStart(firstValidIndex);
    }

    void* MemoryStack::get_validBufferStart(s32 &firstValidIndex)
    {
      const size_t bufferSizeT = reinterpret_cast<size_t>(this->buffer);
      firstValidIndex = static_cast<s32>( RoundUp<size_t>(bufferSizeT+MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH - bufferSizeT );

      void * validStart = reinterpret_cast<char*>(this->buffer) + firstValidIndex;

      return validStart;
    }

    const void* MemoryStack::get_validBufferStart(s32 &firstValidIndex) const
    {
      const size_t bufferSizeT = reinterpret_cast<size_t>(this->buffer);
      firstValidIndex = static_cast<s32>( RoundUp<size_t>(bufferSizeT+MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH - bufferSizeT );

      const void * validStart = reinterpret_cast<const char*>(this->buffer) + firstValidIndex;

      return validStart;
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
      const size_t bufferSizeT = reinterpret_cast<size_t>(memory.get_buffer());

      this->index = static_cast<s32>( RoundUp<size_t>(bufferSizeT+MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH - bufferSizeT );
    }

    bool MemoryStackConstIterator::HasNext() const
    {
      // TODO: These extra bytes are a bit of a hack for the SerializedBuffer case.
      // I think index should match the used bytes exactly, but seems to be a bit short.
      s32 extraBytes = MEMORY_ALIGNMENT;
      if(this->memory.get_flags().get_useBoundaryFillPatterns()) {
        extraBytes += MemoryStack::HEADER_LENGTH + MemoryStack::FOOTER_LENGTH;
      }

      if((this->index + extraBytes) < memory.get_usedBytes())
        return true;
      else
        return false;
    }

    const void * MemoryStackConstIterator::GetNext(s32 &segmentLength, const bool requireFillPatternMatch)
    {
      segmentLength = 0;

      if(!this->HasNext()) {
        return NULL;
      }

      const char * const bufferCharStar = reinterpret_cast<const char*>(memory.get_buffer());
      const size_t bufferSizeT = reinterpret_cast<size_t>(memory.get_buffer());

      // Get the start of the next valid segment
      this->index = static_cast<s32>( RoundUp<size_t>(bufferSizeT+this->index+MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH - bufferSizeT );

      // A segment's size should only be multiples of MEMORY_ALIGNMENT, but check to be sure
      segmentLength = reinterpret_cast<const u32*>(bufferCharStar+this->index)[0];
      const s32 roundedSegmentLength = RoundUp<s32>(segmentLength, MEMORY_ALIGNMENT);

      AnkiConditionalErrorAndReturnValue(segmentLength == roundedSegmentLength, NULL, "Anki.MemoryStackConstIterator.GetNext", "The segmentLength is not a multiple of MEMORY_ALIGNMENT (%x!=%x)", segmentLength, roundedSegmentLength);

      // Check if the segment end is beyond the end of the buffer (NOTE: this is not conservative enough, though errors should be caught later)
      AnkiConditionalErrorAndReturnValue(segmentLength <= (memory.get_usedBytes()-this->index-MemoryStack::HEADER_LENGTH-MemoryStack::FOOTER_LENGTH), NULL, "Anki.MemoryStackConstIterator.GetNext", "The segment end is beyond the end of the buffer. segmentLength=%d (0x%x) usedBytes=%d all=%d", segmentLength, segmentLength, memory.get_usedBytes(), (memory.get_usedBytes()-this->index-MemoryStack::HEADER_LENGTH-MemoryStack::FOOTER_LENGTH));

      if(requireFillPatternMatch) {
        const u32 segmentHeader = reinterpret_cast<const u32*>(bufferCharStar+this->index)[1];

        AnkiConditionalErrorAndReturnValue(segmentHeader == MemoryStack::FILL_PATTERN_START, NULL, "Anki.MemoryStackConstIterator.GetNext", "segmentHeader == FILL_PATTERN_START (%x!=%x)", segmentHeader, MemoryStack::FILL_PATTERN_START);

        const u32 segmentFooter = reinterpret_cast<const u32*>(bufferCharStar+this->index+MemoryStack::HEADER_LENGTH+segmentLength)[0];

        AnkiConditionalErrorAndReturnValue(segmentFooter == MemoryStack::FILL_PATTERN_END, NULL, "Anki.MemoryStackConstIterator.GetNext", "segmentFooter == FILL_PATTERN_END (%x != %x)", segmentFooter, MemoryStack::FILL_PATTERN_END);
      }

      const void * segmentToReturn = reinterpret_cast<const void*>(bufferCharStar + this->index + MemoryStack::HEADER_LENGTH);

      this->index += MemoryStack::HEADER_LENGTH + segmentLength + MemoryStack::FOOTER_LENGTH;

      return segmentToReturn;
    }

    const MemoryStack& MemoryStackConstIterator::get_memory() const
    {
      return memory;
    }

    MemoryStackIterator::MemoryStackIterator(MemoryStack &memory)
      : MemoryStackConstIterator(memory)
    {
    }

    void * MemoryStackIterator::GetNext(s32 &segmentLength, const bool requireFillPatternMatch)
    {
      // To avoid code duplication, we'll use the const version of GetNext(), though our MemoryStack is not const

      const void * segment = MemoryStackConstIterator::GetNext(segmentLength, requireFillPatternMatch);

      return const_cast<void*>(segment);
    }

    MemoryStack& MemoryStackIterator::get_memory()
    {
      return const_cast<MemoryStack&>(memory);
    }

    MemoryStackReconstructingConstIterator::MemoryStackReconstructingConstIterator(const MemoryStack &memory)
      : memory(memory)
    {
      this->index = 0;
    }

    bool MemoryStackReconstructingConstIterator::HasNext() const
    {
      s32 startIndex;
      s32 endIndex;
      s32 reportedLength;

      return HasNext(startIndex, endIndex, reportedLength);
    }

    bool MemoryStackReconstructingConstIterator::HasNext(s32 &startIndex, s32 &endIndex, s32 &reportedLength) const
    {
      const u8 * curBufferPointer = reinterpret_cast<const u8*>(memory.get_buffer()) + this->index;
      const s32 curBufferLength = memory.get_usedBytes() - this->index;

      // These must be copied, or the Keil compiler gets confused
      u32 fillPatternStartU32 = MemoryStack::FILL_PATTERN_START;
      u32 fillPatternEndU32 = MemoryStack::FILL_PATTERN_END;

      const u8 * fillPatternStartU8p = reinterpret_cast<const u8*>(&fillPatternStartU32);
      const u8 * fillPatternEndU8p = reinterpret_cast<const u8*>(&fillPatternEndU32);

      startIndex = FindBytePattern(curBufferPointer, curBufferLength, fillPatternStartU8p, sizeof(u32));
      endIndex = FindBytePattern(curBufferPointer+startIndex, curBufferLength-startIndex, fillPatternEndU8p, sizeof(u32));

      reportedLength = -1;

      if(startIndex >= 0) {
        reportedLength = *reinterpret_cast<const u32*>(curBufferPointer + startIndex - sizeof(u32));

        if(endIndex >= 0) {
          endIndex += this->index + startIndex - 1;
          startIndex += this->index + sizeof(u32);

          return true;
        } else {
          startIndex += this->index + sizeof(u32);
        }
      }

      return false;
    }

    const void * MemoryStackReconstructingConstIterator::GetNext(s32 &trueSegmentLength, s32 &reportedSegmentLength)
    {
      s32 startIndex;
      s32 endIndex;

      const bool hasNext = HasNext(startIndex, endIndex, reportedSegmentLength);

      trueSegmentLength = endIndex - startIndex + 1;

      if(!hasNext)
        return NULL;

      const u8 * bufferCharStar = reinterpret_cast<const u8*>(this->memory.get_buffer());

      const void * segmentToReturn = reinterpret_cast<const void*>(bufferCharStar + startIndex);

      this->index = endIndex + 2*sizeof(u32);

      return segmentToReturn;
    }

    const MemoryStack& MemoryStackReconstructingConstIterator::get_memory() const
    {
      return memory;
    }

    MemoryStackReconstructingIterator::MemoryStackReconstructingIterator(MemoryStack &memory)
      : MemoryStackReconstructingConstIterator(memory)
    {
    }

    void * MemoryStackReconstructingIterator::GetNext(s32 &trueSegmentLength, s32 &reportedSegmentLength)
    {
      // To avoid code duplication, we'll use the const version of GetNext(), though our MemoryStack is not const

      const void * segment = MemoryStackReconstructingConstIterator::GetNext(trueSegmentLength, reportedSegmentLength);

      return const_cast<void*>(segment);
    }

    MemoryStack& MemoryStackReconstructingIterator::get_memory()
    {
      return const_cast<MemoryStack&>(memory);
    }
  } // namespace Embedded
} // namespace Anki
