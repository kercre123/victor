//  errorHandling.cpp is based off DAS.cpp

#include "anki/embeddedCommon/errorHandling.h"

#include <stdarg.h>
#include <stdio.h>

#ifndef UNIT_TEST
#ifdef __cplusplus
extern "C" {
#endif

#define Anki_STRING_LENGTH 256
  static char renderedLogString[Anki_STRING_LENGTH];

#ifdef USING_MOVIDIUS_GCC_COMPILER
  IN_DDR void _Anki_Logf(int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
#if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
    printf("LOG[%d] - %s - ", logLevel, eventName);
    printf(eventValue);
    printf("\n");
#endif // #if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
  }

  IN_DDR void _Anki_Log(int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
#if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
    printf("LOG[%d] - %s - ", logLevel, eventName);
    printf(eventValue);
    printf("\n");
#endif // #if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
  }
#else // #ifdef USING_MOVIDIUS_GCC_COMPILER

#pragma warning(push)
#pragma warning(disable: 4100) // Unreference formal parameter
  IN_DDR void _Anki_Logf(int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
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

  IN_DDR void _Anki_Log(int logLevel, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
#if ANKI_OUTPUT_DEBUG_LEVEL == ANKI_OUTPUT_DEBUG_PRINTF
    va_list argList;
    va_start(argList, line);
    printf("LOG[%d] - %s - ", logLevel, eventName);
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