#ifndef _ANKICORETECH_COMMON_MEMORY_H_
#define _ANKICORETECH_COMMON_MEMORY_H_

#include "anki/common/config.h"

namespace Anki
{
// A MemoryStack is like malloc without free. You can either add to the end of the current buffer, or restart at the top of the buffer.
// Data that is allocated with Allocate() will be MEMORY_ALIGNMENT byte aligned.
// Allocate() data has fill patterns at the beginning and end, to ensure that buffers did not overflow. This can be tested with IsConsistent().
class MemoryStack
{
public:
  static const u32 FILL_PATTERN_START = 0xABCD1089;
  static const u32 FILL_PATTERN_END = 0x89FE0189;

  MemoryStack(void *buffer, u32 bufferLength);

  // Allocate numBytes worth of memory, with the start byte-aligned to MEMORY_ALIGNMENT
  void* Allocate(u32 numBytes);

  // Reset usedBytes to zero 
  void Clear();

  // Check if any Allocate() memory was written out of bounds (via fill patterns at the beginnign and end)
  bool IsConsistent();

  // Probably these should not be used.
  void* get_buffer();
  const void* get_buffer() const;

  u32 get_totalBytes();
  u32 get_usedBytes();

protected:
  void *buffer;
  u32 totalBytes;
  u32 usedBytes;
};

} // namespace Anki

#endif // _ANKICORETECH_COMMON_MEMORY_H_
