#include "anki/common.h"

#include <assert.h>

Anki::MemoryStack::MemoryStack(void *buffer, u32 bufferLength)
  : buffer(buffer), totalBytes(bufferLength), usedBytes(0)
{
  DASConditionalError(buffer, "Anki.MemoryStack.MemoryStack", "Buffer must be allocated");
  DASConditionalError(bufferLength <= 0x3FFFFFFF, "Anki.MemoryStack.MemoryStack", "Maximum size of a MemoryStack is 2^30 - 1");
  DASConditionalError(MEMORY_ALIGNMENT == 16, "Anki.MemoryStack.MemoryStack", "Currently, only MEMORY_ALIGNMENT == 16 is supported");
}

Anki::MemoryStack::MemoryStack(const MemoryStack& ms)
  : buffer(ms.buffer), totalBytes(ms.totalBytes), usedBytes(ms.usedBytes)
{
  DASConditionalError(ms.buffer, "Anki.MemoryStack.MemoryStack", "Buffer must be allocated");
  DASConditionalError(ms.totalBytes <= 0x3FFFFFFF, "Anki.MemoryStack.MemoryStack", "Maximum size of a MemoryStack is 2^30 - 1");
  DASConditionalError(MEMORY_ALIGNMENT == 16, "Anki.MemoryStack.MemoryStack", "Currently, only MEMORY_ALIGNMENT == 16 is supported");
  DASConditionalError(ms.totalBytes >= ms.usedBytes, "Anki.MemoryStack.MemoryStack", "Buffer is using more bytes than it has. Try running IsValid() to test for memory corruption.");
}

void* Anki::MemoryStack::Allocate(u32 numBytesRequested, u32 *numBytesAllocated)
{
  DASConditionalWarnAndReturn(numBytesRequested != 0, NULL, "Anki.MemoryStack.Allocate", "numBytesRequested != 0");
  DASConditionalWarnAndReturn(numBytesRequested <= 0x3FFFFFFF, NULL, "Anki.MemoryStack.Allocate", "numBytesRequested <= 0x3FFFFFFF");

  char * const bufferNextFree = static_cast<char*>(buffer) + usedBytes;

  // Get the pointer locations for header, data, and footer
  // The header doesn't have to be aligned, but it should be right before the start of the segmentMemory
  // The footer is aligned for free, because the allocated memory starts aligned, and has an aligned stride
  char * const segmentMemory = reinterpret_cast<char*>( Anki::RoundUp<size_t>(reinterpret_cast<size_t>(bufferNextFree)+HEADER_LENGTH, MEMORY_ALIGNMENT) );
  u32  * const segmentHeader = reinterpret_cast<u32*> ( reinterpret_cast<size_t>(segmentMemory) - HEADER_LENGTH );
  const u32 numBytesRequestedRounded = Anki::RoundUp<u32>(numBytesRequested, MEMORY_ALIGNMENT);
  u32  * const segmentFooter = reinterpret_cast<u32*> (segmentMemory+numBytesRequestedRounded);

  const u32 requestedBytes = static_cast<u32>( reinterpret_cast<size_t>(segmentFooter) + FOOTER_LENGTH - reinterpret_cast<size_t>(bufferNextFree) );

  DASConditionalEventAndReturn((usedBytes+requestedBytes) <= totalBytes, NULL, "Anki.MemoryStack.Allocate", "Ran out of scratch space");

  // Next, add the header for this block
  segmentHeader[0] = numBytesRequestedRounded;
  segmentHeader[1] = FILL_PATTERN_START;
  segmentFooter[0] = FILL_PATTERN_END;

  usedBytes += requestedBytes;

  assert(static_cast<u32>(reinterpret_cast<size_t>(segmentFooter) - reinterpret_cast<size_t>(segmentMemory)) == numBytesRequestedRounded);

  if(numBytesAllocated) {
    *numBytesAllocated = numBytesRequestedRounded;
  }

  return segmentMemory;
}

bool Anki::MemoryStack::IsValid()
{
  const size_t LOOP_MAX = 1000000;
  const char * const bufferCharStar = reinterpret_cast<const char*>(buffer);
  const size_t bufferSizeT = reinterpret_cast<size_t>(buffer);

  u32 index = static_cast<u32>( Anki::RoundUp<size_t>(bufferSizeT+HEADER_LENGTH, MEMORY_ALIGNMENT) - HEADER_LENGTH - bufferSizeT );

  for(size_t i=0; (i<MIN(LOOP_MAX,totalBytes)) && (index<usedBytes); i++) {
    index = static_cast<u32>( Anki::RoundUp<size_t>(bufferSizeT+index+HEADER_LENGTH, MEMORY_ALIGNMENT) - HEADER_LENGTH - bufferSizeT );

    // A segment's size should only be multiples of MEMORY_ALIGNMENT, but even on the off
    const u32 segmentLength = reinterpret_cast<const u32*>(bufferCharStar+index)[0];
    const u32 roundedSegmentLength = Anki::RoundUp<u32>(segmentLength, MEMORY_ALIGNMENT);
    DASConditionalWarnAndReturn(segmentLength == roundedSegmentLength, false, "Anki.MemoryStack.IsValid", "The segmentLength is not a multiple of MEMORY_ALIGNMENT");

    // Check if the segment end is beyond the end of the buffer (NOTE: this is not conservative enough, though errors should be caught later)
    DASConditionalWarnAndReturn(segmentLength <= (usedBytes-index-HEADER_LENGTH-FOOTER_LENGTH), false, "Anki.MemoryStack.IsValid", "The segment end is beyond the end of the buffer");

    const u32 segmentHeader = reinterpret_cast<const u32*>(bufferCharStar+index)[1];

    DASConditionalWarnAndReturn(segmentHeader == FILL_PATTERN_START, false, "Anki.MemoryStack.IsValid", "segmentHeader == FILL_PATTERN_START");

    const u32 segmentFooter = reinterpret_cast<const u32*>(bufferCharStar+index+HEADER_LENGTH+segmentLength)[0];

    DASConditionalWarnAndReturn(segmentFooter == FILL_PATTERN_END, false, "Anki.MemoryStack.IsValid", "segmentFooter == FILL_PATTERN_END");

    index += HEADER_LENGTH + segmentLength + FOOTER_LENGTH;
  }

  if(index == usedBytes) {
    return true;
  } else if(index == LOOP_MAX){
    DASError("Anki.MemoryStack.IsValid", "Infinite while loop");
    return false;
  } else {
    DASError("Anki.MemoryStack.IsValid", "Loop exited at an incorrect position, probably due to corruption");
    return false;
  }
}

u32 Anki::MemoryStack::LargestPossibleAllocation()
{
  const size_t bufferNextFree = reinterpret_cast<size_t>(buffer) + usedBytes;
  const size_t bufferNextFreePlusHeaderAndAlignment = Anki::RoundUp<size_t>(bufferNextFree+HEADER_LENGTH, MEMORY_ALIGNMENT);
  const size_t bufferEnd = reinterpret_cast<size_t>(buffer) + totalBytes;

  // Make sure the next start point isn't past the end of the buffer (done in this way because of unsigned arithmetic)
  if( (bufferNextFreePlusHeaderAndAlignment+FOOTER_LENGTH+MEMORY_ALIGNMENT) > bufferEnd )
    return 0;

  // The RoundDown handles the requirement for all memory blocks to be multiples of MEMORY_ALIGNMENT
  const u32 maxFreeSpace = static_cast<u32>( Anki::RoundDown<size_t>(bufferEnd - bufferNextFreePlusHeaderAndAlignment - FOOTER_LENGTH, MEMORY_ALIGNMENT) );

  return maxFreeSpace;
}

u32 Anki::MemoryStack::get_totalBytes()
{
  return totalBytes;
}

u32 Anki::MemoryStack::get_usedBytes()
{
  return usedBytes;
}

void* Anki::MemoryStack::get_buffer()
{
  return buffer;
}

// Not sure if these should be supported. But I'm leaving them here for the time being.
/*
void Anki::MemoryStack::Clear()
{
usedBytes = 0;
}

const void* Anki::MemoryStack::get_buffer() const
{
return buffer;
}
*/