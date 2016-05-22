/**
File: threadSafeUtilities.h
Author: Peter Barnum
Created: 2014-08-22

Simple utilities for thread-safe things. Works in Windows or Posix.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _THREAD_SAFE_UTILITES_H_
#define _THREAD_SAFE_UTILITES_H_

// Threading includes and defines
#ifdef _MSC_VER
#include <tchar.h>
#include <strsafe.h>
#include <windows.h>

#define SimpleMutex HANDLE
#define ThreadHandle HANDLE
#define ThreadResult DWORD WINAPI

// WARNING: Sleeps for a minimum of 1000 microseconds
inline void usleep(const unsigned int microseconds) { Sleep(MAX(1, microseconds / 1000)); }

#else

#include <unistd.h>
#include <pthread.h>

#define SimpleMutex pthread_mutex_t
#define ThreadHandle pthread_t
#define ThreadResult void*

#endif

namespace Anki
{
  // WARNING: Linux and OSX will deadlock if the same thread locks the same mutex multiple times. Windows won't.
  inline void LockSimpleMutex(SimpleMutex &mutex)
  {
#ifdef _MSC_VER
    WaitForSingleObject(mutex, INFINITE);
#else
    pthread_mutex_lock(&mutex);
#endif
  }

  inline void UnlockSimpleMutex(SimpleMutex &mutex)
  {
#ifdef _MSC_VER
    ReleaseMutex(mutex);
#else
    pthread_mutex_unlock(&mutex);
#endif
  }

#ifdef _MSC_VER
  inline ThreadHandle CreateSimpleThread(_In_ LPTHREAD_START_ROUTINE lpStartAddress, void * parameters)
  {
    DWORD threadId = -1;
    ThreadHandle handle = CreateThread(
      NULL,        // default security attributes
      0,           // use default stack size
      lpStartAddress, // thread function name
      parameters,    // argument to thread function
      0,           // use default creation flags
      &threadId);  // returns the thread identifier

    return handle;
  } // ThreadHandle CreateSimpleThread(_In_ LPTHREAD_START_ROUTINE lpStartAddress, void * parameters)
#else
  inline ThreadHandle CreateSimpleThread( void *(*start_routine) (void *), void * parameters)
  {
    pthread_attr_t connectionAttr;
    pthread_attr_init(&connectionAttr);

    ThreadHandle handle;

    pthread_create(&handle, &connectionAttr, start_routine, parameters);

    return handle;
  } // ThreadHandle CreateSimpleThread( void *(*start_routine) (void *), void * parameters)
#endif

  inline void WaitForSimpleThread(ThreadHandle &handle)
  {
#ifdef _MSC_VER
    WaitForSingleObject(handle, INFINITE);
#else
    pthread_join(handle, NULL);
#endif
  }  // void WaitForSimpleThread(ThreadHandle &handle)
} // namespace Anki

#endif // _THREAD_SAFE_UTILITES_H_
