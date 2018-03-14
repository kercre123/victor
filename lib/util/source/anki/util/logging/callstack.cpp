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
#include "util/string/stringHelpers.h"

#include <cstdlib>

#include <string>
#include <sstream>
#include <vector>
#include <assert.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// appropriate headers per platform
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(__MACH__) && defined(__APPLE__)
#include <execinfo.h>
#include <cxxabi.h>
#elif defined(ANDROID) || defined(LINUX) || defined(VICOS)

#else
#error "Unsupported platform"
#endif

namespace Anki {
namespace Util {

#if defined(__MACH__) && defined(__APPLE__)
std::string DemangleBacktraceSymbols(const std::string& backtraceFrame) {
  std::vector<std::string> splitFrame = SplitString(backtraceFrame, ' ');

  // -4 because the valid statuses from __cxa_demangle() is 0, -1, -2, -3. If we got -4 after
  // calling __cxa_demangle() something very wrong happened.
  int status = -4;

  // Make sure that when the backtrace frame string is split 
  // by spaces there is at least 3 items in there.
  if (splitFrame.size() > 3) {
    // A backtraceFrame looks like:
    // "0   webotsCtrlGameEngine                0x000000010cc84894 _ZN4Anki4Util14sDumpCallstackERKNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE + 52"
    // The mangled symbol is the third element from the back, when split by ' '
    // 
    // Use a tempPtr here in case __cxa_demangle cannot demangle the given string and returns a nullptr.
    char* tempPtr = abi::__cxa_demangle(splitFrame.end()[-3].c_str(), 0, 0, &status);

    switch (status) {
      case 0:
        assert(tempPtr != nullptr);
        // Everything is fine, proceed to change the mangled name into the demangled version.
        splitFrame.end()[-3] = tempPtr;
        break;

      case -2:
      {
        // Demangle didn't work, don't change the name.
        PRINT_NAMED_DEBUG("Callstack.DemangleBacktraceSymbols",
                          "%s is not a valid name under the C++ ABI mangling rules.",
                          splitFrame.end()[-3].c_str());
        break;
      }

      default:
      {
        // Demangle didn't work, don't change the name.
        PRINT_NAMED_WARNING("Callstack.DemangleBacktraceSymbols",
                            "Couldn't demangle the symbol, __cxa_demangle returned status = %i", status);
        break;
      }
    }
    free(tempPtr);
  } else {
    PRINT_NAMED_WARNING("Callstack.DemangleBacktraceSymbols",
                        "Something is wrong with the format of the backtrace frame. It should look "
                        "something like "
                        "'0   webotsCtrlGameEngine                0x000000010cc84894 _ZN4Anki4Util14sDumpCallstackERKNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE + 52'");
  }

  std::ostringstream os;
  for (const std::string& part : splitFrame) {
    os << part << " ";
  }

  const std::string& ret(os.str());
  return ret;
}
#endif  // defined(__MACH__) && defined(__APPLE__)

void sDumpCallstack(const std::string& name)
{
  #if defined(__MACH__) && defined(__APPLE__)
  {
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    for (int i = 0; i < frames; ++i) {
      PRINT_CH_INFO("Unfiltered", name.c_str(), "%s", DemangleBacktraceSymbols(strs[i]).c_str());
    }
    free(strs);
  }
  #else
  
  //TODO implement android

  #endif
}

} // namespace Util
} // namespace Anki
