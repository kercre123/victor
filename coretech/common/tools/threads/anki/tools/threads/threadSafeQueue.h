/**
File: threadSafeQueue.h
Author: Peter Barnum
Created: 2014

Simple thread-safe queue. Works in Windows or Posix.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _THREAD_SAFE_QUEUE_H_
#define _THREAD_SAFE_QUEUE_H_

#include "anki/tools/threads/threadSafeUtilities.h"

#include <queue>

namespace Anki
{
  template<typename Type> class ThreadSafeQueue
  {
  public:
    ThreadSafeQueue();

    Type Front() const;

    void Pop();

    void Push(const Type &newValue);

    bool Empty() const;

  protected:
    mutable SimpleMutex mutex;

    std::queue<Type> buffers;
  }; // class ThreadSafeQueue

  template<typename Type> ThreadSafeQueue<Type>::ThreadSafeQueue()
  {
#ifdef _MSC_VER
    mutex = CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&mutex, NULL);
#endif

    buffers = std::queue<Type>();
  } // template<typename Type> ThreadSafeQueue::ThreadSafeQueue()

  template<typename Type> Type ThreadSafeQueue<Type>::Front() const
  {
    Type value;

    LockSimpleMutex(mutex);

    value = buffers.front();

    UnlockSimpleMutex(mutex);

    return value;
  } // template<typename Type> ThreadSafeQueue::Type Pop()

  template<typename Type> void ThreadSafeQueue<Type>::Pop()
  {
    LockSimpleMutex(mutex);

    buffers.pop();

    UnlockSimpleMutex(mutex);
  } // template<typename Type> ThreadSafeQueue::Type Pop()

  template<typename Type> void ThreadSafeQueue<Type>::Push(const Type &newValue)
  {
    LockSimpleMutex(mutex);

    buffers.push(newValue);

    UnlockSimpleMutex(mutex);
  } // template<typename Type> ThreadSafeQueue::void Push(Type newValue)

  template<typename Type> bool ThreadSafeQueue<Type>::Empty() const
  {
    bool isEmpty;

    LockSimpleMutex(mutex);

    if(buffers.empty()) {
      isEmpty = true;
    } else {
      isEmpty = false;
    }

    UnlockSimpleMutex(mutex);

    return isEmpty;
  } // template<typename Type> bool ThreadSafeQueue::IsEmpty()
} // namespace Anki

#endif // _THREAD_SAFE_QUEUE_H_
