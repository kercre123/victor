/**
File: errorHandling.h
Author: Peter Barnum
Created: 2013

errorHandling.h is based off Das.h
It acts as a low-level interface to error logging and reporting.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

// TODO: For the love of all that is holy, use our normal error/logging macros! (VIC-4941)

#ifndef _ANKICORETECHEMBEDDED_COMMON_ERROR_HANDLING_H_
#define _ANKICORETECHEMBEDDED_COMMON_ERROR_HANDLING_H_

#include "coretech/common/robot/config.h"

#ifdef _MSC_VER
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif
#endif

namespace Anki
{
  namespace Embedded
  {
    typedef enum LogLevel {
      ANKI_LOG_LEVEL_DEBUG     = 0,
      ANKI_LOG_LEVEL_INFO      = 1,
      ANKI_LOG_LEVEL_EVENT     = 2,
      ANKI_LOG_LEVEL_WARN      = 3,
      ANKI_LOG_LEVEL_ASSERT    = 4,
      ANKI_LOG_LEVEL_ERROR     = 5,
      ANKI_LOG_LEVEL_NUMLEVELS = 6
    } LogLevel;
  } // namespace Embedded
} //namespace Anki

//
// Error checking
//

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS
#define AnkiError(eventName, eventValue_format, ...) _Anki_Log(::Anki::Embedded::ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);

#define AnkiConditionalError(expression, eventName, eventValue_format, ...) \
  if(!(expression)) { \
  _Anki_Log(::Anki::Embedded::ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  }

#define AnkiConditionalErrorAndReturn(expression, eventName, eventValue_format, ...) \
  if(!(expression)) {\
  _Anki_Log(::Anki::Embedded::ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return;\
  }

#define AnkiConditionalErrorAndReturnValue(expression, returnValue, eventName, eventValue_format, ...) \
  if(!(expression)) { \
  _Anki_Log(::Anki::Embedded::ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return returnValue;\
  }
#else

#define AnkiError(eventName, eventValue_format, ...)
#define AnkiConditionalError(expression, eventName, eventValue_format, ...)
#define AnkiConditionalErrorAndReturn(expression, eventName, eventValue_format, ...)
#define AnkiConditionalErrorAndReturnValue(expression, returnValue, eventName, eventValue_format, ...)

#endif

//
// Warn checking
//

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS

#define AnkiWarn(eventName, eventValue_format, ...) \
{ _Anki_Log(::Anki::Embedded::ANKI_LOG_LEVEL_WARN, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }

#define AnkiConditionalWarn(expression, eventName, eventValue_format, ...) \
  if(!(expression)) { _Anki_Log(::Anki::Embedded::ANKI_LOG_LEVEL_WARN, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }

#define AnkiConditionalWarnAndReturn(expression, eventName, eventValue_format, ...) \
  if(!(expression)) { \
  _Anki_Log(::Anki::Embedded::ANKI_LOG_LEVEL_WARN, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return;\
  }

#define AnkiConditionalWarnAndReturnValue(expression, returnValue, eventName, eventValue_format, ...)\
  if(!(expression)) { \
  _Anki_Log(::Anki::Embedded::ANKI_LOG_LEVEL_WARN, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return returnValue;\
  }

#else

#define AnkiWarn(eventName, eventValue_format, ...)
#define AnkiConditionalWarn(expression, eventName, eventValue_format, ...)
#define AnkiConditionalWarnAndReturn(expression, eventName, eventValue_format, ...)
#define AnkiConditionalWarnAndReturnValue(expression, returnValue, eventName, eventValue_format, ...)

#endif

//
// Assert checking
//

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS

#define AnkiAssert(expression) \
  if(!(expression)) { \
  _Anki_Log(::Anki::Embedded::ANKI_LOG_LEVEL_ASSERT, "Assert failure", "", __FILE__, __PRETTY_FUNCTION__, __LINE__); \
  assert(false); \
  }

#else

#define AnkiAssert(expression)

#endif

#ifdef __cplusplus
extern "C" {
#endif

  void _Anki_Log (int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...);

  // If true, AnkiError, AnkiWarn, and AnkiLog won't output printfs
  void SetLogSilence(const bool isLogSilent);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_ERROR_HANDLING_H_
