// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : Wrapper-class around Pthreads/WINAPI critical section 
//                    functionality for platform abstraction.
//
// Courtesy of http://scriptionary.com
// -----------------------------------------------------------------------------

#ifndef __CRITICAL_SECTION_H__
#define __CRITICAL_SECTION_H__

#ifdef WIN32
#   include <windows.h>
#else
#   include <unistd.h>
#   include <pthread.h>
#endif

class CriticalSection
{
public:
    CriticalSection(void)
    {
    #ifdef WIN32
        if (0 == InitializeCriticalSectionAndSpinCount(&m_cSection, 0))
            throw("Could not create a CriticalSection");
    #else
        if (pthread_mutex_init(&m_cSection, NULL) != 0)
            throw("Could not create a CriticalSection");
    #endif
    };

	~CriticalSection(void)
    {
        Enter(); // acquire ownership (for pthread)
    #ifdef WIN32
        DeleteCriticalSection(&m_cSection);
    #else
        pthread_mutex_destroy(&m_cSection);
    #endif
    };

    // Wait for unlock and enter the CriticalSection object.
    void Enter(void)
    {
    #ifdef WIN32
        EnterCriticalSection(&m_cSection);
    #else
        pthread_mutex_lock(&m_cSection);
    #endif
    };

    // Leaves the critical section object.
    // This function will only work if the current thread
    // holds the current lock on the CriticalSection object
    // called by Enter()
    void Leave(void)
    {
    #ifdef WIN32
        LeaveCriticalSection(&m_cSection);
    #else
        pthread_mutex_unlock(&m_cSection);
    #endif
    };

    // TryEnter(void)
    // Attempt to enter the CriticalSection object
    bool TryEnter(void)
    {
        // Attempt to acquire ownership:
    #ifdef WIN32
        return(TRUE == TryEnterCriticalSection(&m_cSection));
    #else
        return(0 == pthread_mutex_trylock(&m_cSection));
    #endif
    };
 
private:
#ifdef WIN32
    CRITICAL_SECTION m_cSection; // Internal system critical section object (windows)
#else
    pthread_mutex_t m_cSection; // Internal system critical section object (*nix)
#endif
};

#endif // __CRITCAL_SECTION_H__
