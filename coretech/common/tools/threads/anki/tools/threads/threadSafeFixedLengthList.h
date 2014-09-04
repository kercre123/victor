/**
File: threadSafeVector.h
Author: Peter Barnum
Created: 2014

Thread-safe FixedLengthList. Works in Windows or Posix.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _THREAD_SAFE_FIXED_LENGTH_LIST_H_
#define _THREAD_SAFE_FIXED_LENGTH_LIST_H_

#include "anki/tools/threads/threadSafeUtilities.h"

#include <vector>

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class ThreadSafeFixedLengthList
    {
    public:
      ThreadSafeFixedLengthList(s32 maximumSize, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false,false));

      // The maximum size is set at object construction
      s32 get_maximumSize() const;

      bool IsValid() const;

      // Get or release exclusive access to the buffer
      void Lock() const;
      void Unlock() const;

      // WARNING: allows non-thread-safe access. Unless you're sure no-one else is writing to it, use Lock and Unlock.
      FixedLengthList<Type>& get_buffer();

    protected:
      mutable SimpleMutex mutex;

      FixedLengthList<Type> buffer;
    }; // class ThreadSafeFixedLengthList

    template<typename Type> ThreadSafeFixedLengthList<Type>::ThreadSafeFixedLengthList(s32 maximumSize, MemoryStack &memory, const Flags::Buffer flags)
    {
#ifdef _MSC_VER
      mutex = CreateMutex(NULL, FALSE, NULL);
#else
      pthread_mutex_init(&mutex, NULL);
#endif

      buffer = FixedLengthList<Type>(maximumSize, memory, flags);
    } // template<typename Type> ThreadSafeFixedLengthList::ThreadSafeFixedLengthList()

    template<typename Type> s32 ThreadSafeFixedLengthList<Type>::get_maximumSize() const
    {
      return buffer.get_maximumSize();
    }

    template<typename Type> bool ThreadSafeFixedLengthList<Type>::IsValid() const
    {
      LockSimpleMutex(mutex);

      const bool isValid = this->buffer.IsValid();

      UnlockSimpleMutex(mutex);

      return isValid;
    }

    template<typename Type> void ThreadSafeFixedLengthList<Type>::Lock() const
    {
      LockSimpleMutex(mutex);
    }

    template<typename Type> void ThreadSafeFixedLengthList<Type>::Unlock() const
    {
      UnlockSimpleMutex(mutex);
    }

    template<typename Type> FixedLengthList<Type>& ThreadSafeFixedLengthList<Type>::get_buffer()
    {
      return this->buffer;
    }
  } // namespace Embedded
} // namespace Anki

#endif // _THREAD_SAFE_FIXED_LENGTH_LIST_H_
