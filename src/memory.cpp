
#include "anki/common.h"

#include <assert.h>

Anki::MemoryStack::MemoryStack(void *buffer, u32 bufferLength) 
  : buffer(buffer), totalBytes(bufferLength), usedBytes(0)
{
}

Anki::MemoryStack::MemoryStack(const MemoryStack& ms) 
  : buffer(ms.buffer), totalBytes(ms.totalBytes), usedBytes(ms.usedBytes)
{
}

void* Anki::MemoryStack::Allocate(u32 numBytes)
{
  assert(MEMORY_ALIGNMENT == 8);

  if(numBytes == 0) {
    DASWarn("AnkiVision.MemoryStack.Allocate", "numBytes == 0");
    return NULL;
  }

  if(numBytes > s32_MAX) {
    DASWarn("AnkiVision.MemoryStack.Allocate", "numBytes > s32_MAX");
    return NULL;
  }

  char * const bufferNextFree = static_cast<char*>(buffer) + usedBytes;
  
  // Get the pointer locations for header, data, and footer
  char * const segmentHeader = reinterpret_cast<char*>( Anki::RoundUp<size_t>(reinterpret_cast<size_t>(bufferNextFree), MEMORY_ALIGNMENT) );
  void * const segmentMemory = reinterpret_cast<void*>(segmentHeader + 8);
  char * const segmentFooter = reinterpret_cast<char*>(segmentMemory) + Anki::RoundUp<u32>(numBytes, MEMORY_ALIGNMENT);
  
  const u32 requestedBytes = static_cast<u32>( reinterpret_cast<size_t>(segmentFooter+4) - reinterpret_cast<size_t>(bufferNextFree) );

  if(requestedBytes > totalBytes) {
    DASEvent("AnkiVision.MemoryStack.Allocate", "Ran out of scratch space");
    return NULL;
  }

  // Next, add the header for this block
  reinterpret_cast<u32*>(segmentHeader)[0] = numBytes;
  reinterpret_cast<u32*>(segmentHeader)[1] = FILL_PATTERN_START;
  reinterpret_cast<u32*>(segmentFooter)[0] = FILL_PATTERN_END;

  usedBytes += requestedBytes;

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
    if(segmentLength > (usedBytes-index)) {
      DASWarn("AnkiVision.MemoryStack.IsConsistent", "segmentLength > (usedBytes-index)");
      return false;
    }

    if(segmentHeader != FILL_PATTERN_START) {
      DASWarn("AnkiVision.MemoryStack.IsConsistent", "segmentHeader != FILL_PATTERN_START");
      return false;
    }

    const u32 segmentFooter = reinterpret_cast<const u32*>(bufferChar+index+8+segmentLength)[0];

    if(segmentFooter != FILL_PATTERN_END) {
      DASWarn("AnkiVision.MemoryStack.IsConsistent", "segmentFooter != FILL_PATTERN_END");
      return false;
    }

    index += 12 + segmentLength;
  }

  if(index == usedBytes) {
    return true;
  } else if(index == LOOP_MAX){
    DASError("AnkiVision.MemoryStack.IsConsistent", "Infinite while loop");
    return false;
  } else {
    DASError("AnkiVision.MemoryStack.IsConsistent", "Loop exited at an incorrect positi	on, probably due to corruption");
    return false;
  }
}

u32 Anki::MemoryStack::get_totalBytes()
{
  return totalBytes;
}

u32 Anki::MemoryStack::get_usedBytes()
{
  return usedBytes;
}


// Not sure if these should be supported. But I'm leaving them here for the time being.
/*
void Anki::MemoryStack::Clear()
{
  usedBytes = 0;
}

void* Anki::MemoryStack::get_buffer()
{
  return buffer;
}

const void* Anki::MemoryStack::get_buffer() const
{
  return buffer;
}
*/
