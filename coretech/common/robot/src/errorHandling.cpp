/**
File: errorHandling.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

//  errorHandling.cpp is based off DAS.cpp

#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/utilities_c.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100) // Unreference formal parameter
#endif

  void _Anki_Log(int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
#if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
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

    printf("LOG[%d] - %s@%d - %s - ", logLevel, file, line, eventName);

    va_start(argList, line);
    vprintf(eventValue, argList);
    va_end(argList);

    printf("\n");
    //fflush(stdout);
#endif // #if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
  }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __cplusplus
} // extern "C"
#endif
