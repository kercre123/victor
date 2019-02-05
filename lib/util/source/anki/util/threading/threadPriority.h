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


#ifndef __Util_Threading_ThreadPriority_H__
#define __Util_Threading_ThreadPriority_H__


#include <stdint.h>
#include <thread>


namespace Anki {
namespace Util {


enum class ThreadPriority : uint8_t
{
  Min = 0,
  Low,
  Default,
  High,
  Max,
};
  
  
void SetThreadPriority(std::thread& inThread, ThreadPriority threadPriority);

// Set thread name, which must be less than 16 characters long.
// on osx/ios it only sets the name if it is called from the target thread
// on linux/android it works as implied
// on success returns true
bool SetThreadName(std::thread::native_handle_type inThread, const char* threadName);

// Same as above, but auto-truncates the name to be less than 16 characters as needed
bool SetThreadName(std::thread::native_handle_type inThread, const std::string& threadName);
  
} // namespace Util
} // namespace Anki


#endif // __Util_Threading_ThreadPriority_H__
