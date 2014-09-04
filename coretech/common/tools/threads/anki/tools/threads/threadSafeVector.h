/**
File: threadSafeVector.h
Author: Peter Barnum
Created: 2014

Simple thread-safe vector. Works in Windows or Posix.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _THREAD_SAFE_VECTOR_H_
#define _THREAD_SAFE_VECTOR_H_

#include "anki/tools/threads/threadSafeUtilities.h"

#include <vector>

namespace Anki
{
  template<typename Type> class ThreadSafeVector
  {
  public:
    ThreadSafeVector();

    void Resize(const s32 newSize);

    // Returns the index where newValue was put
    s32 Push_back(Type newValue);

    // TODO: get the results out

  protected:
    mutable SimpleMutex mutex;

    std::vector<Type> buffer;
  }; // class ThreadSafeVector

  template<typename Type> ThreadSafeVector<Type>::ThreadSafeVector()
  {
#ifdef _MSC_VER
    mutex = CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&mutex, NULL);
#endif

    buffer = std::vector<Type>();
  } // template<typename Type> ThreadSafeVector::ThreadSafeVector()

  template<typename Type> void ThreadSafeVector<Type>::Resize(const s32 newSize)
  {
    LockSimpleMutex(mutex);

    buffer.resize(newSize);

    UnlockSimpleMutex(mutex);
  } // ThreadSafeVector<Type>::Resize(const s32 newSize)

  template<typename Type> s32 ThreadSafeVector<Type>::Push_back(Type newValue)
  {
    LockSimpleMutex(mutex);

    buffer.push_back(newValue);

    const s32 index = buffer.size() - 1;

    UnlockSimpleMutex(mutex);

    return index;
  } // ThreadSafeVector<Type>::Push_back(const Type &newValue)
} // namespace Anki

#endif // _THREAD_SAFE_VECTOR_H_
