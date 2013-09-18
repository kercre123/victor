#include "anki/embeddedCommon.h"

#include <assert.h>

namespace Anki
{
  namespace Embedded
  {
    IN_DDR MemoryStack::MemoryStack(void *buffer, s32 bufferLength)
      : buffer(buffer), totalBytes(bufferLength), usedBytes(0)
    {
      AnkiConditionalEssentialAndReturn(buffer, "Anki.MemoryStack.MemoryStack", "Buffer must be allocated");
      AnkiConditionalEssentialAndReturn(bufferLength <= 0x3FFFFFFF, "Anki.MemoryStack.MemoryStack", "Maximum size of a MemoryStack is 2^30 - 1");
      AnkiConditionalError(MEMORY_ALIGNMENT == 16, "Anki.MemoryStack.MemoryStack", "Currently, only MEMORY_ALIGNMENT == 16 is supported");
    }

    IN_DDR MemoryStack::MemoryStack(const MemoryStack& ms)
      : buffer(ms.buffer), totalBytes(ms.totalBytes), usedBytes(ms.usedBytes)
    {
      AnkiConditionalEssentialAndReturn(ms.buffer, "Anki.MemoryStack.MemoryStack", "Buffer must be allocated");
      AnkiConditionalEssentialAndReturn(ms.totalBytes <= 0x3FFFFFFF, "Anki.MemoryStack.MemoryStack", "Maximum size of a MemoryStack is 2^30 - 1");
      AnkiConditionalError(MEMORY_ALIGNMENT == 16, "Anki.MemoryStack.MemoryStack", "Currently, only MEMORY_ALIGNMENT == 16 is supported");
      AnkiConditionalError(ms.totalBytes >= ms.usedBytes, "Anki.MemoryStack.MemoryStack", "Buffer is using more bytes than it has. Try running IsValid() to test for memory corruption.");
    }

    IN_DDR void* MemoryStack::Allocate(s32 numBytesRequested, s32 *numBytesAllocated)
    {
      AnkiConditionalEssentialAndReturnValue(numBytesRequested > 0, NULL, "Anki.MemoryStack.Allocate", "numBytesRequested > 0");
      AnkiConditionalEssentialAndReturnValue(numBytesRequested <= 0x3FFFFFFF, NULL, "Anki.MemoryStack.Allocate", "numBytesRequested <= 0x3FFFFFFF");

      char * const bufferNextFree = static_cast<char*>(buffer) + usedBytes;

      // Get the pointer locations for header, data, and footer
      // The header doesn't have to be aligned, but it should be right before the start of the segmentMemory
      // The footer is aligned for free, because the allocated memory starts aligned, and has an aligned stride
      char * const segmentMemory = reinterpret_cast<char*>( RoundUp<size_t>(reinterpret_cast<size_t>(bufferNextFree)+HEADER_LENGTH, MEMORY_ALIGNMENT) );
      u32  * const segmentHeader = reinterpret_cast<u32*> ( reinterpret_cast<size_t>(segmentMemory) - HEADER_LENGTH );
      const u32 numBytesRequestedRounded = RoundUp<u32>(numBytesRequested, MEMORY_ALIGNMENT);
      u32  * const segmentFooter = reinterpret_cast<u32*> (segmentMemory+numBytesRequestedRounded);

      const s32 requestedBytes = static_cast<s32>( reinterpret_cast<size_t>(segmentFooter) + FOOTER_LENGTH - reinterpret_cast<size_t>(bufferNextFree) );

      AnkiConditionalEssentialAndReturnValue((usedBytes+requestedBytes) <= totalBytes, NULL, "Anki.MemoryStack.Allocate", "Ran out of scratch space");

      // Next, add the header for this block
      segmentHeader[0] = numBytesRequestedRounded;
      segmentHeader[1] = FILL_PATTERN_START;
      segmentFooter[0] = FILL_PATTERN_END;

      usedBytes += requestedBytes;

      assert(static_cast<s32>(reinterpret_cast<size_t>(segmentFooter) - reinterpret_cast<size_t>(segmentMemory)) == numBytesRequestedRounded);

      if(numBytesAllocated) {
        *numBytesAllocated = numBytesRequestedRounded;
      }

      // TODO: if this is slow, make this optional (or just remove it)
      memset(segmentMemory, 0, numBytesRequestedRounded);

      return segmentMemory;
    }

    IN_DDR bool MemoryStack::IsValid()
    {
      const size_t LOOP_MAX = 1000000;
      const char * const bufferCharStar = reinterpret_cast<const char*>(buffer);
      const size_t bufferSizeT = reinterpret_cast<size_t>(buffer);

      AnkiConditionalWarnAndReturnValue(buffer != NULL, false, "Anki.MemoryStack.IsValid", "buffer is not allocated");

#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
      AnkiConditionalWarnAndReturnValue(usedBytes <= totalBytes, false, "Anki.MemoryStack.IsValid", "usedBytes is greater than totalBytes");
      AnkiConditionalWarnAndReturnValue(usedBytes >= 0 && totalBytes >= 0, false, "Anki.MemoryStack.IsValid", "usedBytes or totalBytes is less than zero");
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH

      if(usedBytes == 0)
        return true;

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_LOW

      s32 index = static_cast<s32>( RoundUp<size_t>(bufferSizeT+HEADER_LENGTH, MEMORY_ALIGNMENT) - HEADER_LENGTH - bufferSizeT );

      for(size_t i=0; (i<MIN(LOOP_MAX,totalBytes)) && (index<usedBytes); i++) {
        index = static_cast<s32>( RoundUp<size_t>(bufferSizeT+index+HEADER_LENGTH, MEMORY_ALIGNMENT) - HEADER_LENGTH - bufferSizeT );

        // A segment's size should only be multiples of MEMORY_ALIGNMENT, but even on the off
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
#endif // #if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_LOW
      return true;
    }

    IN_DDR s32 MemoryStack::ComputeLargestPossibleAllocation()
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

    IN_DDR s32 MemoryStack::get_totalBytes()
    {
      return totalBytes;
    }

    IN_DDR s32 MemoryStack::get_usedBytes()
    {
      return usedBytes;
    }

    IN_DDR void* MemoryStack::get_buffer()
    {
      return buffer;
    }

    IN_DDR const void* MemoryStack::get_buffer() const
    {
      return buffer;
    }

    // Not sure if these should be supported. But I'm leaving them here for the time being.
    /*
    void MemoryStack::Clear()
    {
    usedBytes = 0;
    }
    */
  } // namespace Embedded
} // namespace Anki