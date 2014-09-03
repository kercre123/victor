/**
File: threadSafeQueue.h
Author: Peter Barnum
Created: 2014

WARNING: Difficult to use correctly. You must pair Front/Pop or Front/Unlock calls manually

Thread-safe queue. Works in Windows or Posix.

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

    // WARNING: You must call either "Front and Pop" or "Front and Unlock"
    // This is for efficiency, though it makes code errors potentially disasterous
    const Type& Front() const; // WARNING: Locks the mutex, but doesn't unlock it
    void Pop(); // WARNING: A thread can call Pop, even if it doesn't have the mutex lock
    void Unlock() const; // WARNING: A thread can call Unlock, even if it doesn't have the mutex lock

    void Push(const Type &newValue);

    void Emplace(const Type &&newValue);

    s32 Size() const;

    bool Empty() const;

  protected:
    mutable SimpleMutex mutex;

    std::queue<Type> buffer;
  }; // class ThreadSafeQueue

  template<typename Type> ThreadSafeQueue<Type>::ThreadSafeQueue()
  {
#ifdef _MSC_VER
    mutex = CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&mutex, NULL);
#endif

    buffer = std::queue<Type>();
  } // template<typename Type> ThreadSafeQueue::ThreadSafeQueue()

  template<typename Type> const Type& ThreadSafeQueue<Type>::Front() const
  {
    LockSimpleMutex(mutex);

    return buffer.front();
  } // template<typename Type> ThreadSafeQueue::Type Pop()

  template<typename Type> void ThreadSafeQueue<Type>::Pop()
  {
    // TODO: check if the current thread has the lock

    buffer.pop();

    UnlockSimpleMutex(mutex);
  } // ThreadSafeQueue<Type>::Pop()

  template<typename Type> void ThreadSafeQueue<Type>::Unlock() const
  {
    // TODO: check if the current thread has the lock

    UnlockSimpleMutex(mutex);
  } // ThreadSafeQueue<Type>::Unlock()

  template<typename Type> void ThreadSafeQueue<Type>::Push(const Type &newValue)
  {
    LockSimpleMutex(mutex);

    buffer.push(newValue);

    UnlockSimpleMutex(mutex);
  } // template<typename Type> ThreadSafeQueue::void Push(Type newValue)

  template<typename Type> void ThreadSafeQueue<Type>::Emplace(const Type &&newValue)
  {
    LockSimpleMutex(mutex);

    buffer.emplace(newValue);

    UnlockSimpleMutex(mutex);
  } // template<typename Type> ThreadSafeQueue::void Push(Type newValue)

  template<typename Type> s32 ThreadSafeQueue<Type>::Size() const
  {
    LockSimpleMutex(mutex);

    s32 curSize = buffer.size();

    UnlockSimpleMutex(mutex);

    return curSize;
  } // template<typename Type> bool ThreadSafeQueue::Size()

  template<typename Type> bool ThreadSafeQueue<Type>::Empty() const
  {
    bool isEmpty;

    LockSimpleMutex(mutex);

    if(buffer.empty()) {
      isEmpty = true;
    } else {
      isEmpty = false;
    }

    UnlockSimpleMutex(mutex);

    return isEmpty;
  } // template<typename Type> bool ThreadSafeQueue::Empty()
} // namespace Anki

#endif // _THREAD_SAFE_QUEUE_H_
