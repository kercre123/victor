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

#ifndef UNIT_TEST
#ifdef __cplusplus
extern "C" {
#endif

#ifdef USING_MOVIDIUS_GCC_COMPILER
  void _Anki_Logf(int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
#if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
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

    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, "LOG[%d] - ", logLevel);
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, file);
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, "@");
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, "%d - ", line);
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, eventName);
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, " - ");
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, eventValue);
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, "\n");
#endif // #if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
  }

  void _Anki_Log(int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
#if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
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

    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, "LOG[%d] - ", logLevel);
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, file);
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, "@");
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, "%d - ", line);
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, eventName);
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, " - ");
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, eventValue);
    explicitPrintf(0,EXPLICIT_PRINTF_FLIP_CHARACTERS, "\n");
#endif // #if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
  }
#else // #ifdef USING_MOVIDIUS_GCC_COMPILER

#define Anki_STRING_LENGTH 1024
  static char renderedLogString[Anki_STRING_LENGTH];

#pragma warning(push)
#pragma warning(disable: 4100) // Unreference formal parameter
  void _Anki_Logf(int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
#if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
    va_list argList;
    va_start(argList, line);
    vsnprintf(renderedLogString, Anki_STRING_LENGTH, eventValue, argList);
    va_end(argList);
    _Anki_Log(logLevel, eventName, renderedLogString, file, funct, line, NULL);
    //fflush(stdout);
#endif // #if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
  }

  void _Anki_Log(int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
#if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
    va_list argList;
    va_start(argList, line);

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
    vprintf(eventValue, argList);
    printf("\n");
    va_end(argList);
    //fflush(stdout);
#endif // #if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
  }
#pragma warning(pop)

#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER

#ifdef __cplusplus
} // extern "C"
#endif
#endif