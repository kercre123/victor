#ifndef _ANKICORETECH_COMMON_MEMORY_H_
#define _ANKICORETECH_COMMON_MEMORY_H_

#include "anki/common/config.h"

namespace Anki
{
// A MemoryStack keeps track of an external memory buffer, by using the system stack. It is not thread safe.
//
// Passed by value: A copy is made on the system stack. This means that on return of that function, the MemoryStack will be automatically "popped" to the location it was before calling the function.
// Passed by const reference: No copy is made on the system stack. But using the copy constructor allows the stack to be increased by nested calls, without modifying the MemoryStack at higher levels of the system stack.
// Passed by non-const reference: This will compile and work, but use with caution, as the MemoryStack object will no longer mirror the system stack.
//
// Data that is allocated with Allocate() will be MEMORY_ALIGNMENT bytes-aligned.
// Allocate() data has fill patterns at the beginning and end, to ensure that buffers did not overflow. This can be tested with IsConsistent().
class MemoryStack
{
public:
  static const u32 FILL_PATTERN_START = 0xABCD1089;
  static const u32 FILL_PATTERN_END = 0x89FE0189;

  MemoryStack(void *buffer, u32 bufferLength);
  MemoryStack(const MemoryStack &ms); // This is a safe way to remove const, rather than using const_cast()

  // Allocate numBytes worth of memory, with the start byte-aligned to MEMORY_ALIGNMENT
  void* Allocate(u32 numBytes);

  // Check if any Allocate() memory was written out of bounds (via fill patterns at the beginning and end)
  bool IsConsistent();

  u32 get_totalBytes();
  u32 get_usedBytes();
    
  // Probably these should not be used?
  // void* get_buffer();
  // const void* get_buffer() const;
  // void Clear(); // Reset usedBytes to zero 

protected:
  void *buffer;
  u32 totalBytes;
  u32 usedBytes;

  const void* Allocate(u32 numBytes) const; // Not allowed
};

} // namespace Anki

#endif // _ANKICORETECH_COMMON_MEMORY_H_
