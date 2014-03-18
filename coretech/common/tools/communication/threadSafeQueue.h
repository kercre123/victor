#ifndef _THREAD_SAFE_QUEUE_H_
#define _THREAD_SAFE_QUEUE_H_

#include <queue>

template<typename Type> class ThreadSafeQueue
{
public:
  ThreadSafeQueue();

  Type Pop();

  void Push(Type newString);

  bool IsEmpty();

protected:
  HANDLE mutex;

  std::queue<Type> buffers;
}; // class ThreadSafeQueue

template<typename Type> ThreadSafeQueue<Type>::ThreadSafeQueue()
{
  mutex = CreateMutex(NULL, FALSE, NULL);
  buffers = std::queue<Type>();
} // template<typename Type> ThreadSafeQueue::ThreadSafeQueue()

template<typename Type> Type ThreadSafeQueue<Type>::Pop()
{
  Type value;

  WaitForSingleObject(mutex, INFINITE);

  // TODO: figure out what to return on failure
  if(buffers.empty()) {
    //    value = static_cast<Type>(0);
  } else {
    value = buffers.front();
    buffers.pop();
  }

  ReleaseMutex(mutex);

  return value;
} // template<typename Type> ThreadSafeQueue::Type Pop()

template<typename Type> void ThreadSafeQueue<Type>::Push(Type newString)
{
  WaitForSingleObject(mutex, INFINITE);

  buffers.push(newString);

  ReleaseMutex(mutex);
} // template<typename Type> ThreadSafeQueue::void Push(Type newString)

template<typename Type> bool ThreadSafeQueue<Type>::IsEmpty()
{
  bool isEmpty;

  WaitForSingleObject(mutex, INFINITE);

  if(buffers.empty()) {
    isEmpty = true;
  } else {
    isEmpty = false;
  }

  ReleaseMutex(mutex);

  return isEmpty;
} // template<typename Type> bool ThreadSafeQueue::IsEmpty()

#endif // _THREAD_SAFE_QUEUE_H_
