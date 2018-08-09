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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(NDEBUG)
  #ifndef ANKI_DEVELOPER_CODE
    #define ANKI_DEVELOPER_CODE     0
  #endif
  #ifndef ANKI_DEV_CHEATS
    #define ANKI_DEV_CHEATS         1 // This is the expected behavior on "release" builds, overridden with command-line defines for shipping builds
  #endif
  #ifndef ANKI_PROFILING_ENABLED
    #define ANKI_PROFILING_ENABLED  1
  #endif
#else
  #define ANKI_DEVELOPER_CODE     1
  #define ANKI_DEV_CHEATS         1
  #define ANKI_PROFILING_ENABLED  1
#endif

//
// Use ANKI_PRIVACY_GUARD and HidePersonallyIdentifiableInfo() for anything that should not be present in shipping mode,
// such as players' names in logs. NOTE: This is a separate flag from DEV_CHEATS to make it easier to find use cases in
// the code and in case we want to set it differently via other build flags later without changing code.
//
// ANKI_PRIVACY_GUARD is OFF by default for debug and release builds.
// This means PII will be displayed for developers and QA testing.
//
// Privacy guard must be enabled with -DANKI_PRIVACY_GUARD=1 for shipping builds.
//
#ifndef ANKI_PRIVACY_GUARD
#define ANKI_PRIVACY_GUARD 0
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
  inline const char * HidePersonallyIdentifiableInfo(const char* str);



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// inlined definitions ...

const char * HidePersonallyIdentifiableInfo(const char* str)
{
  #if ANKI_PRIVACY_GUARD
  {
    static const char * const kPrivacyString = "<PII>";
    return kPrivacyString;
  }
  #else
  {
    return str;
  }
  #endif // ANKI_PRIVACY_GUARD
}

}
}


ANKI_C_BEGIN

ANKI_EXPORT bool  NativeAnkiUtilAreDevFeaturesEnabled();

ANKI_C_END

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ...
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif // __Util_Global_GlobalDefinitions_H__
