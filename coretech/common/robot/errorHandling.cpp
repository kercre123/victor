/**
File: errorHandling.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

//  errorHandling.cpp is based off DAS.cpp

#include "coretech/common/robot/errorHandling.h"
#include "coretech/common/robot/utilities_c.h"

#include "coretech/common/shared/utilities_shared.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100) // Unreference formal parameter
#endif

  static bool isLogSilent_ = false;

  void SetLogSilence(const bool isLogSilent)
  {
    isLogSilent_ = isLogSilent;
  }
  
  inline static bool ShouldLog(int logLevel)
  {
    if(isLogSilent_)
    {
      return false;
    }
    
    using namespace Anki::Embedded;
    switch(static_cast<LogLevel>(logLevel))
    {
      case ANKI_LOG_LEVEL_DEBUG:
      case ANKI_LOG_LEVEL_INFO:
      case ANKI_LOG_LEVEL_EVENT:
        return (ANKI_DEBUG_LEVEL <= ANKI_DEBUG_ALL);
        
      case ANKI_LOG_LEVEL_WARN:
        return (ANKI_DEBUG_LEVEL <= ANKI_DEBUG_ERRORS_AND_WARNS);
        
      case ANKI_LOG_LEVEL_ASSERT:
        return (ANKI_DEBUG_LEVEL <= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS);
        
      case ANKI_LOG_LEVEL_ERROR:
      case ANKI_LOG_LEVEL_NUMLEVELS:
        return (ANKI_DEBUG_LEVEL <= ANKI_DEBUG_ERRORS);
    }
  }
  
  void _Anki_Log(int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
    if(ShouldLog(logLevel)) {
      va_list argList;

      // Don't print all the path of the file
      s32 lastSlashIndex = -1;
      for(s32 i=0; i<10000; i++) {
        if(file[i] == '\\' ||
          file[i] == '/') {
            lastSlashIndex = i;
        } else if(!file[i]) {
          break;
        }
      }

      if(lastSlashIndex >= 0) {
        file += lastSlashIndex + 1;
      }

      Anki::CoreTechPrint("LOG[%d] - %s@%d - %s - ", logLevel, file, line, eventName);

      va_start(argList, line);
      Anki::CoreTechPrint(eventValue, argList);
      va_end(argList);

      Anki::CoreTechPrint("\n");
      //fflush(stdout);
    }
  }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __cplusplus
} // extern "C"
#endif
