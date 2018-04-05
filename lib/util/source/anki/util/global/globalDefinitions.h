/**
 * File: globalDefinitions.h
 *
 * Author: raul
 * Created: 07/16/2014
 *
 * Description: Global definitions to be included in all or a large amount of files. Keep content to a minimum, please.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __Util_Global_GlobalDefinitions_H__
#define __Util_Global_GlobalDefinitions_H__

#include "util/export/export.h"
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Use ANKI_DEVELOPER_CODE to strip down code that should only be available to developers in debug, but not for
// testing in release, nor available to players.
// Use ANKI_DEV_CHEATS to strip down code that should be available to developers in debug and testing in release,
// but not available to players.
// Use ANKI_PRIVACY_GUARD and HidePersonallyIdentifiableInfo() for anything that should not be present in shipping mode,
// such as players' names in logs. NOTE: This is a separate flag from DEV_CHEATS to make it easier to find use cases in
// the code and in case we want to set it differently via other build flags later without changing code.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(DEBUG)
  #define ANKI_DEVELOPER_CODE     1
  #define ANKI_DEV_CHEATS         1
  #define ANKI_PROFILING_ENABLED  1
  #define ANKI_PRIVACY_GUARD      0 // PII displayed in debug logs!!!
#elif defined(RELEASE)
  #define ANKI_DEVELOPER_CODE     0
  #define ANKI_DEV_CHEATS         0
  #define ANKI_PROFILING_ENABLED  0
  #define ANKI_PRIVACY_GUARD      1 // PII displayed in non-shipping release logs!!!
#elif defined(SHIPPING)
  #define ANKI_DEVELOPER_CODE     0
  #define ANKI_DEV_CHEATS         0
  #define ANKI_PROFILING_ENABLED  0
  #define ANKI_PRIVACY_GUARD      1 // PII redacted in shipping logs
#else
  #error "You must define DEBUG, RELEASE, or SHIPPING"
#endif


#if ANKI_DEVELOPER_CODE
  #define ANKI_DEVELOPER_CODE_ONLY(expr)      expr
  #define ANKI_NON_DEVELOPER_CODE_ONLY(expr)
  #define ANKI_NON_DEVELOPER_CONSTEXPR
#else
  #define ANKI_DEVELOPER_CODE_ONLY(expr)
  #define ANKI_NON_DEVELOPER_CODE_ONLY(expr)  expr
  #define ANKI_NON_DEVELOPER_CONSTEXPR        constexpr  // Function can be constexpr as e.g. Dev asserts are disabled
#endif


namespace Anki {
namespace Util {

  // Simply returns given string unless ANKI_PRIVACY_GUARD is enabled, in which case "<PII>" is returned.
  const char * HidePersonallyIdentifiableInfo(const char* str);

}
}


ANKI_C_BEGIN

ANKI_EXPORT bool  NativeAnkiUtilAreDevFeaturesEnabled();

ANKI_C_END

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ...
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif // __Util_Global_GlobalDefinitions_H__
