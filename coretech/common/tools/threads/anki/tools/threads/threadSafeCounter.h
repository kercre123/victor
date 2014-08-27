/**
File: threadSafeCounter.h
Author: Peter Barnum
Created: 2014-08-22

A simple mutex-guarded counter that can be incremented, decremented, or set. Works in Windows or Posix.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _THREAD_SAFE_COUNTER_H_
#define _THREAD_SAFE_COUNTER_H_

#include "anki/tools/threads/threadSafeUtilities.h"

namespace Anki
{
  template<typename Type> class ThreadSafeCounter
  {
  public:
    ThreadSafeCounter(Type initialValue);

    // If delta is negative, it decrements
    // Returns the updated value
    Type Increment(const Type delta);

    // Returns the updated value (should always be the same as the input)
    Type Set(const Type value);

    // Returns the current value
    Type Get() const;

  protected:
    mutable SimpleMutex mutex;

    Type value;
  }; // class ThreadSafeCounter

  template<typename Type> ThreadSafeCounter<Type>::ThreadSafeCounter(Type initialValue)
  {
#ifdef _MSC_VER
    mutex = CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&mutex, NULL);
#endif

    value = initialValue;
  } // template<typename Type> ThreadSafeCounter::ThreadSafeCounter()

  template<typename Type> Type ThreadSafeCounter<Type>::Increment(const Type delta)
  {
    Type localValue;

    LockSimpleMutex(mutex);

    this->value += delta;
    localValue = this->value;

    UnlockSimpleMutex(mutex);

    return localValue;
  } // Increment()

  template<typename Type> Type ThreadSafeCounter<Type>::Set(const Type value)
  {
    Type localValue;

    LockSimpleMutex(mutex);

    this->value = value;
    localValue = value;

    UnlockSimpleMutex(mutex);

    return localValue;
  } // Set()

  template<typename Type> Type ThreadSafeCounter<Type>::Get() const
  {
    Type localValue;

    LockSimpleMutex(mutex);

    localValue = this->value;

    UnlockSimpleMutex(mutex);

    return localValue;
  } // Set()
} // namespace Anki
#endif // _THREAD_SAFE_QUEUE_H_
