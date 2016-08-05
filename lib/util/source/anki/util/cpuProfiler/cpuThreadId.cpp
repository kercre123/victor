/**
 * File: cpuThreadId
 *
 * Author: Mark Wesley
 * Created: 07/30/16
 *
 * Description: Support for ThreadId (mostly just a wrapper around underlying support from thread libraries)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "util/cpuProfiler/cpuThreadId.h"
#include <pthread.h>


namespace Anki {
namespace Util {

  
extern const CpuThreadId kCpuThreadIdInvalid = CpuThreadId();
  
  
bool GetCurrentThreadName(char* outName, size_t outNameCapacity)
{
  #if defined(ANDROID)
  int res = 0;
  snprintf(outName, outNameCapacity, "UnsupportedFixme");
  #else
  int	res = pthread_getname_np(pthread_self(), outName, outNameCapacity);
  #endif
  
  const bool success = (res == 0);
  if (!success)
  {
    snprintf(outName, outNameCapacity, "ERROR");
  }
  return success;
}
  
  
std::string GetCurrentThreadName()
{
  std::string threadName;
  char tempBuff[128];
  if (GetCurrentThreadName(tempBuff, sizeof(tempBuff)))
  {
    threadName = tempBuff;
  }
  else
  {
    threadName = "ERROR";
  }

  return threadName;
}
  
  
  
} // end namespace Util
} // end namespace Anki



