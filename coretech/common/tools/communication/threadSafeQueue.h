#ifndef _THREAD_SAFE_QUEUE_H_
#define _THREAD_SAFE_QUEUE_H_

#include <queue>

#ifdef _MSC_VER
#include <windows.h>
#define SimpleMutex HANDLE
#else
#include <pthread.h>
#define SimpleMutex pthread_mutex_t
#endif

inline void WaitForSimpleMutex(SimpleMutex mutex)
{
#ifdef _MSC_VER
  WaitForSingleObject(mutex, INFINITE);
#else
  pthread_mutex_lock(&mutex);
#endif
}

inline void ReleaseSimpleMutex(SimpleMutex mutex)
{
#ifdef _MSC_VER
  ReleaseMutex(mutex);
#else
  pthread_mutex_unlock(&mutex);
#endif
}

template<typename Type> class ThreadSafeQueue
{
public:
  ThreadSafeQueue();

  Type Pop(bool * popSuccessful = NULL);

  void Push(Type newString);

  bool IsEmpty();

protected:
  SimpleMutex mutex;

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

template<typename Type> Type ThreadSafeQueue<Type>::Pop(bool * popSuccessful)
{
  Type value;

  WaitForSimpleMutex(mutex);

  if(buffers.empty()) {
    if(popSuccessful)
      *popSuccessful = false;
  } else {
    value = buffers.front();
    buffers.pop();

    if(popSuccessful)
      *popSuccessful = true;
  }

  ReleaseSimpleMutex(mutex);

  return value;
} // template<typename Type> ThreadSafeQueue::Type Pop()

template<typename Type> void ThreadSafeQueue<Type>::Push(Type newString)
{
  WaitForSimpleMutex(mutex);

  buffers.push(newString);

  ReleaseSimpleMutex(mutex);
} // template<typename Type> ThreadSafeQueue::void Push(Type newString)

template<typename Type> bool ThreadSafeQueue<Type>::IsEmpty()
{
  bool isEmpty;

  WaitForSimpleMutex(mutex);

  if(buffers.empty()) {
    isEmpty = true;
  } else {
    isEmpty = false;
  }

  ReleaseSimpleMutex(mutex);

  return isEmpty;
} // template<typename Type> bool ThreadSafeQueue::IsEmpty()

#endif // _THREAD_SAFE_QUEUE_H_
