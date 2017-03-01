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


#ifndef __Util_CpuProfiler_CpuThreadId_H__
#define __Util_CpuProfiler_CpuThreadId_H__


#include <string>
#include <thread>


namespace Anki {
namespace Util {


using CpuThreadId = std::thread::id;
extern const CpuThreadId kCpuThreadIdInvalid;


inline CpuThreadId GetCurrentThreadId()
{
  return std::this_thread::get_id();
}
  
  
bool GetCurrentThreadName(char* outName, size_t outNameCapacity);
std::string GetCurrentThreadName();


inline bool AreCpuThreadIdsEqual(CpuThreadId threadId1, CpuThreadId threadId2)
{
  return threadId1 == threadId2;
}

  
} // end namespace Util
} // end namespace Anki


#endif // __Util_CpuProfiler_CpuThreadId_H__
