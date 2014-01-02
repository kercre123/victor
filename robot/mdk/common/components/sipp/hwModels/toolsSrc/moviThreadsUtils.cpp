#include "moviThreadsUtils.h"
//
//------------------------
// Mutex class definitions
//------------------------
//
// Default constructor
Mutex::Mutex()
{
    isValidMutex = true;
#ifdef WIN32
    mutexResource = CreateMutex(NULL, false, NULL);
    if(mutexResource == NULL)
    {
        isValidMutex = false;
    }
#else
    mutexResource = new pthread_mutex_t;
    if(pthread_mutex_init(mutexResource, NULL) != 0)
    {
        isValidMutex = false;
    }
#endif
}

// Constructor for encapsulation of raw mutex
#ifdef WIN32
Mutex::Mutex(HANDLE *mutex)
#else // Unix implementation
Mutex::Mutex(pthread_mutex_t *mutex)
#endif
{
    isValidMutex  = true;
#ifdef WIN32
    mutexResource = *mutex;
#else
    mutexResource = mutex;
#endif
}

// Default destructor
Mutex::~Mutex()
{
#ifdef WIN32
    CloseHandle(mutexResource);
#else
    pthread_mutex_destroy(mutexResource);
    //delete mutexResource;
#endif
}

// Return if valid created mutex
Mutex::operator bool()
{
    return isValidMutex;
}

// Lock mutex
void Mutex::Lock()
{
#ifdef WIN32
    // Acquire mutex resource
    WaitForSingleObject(mutexResource, INFINITE);
#else // Unix implementation
    pthread_mutex_lock(mutexResource);
#endif
}

// Unlock/Release mutex
void Mutex::Unlock()
{
    // Release mutex resource
#ifdef WIN32
    ReleaseMutex(mutexResource);
#else // Unix implementation
    pthread_mutex_unlock(mutexResource);
#endif
}


//----------------------------
// MutexLock class definitions
//----------------------------

// Constructor for object-encaplsulated mutex
MutexLock::MutexLock(Mutex *mutex)
{
    // Save pointer to object
    mutexResource        = mutex;
    incapsulatedRawMutex = false;
    // Acquire mutex resource
    mutexResource->Lock();
}

// Constuctor for raw-type mutex
#ifdef WIN32
MutexLock::MutexLock(HANDLE *mutex)
#else // Unix implementation
MutexLock::MutexLock(pthread_mutex_t *mutex)
#endif
{
    // Encapsulate mutex into object
    mutexResource        = new Mutex(mutex);
    incapsulatedRawMutex = true;
    // Acquire mutex resource
    mutexResource->Lock();
}

// Default destructor
MutexLock::~MutexLock()
{
    // Release mutex resource
    mutexResource->Unlock();
    // If self-incapsulated raw object
    if(incapsulatedRawMutex)
    {
        // Deleted self-generated incapsulating mutex object
        delete mutexResource;
    }
}

