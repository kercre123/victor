/**
File: memory.h
Author: Peter Barnum
Created: 2013

Routines and classes to do with the handling of large amounts of memory.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_MEMORY_H_
#define _ANKICORETECHEMBEDDED_COMMON_MEMORY_H_

#include "coretech/common/shared/types.h"
#include "coretech/common/robot/flags_declarations.h"

namespace Anki
{
  namespace Embedded
  {
#define PUSH_MEMORY_STACK(memoryStack) \
  const ::Anki::Embedded::MemoryStack memoryStack ## _pushedTmp(memoryStack);\
  ::Anki::Embedded::MemoryStack memoryStack(memoryStack ## _pushedTmp);

    class MemoryStack;
    class MemoryStackIterator;
    class MemoryStackConstIterator;
    class MemoryStackReconstructingIterator;
    class MemoryStackReconstructingConstIterator;

    class SerializedBuffer;

    // A MemoryStack keeps track of an external memory buffer, by using the system stack. It is not
    // thread safe. Data that is allocated with Allocate() will be MEMORY_ALIGNMENT bytes-aligned.
    // Allocate() data has fill patterns at the beginning and end, to ensure that buffers did not
    // overflow. This can be tested with IsValid().
    //
    // The safest way to use a MemoryStack is to pass by value. Passed by value: A copy is made on
    // the system stack. This means that on return of that function, the MemoryStack will be
    // automatically "popped" to the location it was before calling the function.
    //
    // A reference can be used for passing to initialization functions, or for speed-critical or
    // memory-critical areas.
    // 1. Passed by const reference: Using the copy constructor allows nested call to push onto the
    // MemoryStack, without modifying the MemoryStack at higher levels of the system stack.
    // 2. Passed by non-const reference: This will compile and work, but use with caution, as the
    // MemoryStack object will no longer mirror the system stack. Mainly useful for initialization functions.
    class MemoryStack
    {
    public:
      static const u32 FILL_PATTERN_START = 0xFF01FE02;
      static const u32 FILL_PATTERN_END = 0x03FF04FE;

      // The header contains the size of the allocated segment, and the fill pattern
      static const s32 HEADER_LENGTH = 8;

      // The footer is just a fill pattern
      static const s32 FOOTER_LENGTH = 4;

      MemoryStack();
      MemoryStack(void *buffer, const s32 bufferLength, const Flags::Buffer flags=Flags::Buffer(true,true,false));
      MemoryStack(const MemoryStack &ms);

      // Allocate numBytes worth of memory, with the start byte-aligned to MEMORY_ALIGNMENT
      //
      // numBytesAllocated is an optional parameter. To satisfy stride requirements, Allocate() may
      // allocate more than numBytesRequested. numBytesAllocated is the amount of memory available
      // to the user from the returned void* pointer, and doesn't include overhead like the fill patterns.
      //
      // If Flags::Buffer::zeroAllocatedMemory is true, then all memory in the array is zeroed out
      // once it is allocated, making Allocate more like calloc() than malloc()
      void* Allocate(const s32 numBytesRequested);
      void* Allocate(const s32 numBytesRequested, s32 &numBytesAllocated);

      // Same as the above, but zeroAllocatedMemory is specified only for this segment
      void* Allocate(const s32 numBytesRequested, const bool zeroAllocatedMemory, s32 &numBytesAllocated);

      // Reallocate will change the size of the last allocated memory segment. It only works on the
      // last segment. The return value is equal to memoryLocation, or NULL if there was an error.
      // The reallocated memory will not be cleared.
      //
      // WARNING:
      // This will not update any references to the memory, you must update all references manually.
      void* Reallocate(void* memoryLocation, s32 numBytesRequested);
      void* Reallocate(void* memoryLocation, s32 numBytesRequested, s32 &numBytesAllocated);

      // Check if any Allocate() memory was written out of bounds (via fill patterns at the beginning and end)
      bool IsValid() const;

      // Returns the number of bytes that can still be allocated.
      // The max allocation is less than or equal to "get_totalBytes() - get_usedBytes() - 12".
      s32 ComputeLargestPossibleAllocation() const;

      // Print out this MemoryStack's id and memory usage
      Result Print() const;

      s32 get_totalBytes() const;
      s32 get_usedBytes() const;

      void* get_buffer();
      const void* get_buffer() const;

      // The first few bytes of a buffer may be garbage, due to memory alignment restrictions
      // These functions return the first location on the buffer that is actually used
      void* get_validBufferStart();
      const void* get_validBufferStart() const;

      void* get_validBufferStart(s32 &firstValidIndex);
      const void* get_validBufferStart(s32 &firstValidIndex) const;

      // Each MemoryStack created by the MemoryStack(void *buffer, s32 bufferLength) constructor has
      // a unique id. This is used for debugging to keep track of things like its maximum memory
      // usage.
      s32 get_id() const;

      // Flags are bitwise OR of MemoryStack::Flags
      Flags::Buffer get_flags() const;

    protected:
      friend class SerializedBuffer;

      void * buffer;
      s32 totalBytes;
      s32 usedBytes;

      // Useful information for Reallocate()
      s32 usedBytesBeforeLastAllocation;
      void *lastAllocatedMemory;

      s32 id;

      Flags::Buffer flags;

    private:
      const void* Allocate(const s32 numBytes) const; // Not allowed
    }; // class MemoryStack

    class MemoryStackConstIterator
    {
    public:
      MemoryStackConstIterator(const MemoryStack &memory);

      bool HasNext() const;

      // Returns NULL if there are no more segments
      const void * GetNext(s32 &segmentLength, const bool requireFillPatternMatch=true);

      const MemoryStack& get_memory() const;

    protected:
      s32 index;
      const MemoryStack &memory;
    }; // class MemoryStackConstIterator

    class MemoryStackIterator : public MemoryStackConstIterator
    {
    public:
      MemoryStackIterator(MemoryStack &memory);

      // Returns NULL if there are no more segments
      void * GetNext(s32 &segmentLength, const bool requireFillPatternMatch=true);

      MemoryStack& get_memory();
    }; // class MemoryStackIterator

    class MemoryStackReconstructingConstIterator
    {
      // A Raw Iterator doesn't use the reported segment length to find the next segment. Instead,
      // it searches through every byte of the buffer, to find matching begin/end fill patterns. As
      // a result, it is more tolerant to missing data, but will also be slower.
    public:
      MemoryStackReconstructingConstIterator(const MemoryStack &memory);

      bool HasNext() const;
      bool HasNext(s32 &startIndex, s32 &endIndex, s32 &reportedLength) const;

      // Returns NULL if there are no more segments
      const void * GetNext(s32 &trueSegmentLength, s32 &reportedSegmentLength);

      const MemoryStack& get_memory() const;

    protected:
      s32 index;
      const MemoryStack &memory;
    }; // class MemoryStackReconstructingConstIterator

    class MemoryStackReconstructingIterator : public MemoryStackReconstructingConstIterator
    {
      // See MemoryStackReconstructingConstIterator
    public:
      MemoryStackReconstructingIterator(MemoryStack &memory);

      // Returns NULL if there are no more segments
      void * GetNext(s32 &trueSegmentLength, s32 &reportedSegmentLength);

      MemoryStack& get_memory();
    }; // class MemoryStackReconstructingIterator
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_MEMORY_H_
