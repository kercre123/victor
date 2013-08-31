//
//  DAS.cpp
//  BaseStation
//
//  Created by damjan stulic on 1/2/13.
//  DASlight.cpp is based off DAS.cpp
//  Copyright (c) 2013 Anki. All rights reserved.
//

#include "anki/embeddedCommon.h"

#include <stdarg.h>
#include <stdio.h>

#ifndef UNIT_TEST
#ifdef __cplusplus
extern "C" {
#endif

#ifdef USING_MOVIDIUS_GCC_COMPILER
  void _DAS_Logf(DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
  }

  void _DAS_Log(DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
  }
#else // #ifdef USING_MOVIDIUS_GCC_COMPILER
#define DAS_STRING_LENGTH 256
  static char renderedLogString[DAS_STRING_LENGTH];

#pragma warning(push)
#pragma warning(disable: 4100) // Unreference formal parameter
  void _DAS_Logf(DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
    va_list argList;
    va_start(argList, line);
    vsnprintf(renderedLogString, DAS_STRING_LENGTH, eventValue, argList);
    va_end(argList);
    _DAS_Log(level, eventName, renderedLogString, file, funct, line, NULL);
    //fflush(stdout);
  }

  void _DAS_Log(DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
    va_list argList;
    va_start(argList, line);
    printf("LOG[%d] - %s - ", level, eventName);
    vprintf(eventValue, argList);
    printf("\n");
    va_end(argList);
    //fflush(stdout);
  }
#pragma warning(pop)

#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER

#ifdef __cplusplus
} // extern "C"
#endif
#endif