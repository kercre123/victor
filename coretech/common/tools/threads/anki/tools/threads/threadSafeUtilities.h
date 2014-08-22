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

#else

#include <unistd.h>
#include <pthread.h>

#define SimpleMutex pthread_mutex_t
#define ThreadHandle pthread_t
#define ThreadResult void*

#endif

namespace Anki
{
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
} // namespace Anki

#endif // _THREAD_SAFE_UTILITES_H_
