#ifndef __MOVI_THREADS_UTILS_H__
#define __MOVI_THREADS_UTILS_H__

#pragma region Includes
// OS-specific include files
#ifdef WIN32
#include <windows.h>   // For console API, CreateMutex(), WaitForSingleObject(), ReleaseMutex()
#else
#include <pthread.h>   // For pthread_mutex_init(), pthread_mutex_lock(), pthread_mutex_unlock()
#include <semaphore.h> // 
#endif
#pragma endregion


//------------------
// Class definitions
//------------------

class Mutex
{
    // Pointer to acquired mutex
#ifdef WIN32
    HANDLE mutexResource;
#else
    pthread_mutex_t *mutexResource;
#endif
    // If valid created mutex
    bool isValidMutex;

    // Methods
public:
    Mutex();
#ifdef WIN32
    Mutex(HANDLE *mutex);
#else // Unix implementation
    Mutex(pthread_mutex_t *mutex);
#endif
    ~Mutex();
    operator bool();
    void Lock();
    void Unlock();
};

// MutexLock class declarations for managing mutex resources
class MutexLock
{
    // Pointer to acquired mutex
    Mutex *mutexResource;
    // If raw type Mutex has been incapsulated by lock class
    bool incapsulatedRawMutex;

    // Methods
public:
    explicit MutexLock(Mutex *mutex);
#ifdef WIN32
    explicit MutexLock(HANDLE *mutex);
#else // Unix implementation
    explicit MutexLock(pthread_mutex_t *mutex);
#endif
    ~MutexLock();
};

#endif
