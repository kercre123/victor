/**
 * File: boundedWhile
 *
 * Author: damjan
 * Created: 5/7/14
 *
 * Description: helper for bounding while loops
 *
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef UTIL_BOUNDEDWHILE_H_
#define UTIL_BOUNDEDWHILE_H_

#include <stdexcept>
#include "util/logging/logging.h"

#define BOUNDED_WHILE(n, exp) unsigned int MAKE_NAME=0; while(MAKE_NAME++ < (n) ? (exp) : Anki::Util::util_PrintErrorAndCrashInDev(__FILE__, __LINE__))

#define SOFT_BOUNDED_WHILE(str, n, exp) unsigned int MAKE_NAME=0; while(MAKE_NAME++ < (n) ? (exp) : Anki::Util::util_PrintWarning(str))

namespace Anki{ namespace Util
{


// macro hacking stuff
#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )

// a while loop that executes a limited number of execution (throws exception)
#define MAKE_NAME MACRO_CONCAT(_bvar_, __LINE__)

inline bool util_PrintWarning(const char* errorString) {
  PRINT_NAMED_WARNING("LoopBoundOverflow", "%s", errorString);
  return false;
}

inline bool util_PrintErrorAndCrashInDev(const char* file, int line) {
  PRINT_NAMED_ERROR("LoopBoundOverflow", "%s:%d", file, line);
  if( ANKI_DEVELOPER_CODE ) { // dont crash prod
    throw std::runtime_error("LoopBoundOverflow");
  }
  return false;
}


} // end namespace Anki
} // end namespace Util

#endif //UTIL_BOUNDEDWHILE_H_
