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

#include "anki/common/robot/config.h"
#include "anki/common/robot/flags_declarations.h"

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

    // A MemoryStack keeps track of an external memory buffer, by using the system stack. It is not
    // thread safe. Data that is allocated with Allocate() will be MEMORY_ALIGNMENT bytes-aligned.
    // Allocate() data has fill patterns at the beginning and end, to ensure that buffers did not
    // overflow. This can be tested with IsValid().
    //
    // The safest way to use a MemoryStack is to pass by value. Passed by value: A copy is made on the
    // system stack. This means that on return of that function, the MemoryStack will be automatically
    // "popped" to the location it was before calling the function.
    //
    // A reference can be used for passing to initialization functions, or for speed- or
    // memory-critical areas. Passed by const reference: Using the copy constructor allows nested call
    // to push onto the MemoryStack, without modifying the MemoryStack at higher levels of the system
    // stack. Passed by non-const reference: This will compile and work, but use with caution, as the
    // MemoryStack object will no longer mirror the system stack. Mainly useful for initialization
    // functions.
    //

    class MemoryStack
    {
    public:
      MemoryStack(void) : buffer(NULL) { }
      MemoryStack(void *buffer, const s32 bufferLength, const Flags::Buffer flags=Flags::Buffer(true,true));
      MemoryStack(const MemoryStack &ms); // This is a safe way to remove const by making a copy, rather than using const_cast()

      // Allocate numBytes worth of memory, with the start byte-aligned to MEMORY_ALIGNMENT
      //
      // numBytesAllocated is an optional parameter. To satisfy stride requirements, Allocate() may
      // allocate more than numBytesRequested. numBytesAllocated is the amount of memory available
      // to the user from the returned void* pointer, and doesn't include overhead like the fill
      // patterns.
      //
      // All memory in the array is zeroed out once it is allocated, making Allocate more like calloc() than malloc()
      void* Allocate(const s32 numBytesRequested);
      void* Allocate(const s32 numBytesRequested, s32 &numBytesAllocated);

      // Reallocate will change the size of the last allocated memory segment. It only works on the
      // last segment. The return value is equal to memoryLocation, or NULL if there was an error.
      // The reallocated memory will not be cleared
      //
      // WARNING: This will not update any references to the memory, you must update all references
      //          manually.
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

      // Each MemoryStack created by the MemoryStack(void *buffer, s32 bufferLength) constructor has
      // a unique id. This is used for debugging to keep track of things like its maximum memory
      // usage.
      s32 get_id() const;

      // Flags are bitwise OR of MemoryStack::Flags
      Flags::Buffer get_flags() const;

    protected:
      friend class MemoryStackConstIterator;

      static const u32 FILL_PATTERN_START = 0xFF01FF02;
      static const u32 FILL_PATTERN_END = 0x03FF04FF;

      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 4;

      void * buffer;
      s32 totalBytes;
      s32 usedBytes;

      // Useful information for Reallocate()
      s32 usedBytesBeforeLastAllocation;
      void *lastAllocatedMemory;

      s32 id;

      // flags are set via MemoryStack::Flags
      Flags::Buffer flags;

    private:
      const void* Allocate(const s32 numBytes) const; // Not allowed
      //MemoryStack & operator= (const MemoryStack & rightHandSide); // Not allowed
    }; // class MemoryStack

    class MemoryStackConstIterator
    {
    public:
      MemoryStackConstIterator(const MemoryStack &memory);

      bool HasNext() const;

      const void * GetNext(s32 &segmentLength);

      const MemoryStack& get_memory() const;

    protected:
      s32 index;
      const MemoryStack &memory;
    }; // class MemoryStackConstIterator

    class MemoryStackIterator : public MemoryStackConstIterator
    {
    public:
      MemoryStackIterator(MemoryStack &memory);

      void * GetNext(s32 &segmentLength);

      MemoryStack& get_memory();
    }; // class MemoryStackIterator
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_MEMORY_H_
