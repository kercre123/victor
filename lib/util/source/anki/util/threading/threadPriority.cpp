/**
 * File: threadPriority
 *
 * Author: Mark Wesley
 * Created: 05/24/16
 *
 * Description: Support for setting a desired thread priority
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "util/threading/threadPriority.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include <assert.h>
#include <pthread.h>
#include <string.h>


namespace Anki {
namespace Util {
  
  
static int ThreadPriorityToPolicy(ThreadPriority threadPriority)
{
  switch(threadPriority)
  {
    case ThreadPriority::Default:
      return SCHED_OTHER;
    case ThreadPriority::Min:
    case ThreadPriority::Low:
    case ThreadPriority::High:
    case ThreadPriority::Max:
      return SCHED_RR; // SCHED_FIFO
  }
}


static int ThreadPriorityToIntPriority(int policy, ThreadPriority threadPriority)
{
  const int minPriority = sched_get_priority_min(policy);
  const int maxPriority = sched_get_priority_max(policy);
  
  switch(threadPriority)
  {
    case ThreadPriority::Min:
      return minPriority;
    case ThreadPriority::Low:
      return minPriority + (int)(0.25f * float(maxPriority-minPriority));
    case ThreadPriority::Default:
      // We don't know what the default was, assume medium (we shouldn't usually get here as we only set for non-default)
      return minPriority + (int)(0.5f * float(maxPriority-minPriority));
    case ThreadPriority::High:
      return minPriority + (int)(0.75f * float(maxPriority-minPriority));
    case ThreadPriority::Max:
      return maxPriority;
  }
}


void SetThreadPriority(pthread_t inThread, ThreadPriority threadPriority)
{
  int oldPolicy   = -1;
  int oldPriority = -1;
  
  if (ANKI_DEVELOPER_CODE)
  {
    sched_param oldParam;
    
    int res = pthread_getschedparam(inThread, &oldPolicy, &oldParam);
    if (res != 0)
    {
      PRINT_NAMED_ERROR("GetThreadPriority.Failed",
                        "Error: %s (res=%d) getting thread priority and policy", std::strerror(errno), res);
    }
    else
    {
      oldPriority = oldParam.sched_priority;
    }
  }
  
  const int policy   = ThreadPriorityToPolicy(threadPriority);
  const int priority = ThreadPriorityToIntPriority(policy, threadPriority);
  
  sched_param sch_params;
  sch_params.sched_priority = priority;
  
  const int res = pthread_setschedparam(inThread, policy, &sch_params);

  if (res != 0)
  {

#if defined(ANDROID)
    // Currently, on Android, we expect pthread_setschedparam to fail with EPERM.
    // A regular (non-root) process cannot change thread priority with pthread_setschedparam
    if (res != EPERM) {
#endif // defined(ANDROID)

    PRINT_NAMED_ERROR("SetThreadPriority.Failed",
                      "Error: %s (res=%d) setting thread policy:priority %d:%d",
                      std::strerror(errno), res, policy, priority);

#if defined(ANDROID)
    }
#endif
  }
  else
  {
    PRINT_NAMED_INFO("SetThreadPriority.Success",
                      "Changed thread policy:priority from %d:%d to %d:%d)", oldPolicy, oldPriority, policy, priority);    
  }
}
  
  
void SetThreadPriority(std::thread& inThread, ThreadPriority threadPriority)
{
  SetThreadPriority(inThread.native_handle(), threadPriority);
}

// Set thread name
// on osx/ios it only sets the name if it is called from the target thread
// on linux/android it works as implied
// on success returns true
bool SetThreadName(std::thread::native_handle_type inThread, const char* threadName)
{
  if (threadName == nullptr) {
    return false;
  }
  
  // The thread name length is restricted to 16 characters,including the terminating null byte ('\0').
  // A C string is as long as the number of characters between the beginning of the string and the terminating null
  // character (without including the terminating null character itself).
  ASSERT_NAMED(strlen(threadName) < 16, "SetThreadPriority.NameTooLong");
  
  // RETURN VALUE - On success, these functions return 0; on error, they return a nonzero  error number.
  int returnValue = 0;
#if defined(LINUX) || defined(ANDROID) || defined(VICOS)
  returnValue = pthread_setname_np(inThread, threadName);
#else
  // Mac OS X: must be set from within the thread (can't specify thread ID)
  // int pthread_setname_np(const char*);
  // verify that we are on the target thread.. if not exit with error
  if (ANKI_VERIFY(pthread_self() == inThread, "SetThreadPriority.NotCalledFromTargetThread",
                  "Must be called from target thread on osx")) {
      returnValue = pthread_setname_np(threadName);
  }  else returnValue = 1;
#endif
  return returnValue == 0;
}

}
}

