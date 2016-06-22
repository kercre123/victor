/**
 * File: callstack
 *
 * Author: raul
 * Created: 06/18/2016
 *
 * Description: Functions related to debugging the callstack.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "callstack.h"
#include "util/logging/logging.h"

#include <cstdlib>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// appropriate headers per platform
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(__MACH__) && defined(__APPLE__)
#include <execinfo.h>
#elif defined(ANDROID) || defined(LINUX)

#else
#error "Unsupported platform"
#endif

namespace Anki {
namespace Util {

void sDumpCallstack(const std::string& name)
{
  #if defined(__MACH__) && defined(__APPLE__)
  {
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    for (int i = 0; i < frames; ++i)
    {
      PRINT_NAMED_INFO(name.c_str(), "%s", strs[i]);
    }
    free(strs);
  }
  #else
  
  //TODO implement android

  #endif
}

} // namespace Util
} // namespace Anki
