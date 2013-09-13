//
//  DASlight.h
//  TestLogging
//
//  DAS.h Created by Mark Pauley on 8/13/12.
//  DASlight.h is based off DAS.h
//  Copyright (c) 2012 Mark Pauley. All rights reserved.
//

#ifndef __DAS_LIGHT_H__
#define __DAS_LIGHT_H__

#include "anki/embeddedCommon/config.h"

#ifdef _MSC_VER
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif
#endif

typedef enum DASLogLevel {
  DASLogLevel_Debug, // Sent to server in development by default
  DASLogLevel_Info,  // Sent to server in development by default
  DASLogLevel_Event, // Sent to server in production by default
  DASLogLevel_Warn,  // Sent to server in production by default
  DASLogLevel_Error,  // Sent to server in production by default
  DASLogLevel_NumLevels
} DASLogLevel;

// Plain old, vanilla logging. eventValue is a printf-style format specifier.  Defaults to INFO level.
// DASEvent("ClassName.eventName", "Value1 = %d, Value2 = %d", value1, value2); or something like that

#define DASDebug(eventName, eventValue_format, ...) _DAS_Logf(DASLogLevel_Debug, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);

#define DASInfo(eventName, eventValue_format, ...)  _DAS_Logf(DASLogLevel_Info, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);

#define DASEvent(eventName, eventValue_format, ...) _DAS_Logf(DASLogLevel_Event, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);
#define DASEventAndReturn(eventName, eventValue_format, ...) { _DAS_Logf(DASLogLevel_Event, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); return;}
#define DASEventAndReturnValue(returnValue, eventName, eventValue_format, ...) { _DAS_Logf(DASLogLevel_Event, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); return returnValue;}

#define DASConditionalEvent(expression, eventName, eventValue_format, ...) if(!(expression)) { _DAS_Logf(DASLogLevel_Event, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }
#define DASConditionalEventAndReturn(expression, eventName, eventValue_format, ...) if(!(expression)) { _DAS_Logf(DASLogLevel_Event, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); return;}
#define DASConditionalEventAndReturnValue(expression, returnValue, eventName, eventValue_format, ...) if(!(expression)) { _DAS_Logf(DASLogLevel_Event, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); return returnValue;}

#define DASWarn(eventName, eventValue_format, ...)  _DAS_Logf(DASLogLevel_Warn, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);
#define DASWarnAndReturn(eventName, eventValue_format, ...) { _DAS_Logf(DASLogLevel_Warn, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); return;}
#define DASWarnAndReturnValue(returnValue, eventName, eventValue_format, ...) { _DAS_Logf(DASLogLevel_Warn, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); return returnValue;}

#define DASConditionalWarn(expression, eventName, eventValue_format, ...) if(!(expression)) { _DAS_Logf(DASLogLevel_Warn, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }
#define DASConditionalWarnAndReturn(expression, eventName, eventValue_format, ...) if(!(expression)) { _DAS_Logf(DASLogLevel_Warn, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); return;}
#define DASConditionalWarnAndReturnValue(expression, returnValue, eventName, eventValue_format, ...) if(!(expression)) { _DAS_Logf(DASLogLevel_Warn, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); return returnValue;}

#define DASError(eventName, eventValue_format, ...) _DAS_Logf(DASLogLevel_Error, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);
#define DASErrorAndReturn(eventName, eventValue_format, ...) { _DAS_Logf(DASLogLevel_Error, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); return;}
#define DASErrorAndReturnValue(returnValue, eventName, eventValue_format, ...) { _DAS_Logf(DASLogLevel_Error, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); return returnValue;}

#define DASConditionalError(expression, eventName, eventValue_format, ...) if(!(expression)) { _DAS_Logf(DASLogLevel_Error, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }
#define DASConditionalErrorAndReturn(expression, eventName, eventValue_format, ...) if(!(expression)) { _DAS_Logf(DASLogLevel_Error, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); return;}
#define DASConditionalErrorAndReturnValue(expression, returnValue, eventName, eventValue_format, ...) if(!(expression)) { _DAS_Logf(DASLogLevel_Error, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); return returnValue;}

#ifdef __cplusplus
extern "C" {
#endif

  void _DAS_Logf(DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...);
  void _DAS_Log (DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* defined(__DAS_LIGHT_H__) */
