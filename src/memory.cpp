#include "anki/common.h"

#include <assert.h>

Anki::MemoryStack::MemoryStack(void *buffer, u32 bufferLength)
  : buffer(buffer), totalBytes(bufferLength), usedBytes(0)
{
  DASConditionalError(buffer, "Anki.MemoryStack.MemoryStack", "Buffer must be allocated");
  DASConditionalError(bufferLength <= 0x3FFFFFFF, "Anki.MemoryStack.MemoryStack", "Maximum size of a MemoryStack is 2^30 - 1");
  DASConditionalError(MEMORY_ALIGNMENT == 8, "Anki.MemoryStack.MemoryStack", "Currently, only MEMORY_ALIGNMENT == 8 is supported");
}

Anki::MemoryStack::MemoryStack(const MemoryStack& ms)
  : buffer(ms.buffer), totalBytes(ms.totalBytes), usedBytes(ms.usedBytes)
{
  DASConditionalError(ms.buffer, "Anki.MemoryStack.MemoryStack", "Buffer must be allocated");
  DASConditionalError(ms.totalBytes <= 0x3FFFFFFF, "Anki.MemoryStack.MemoryStack", "Maximum size of a MemoryStack is 2^30 - 1");
  DASConditionalError(MEMORY_ALIGNMENT == 8, "Anki.MemoryStack.MemoryStack", "Currently, only MEMORY_ALIGNMENT == 8 is supported");
  DASConditionalError(ms.totalBytes >= ms.usedBytes, "Anki.MemoryStack.MemoryStack", "Buffer is using more bytes than it has. Try running IsConsistent() to test for memory corruption.");
}

void* Anki::MemoryStack::Allocate(u32 numBytesRequested, u32 *numBytesAllocated)
{
  DASConditionalWarnAndReturn(numBytesRequested != 0, NULL, "Anki.MemoryStack.Allocate", "numBytesRequested != 0");
  DASConditionalWarnAndReturn(numBytesRequested <= 0x3FFFFFFF, NULL, "Anki.MemoryStack.Allocate", "numBytesRequested <= 0x3FFFFFFF");

  char * const bufferNextFree = static_cast<char*>(buffer) + usedBytes;

  // Get the pointer locations for header, data, and footer
  char * const segmentHeader = reinterpret_cast<char*>( Anki::RoundUp<size_t>(reinterpret_cast<size_t>(bufferNextFree), MEMORY_ALIGNMENT) );
  void * const segmentMemory = reinterpret_cast<void*>(segmentHeader + 8);
  char * const segmentFooter = reinterpret_cast<char*>(segmentMemory) + Anki::RoundUp<u32>(numBytesRequested, MEMORY_ALIGNMENT);

  const u32 requestedBytes = static_cast<u32>( reinterpret_cast<size_t>(segmentFooter+4) - reinterpret_cast<size_t>(bufferNextFree) );

  DASConditionalEventAndReturn((usedBytes+requestedBytes) <= totalBytes, NULL, "Anki.MemoryStack.Allocate", "Ran out of scratch space");

  // Next, add the header for this block
  reinterpret_cast<u32*>(segmentHeader)[0] = numBytesRequested;
  reinterpret_cast<u32*>(segmentHeader)[1] = FILL_PATTERN_START;
  reinterpret_cast<u32*>(segmentFooter)[0] = FILL_PATTERN_END;

  usedBytes += requestedBytes;

  if(numBytesAllocated) {
    *numBytesAllocated = reinterpret_cast<u32>(segmentFooter) - reinterpret_cast<u32>(segmentMemory);
  }

  return segmentMemory;
}

bool Anki::MemoryStack::IsConsistent()
{
  const size_t LOOP_MAX = 1000000;
  const char * const bufferChar = reinterpret_cast<const char*>(buffer);
  u32 index = static_cast<u32>( Anki::RoundUp<size_t>(reinterpret_cast<size_t>(buffer), MEMORY_ALIGNMENT) - reinterpret_cast<size_t>(buffer) );

  for(size_t i=0; (i<MIN(LOOP_MAX,totalBytes)) && (index<usedBytes); i++) {
    index = static_cast<u32>( Anki::RoundUp<size_t>(reinterpret_cast<size_t>(buffer)+index, MEMORY_ALIGNMENT) - reinterpret_cast<size_t>(buffer) );

    const u32 segmentLength = static_cast<u32>( Anki::RoundUp<size_t>(reinterpret_cast<const size_t*>(bufferChar+index)[0], MEMORY_ALIGNMENT) );
    const u32 segmentHeader = reinterpret_cast<const u32*>(bufferChar+index)[1];

    // Simple sanity check
    DASConditionalWarnAndReturn(segmentLength <= (usedBytes-index), false, "Anki.MemoryStack.IsConsistent", "segmentLength <= (usedBytes-index)");
    DASConditionalWarnAndReturn(segmentHeader == FILL_PATTERN_START, false, "Anki.MemoryStack.IsConsistent", "segmentHeader == FILL_PATTERN_START");

    const u32 segmentFooter = reinterpret_cast<const u32*>(bufferChar+index+8+segmentLength)[0];

    DASConditionalWarnAndReturn(segmentFooter == FILL_PATTERN_END, false, "Anki.MemoryStack.IsConsistent", "segmentFooter == FILL_PATTERN_END");

    index += 12 + segmentLength;
  }

  if(index == usedBytes) {
    return true;
  } else if(index == LOOP_MAX){
    DASError("Anki.MemoryStack.IsConsistent", "Infinite while loop");
    return false;
  } else {
    DASError("Anki.MemoryStack.IsConsistent", "Loop exited at an incorrect position, probably due to corruption");
    return false;
  }
}

u32 Anki::MemoryStack::LargestPossibleAllocation()
{
  const u32 bufferNextFree = static_cast<u32>(reinterpret_cast<size_t>(buffer)) + usedBytes;
  const u32 bufferNextFreePlusAlignment = Anki::RoundUp<u32>(bufferNextFree, MEMORY_ALIGNMENT);
  const u32 bufferEnd = static_cast<u32>(reinterpret_cast<size_t>(buffer)) + totalBytes;

  if(bufferNextFreePlusAlignment >= (bufferEnd-12-MEMORY_ALIGNMENT+1))
    return 0;

  // The RoundDown handles the requirement for all memory blocks to be multiples of MEMORY_ALIGNMENT
  const u32 maxBytes = Anki::RoundDown<u32>(bufferEnd-bufferNextFreePlusAlignment-12, MEMORY_ALIGNMENT);

  return maxBytes;
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