/**
File: threadSafeQueue.h
Author: Peter Barnum
Created: 2014

WARNING: Difficult to use correctly. You must handle Lock and Unlock manually

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

    // WARNING: You must Lock and Unlock manually
    // This is for efficiency, though it makes code errors potentially disastrous

    void Lock() const;
    void Unlock() const;

    // WARNING: None of these check the mutex lock
    const Type& Front_unsafe() const;
    void Pop_unsafe();
    void Push_unsafe(Type newValue);
    s32 Size_unsafe() const;
    
    std::queue<Type>& get_buffer();
    
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

  template<typename Type> void ThreadSafeQueue<Type>::Lock() const
  {
    LockSimpleMutex(mutex);
  } // ThreadSafeQueue<Type>::Lock()

  template<typename Type> void ThreadSafeQueue<Type>::Unlock() const
  {
    UnlockSimpleMutex(mutex);
  } // ThreadSafeQueue<Type>::Unlock()

  template<typename Type> const Type& ThreadSafeQueue<Type>::Front_unsafe() const
  {
    return buffer.front();
  } // template<typename Type> ThreadSafeQueue::Type Front_unsafe()

  template<typename Type> void ThreadSafeQueue<Type>::Pop_unsafe()
  {
    buffer.pop();
  } // ThreadSafeQueue<Type>::Pop_unsafe()

  template<typename Type> void ThreadSafeQueue<Type>::Push_unsafe(Type newValue)
  {
    buffer.push(newValue);
  } // template<typename Type> ThreadSafeQueue::void Push_unsafe(Type newValue)

  template<typename Type> s32 ThreadSafeQueue<Type>::Size_unsafe() const
  {
    return static_cast<s32>(buffer.size());
  } // template<typename Type> bool ThreadSafeQueue::Size_unsafe()
  
  template<typename Type> std::queue<Type>& ThreadSafeQueue<Type>::get_buffer()
  {
    return buffer;
  }
} // namespace Anki

#endif // _THREAD_SAFE_QUEUE_H_
