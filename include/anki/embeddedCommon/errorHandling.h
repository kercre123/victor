//  errorHandling.h is based off Das.h

#ifndef __Anki_LIGHT_H__
#define __Anki_LIGHT_H__

#include "anki/embeddedCommon/config.h"

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
      ANKI_LOG_LEVEL_DEBUG = 0,
      ANKI_LOG_LEVEL_INFO = 1,
      ANKI_LOG_LEVEL_EVENT = 2,
      ANKI_LOG_LEVEL_WARN = 3,
      ANKI_LOG_LEVEL_ERROR = 4,
      ANKI_LOG_LEVEL_ESSENTIAL = 5,
      ANKI_LOG_LEVEL_NUMLEVELS = 6
    } LogLevel;
  } // namespace Embedded
} //namespace Anki

#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_ESSENTIAL

#define AnkiWarn(expression, eventName, eventValue_format, ...)

#define AnkiConditionalWarn(expression, eventName, eventValue_format, ...)

#define AnkiConditionalWarnAndReturn(expression, eventName, eventValue_format, ...)

#define AnkiConditionalWarnAndReturnValue(expression, returnValue, eventName, eventValue_format, ...)

#define AnkiError(eventName, eventValue_format, ...)

#define AnkiConditionalError(expression, eventName, eventValue_format, ...)

#define AnkiConditionalErrorAndReturn(expression, eventName, eventValue_format, ...)

#define AnkiConditionalErrorAndReturnValue(expression, returnValue, eventName, eventValue_format, ...)

#define AnkiConditionalEssentialAndReturn(expression, eventName, eventValue_format, ...) \
  if(!(expression)) {\
  return;\
  }

#define AnkiConditionalEssentialAndReturnValue(expression, returnValue, eventName, eventValue_format, ...) \
  if(!(expression)) { \
  return returnValue;\
  }

#elif ANKI_DEBUG_LEVEL == ANKI_DEBUG_ESSENTIAL_AND_ERROR

#define AnkiWarn(expression, eventName, eventValue_format, ...)

#define AnkiConditionalWarn(expression, eventName, eventValue_format, ...)

#define AnkiConditionalWarnAndReturn(expression, eventName, eventValue_format, ...)

#define AnkiConditionalWarnAndReturnValue(expression, returnValue, eventName, eventValue_format, ...)

#define AnkiError(eventName, eventValue_format, ...) _Anki_Logf(ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);

#define AnkiConditionalError(expression, eventName, eventValue_format, ...) \
  if(!(expression)) {\
  _Anki_Logf(ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  }

#define AnkiConditionalErrorAndReturn(expression, eventName, eventValue_format, ...) \
  if(!(expression)) {\
  _Anki_Logf(ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return;\
  }

#define AnkiConditionalErrorAndReturnValue(expression, returnValue, eventName, eventValue_format, ...) \
  if(!(expression)) { \
  _Anki_Logf(ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return returnValue;\
  }

#define AnkiConditionalEssentialAndReturn(expression, eventName, eventValue_format, ...) \
  if(!(expression)) {\
  _Anki_Logf(ANKI_LOG_LEVEL_ESSENTIAL, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return;\
  }

#define AnkiConditionalEssentialAndReturnValue(expression, returnValue, eventName, eventValue_format, ...) \
  if(!(expression)) { \
  _Anki_Logf(ANKI_LOG_LEVEL_ESSENTIAL, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return returnValue;\
  }

#elif (ANKI_DEBUG_LEVEL == ANKI_DEBUG_ESSENTIAL_AND_ERROR_AND_WARN) || (ANKI_DEBUG_LEVEL == ANKI_DEBUG_ALL)

#define AnkiWarn(eventName, eventValue_format, ...) \
  if(!(expression)) { _Anki_Logf(ANKI_LOG_LEVEL_WARN, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }

#define AnkiConditionalWarn(expression, eventName, eventValue_format, ...) \
  if(!(expression)) { _Anki_Logf(ANKI_LOG_LEVEL_WARN, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }

#define AnkiConditionalWarnAndReturn(expression, eventName, eventValue_format, ...) \
  if(!(expression)) {\
  _Anki_Logf(ANKI_LOG_LEVEL_WARN, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return;\
  }

#define AnkiConditionalWarnAndReturnValue(expression, returnValue, eventName, eventValue_format, ...)\
  if(!(expression)) { \
  _Anki_Logf(ANKI_LOG_LEVEL_WARN, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return returnValue;\
  }

#define AnkiError(eventName, eventValue_format, ...) _Anki_Logf(ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);

#define AnkiConditionalError(expression, eventName, eventValue_format, ...) \
  if(!(expression)) { \
  _Anki_Logf(ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  }

#define AnkiConditionalErrorAndReturn(expression, eventName, eventValue_format, ...) \
  if(!(expression)) {\
  _Anki_Logf(ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return;\
  }

#define AnkiConditionalErrorAndReturnValue(expression, returnValue, eventName, eventValue_format, ...) \
  if(!(expression)) { \
  _Anki_Logf(ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return returnValue;\
  }

#define AnkiConditionalEssentialAndReturn(expression, eventName, eventValue_format, ...) \
  if(!(expression)) {\
  _Anki_Logf(ANKI_LOG_LEVEL_ESSENTIAL, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return;\
  }

#define AnkiConditionalEssentialAndReturnValue(expression, returnValue, eventName, eventValue_format, ...) \
  if(!(expression)) { \
  _Anki_Logf(ANKI_LOG_LEVEL_ESSENTIAL, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
  return returnValue;\
  }
#endif

#ifdef __cplusplus
extern "C" {
#endif

  void _Anki_Logf(int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...);
  void _Anki_Log (int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* defined(__Anki_LIGHT_H__) */
