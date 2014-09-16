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
    ThreadSafeCounter(const Type initialValue, const Type maxValue);

    // If delta is negative, it decrements
    // If this->value + delta is greater than maxValue, Increment will set this->value to this->maxValue
    // Returns the change between this->value, before and after the call. If this->value was not capped at this->maxValue, the return value is equal to delta.
    // TODO: use a condition variable internally, with a producer-consumer model
    Type Increment(const Type delta);

    // Returns MIN(this->maxValue, value);
    Type Set(const Type value);

    // Returns the current value
    Type Get() const;

  protected:
    mutable SimpleMutex mutex;

    Type value;
    Type maxValue;
  }; // class ThreadSafeCounter

  template<typename Type> ThreadSafeCounter<Type>::ThreadSafeCounter(const Type initialValue, const Type maxValue)
    : value(initialValue), maxValue(maxValue)
  {
#ifdef _MSC_VER
    mutex = CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&mutex, NULL);
#endif
  } // template<typename Type> ThreadSafeCounter::ThreadSafeCounter()

  template<typename Type> Type ThreadSafeCounter<Type>::Increment(const Type delta)
  {
    LockSimpleMutex(mutex);

    Type trueDelta = delta;

    if((this->value + delta) > this->maxValue) {
      trueDelta = this->maxValue - this->value;
      this->value = this->maxValue;
    } else {
      this->value += trueDelta;
    }

    AnkiAssert(this->value >= 0);

    UnlockSimpleMutex(mutex);

    return trueDelta;
  } // Increment()

  template<typename Type> Type ThreadSafeCounter<Type>::Set(const Type value)
  {
    LockSimpleMutex(mutex);

    this->value = MIN(this->maxValue, value);
    const Type localValue = this->value;

    UnlockSimpleMutex(mutex);

    return localValue;
  } // Set()

  template<typename Type> Type ThreadSafeCounter<Type>::Get() const
  {
    LockSimpleMutex(mutex);

    Type localValue = this->value;

    UnlockSimpleMutex(mutex);

    return localValue;
  } // Set()
} // namespace Anki
#endif // _THREAD_SAFE_QUEUE_H_
