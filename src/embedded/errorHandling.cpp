//  errorHandling.cpp is based off DAS.cpp

#include "anki/embeddedCommon.h"

#include <stdarg.h>
#include <stdio.h>

#ifndef UNIT_TEST
#ifdef __cplusplus
extern "C" {
#endif

#define Anki_STRING_LENGTH 256
  static char renderedLogString[Anki_STRING_LENGTH];

#ifdef USING_MOVIDIUS_GCC_COMPILER
  IN_DDR void _Anki_Logf(const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
    printf("LOG - %s - ", eventName);
    printf(eventValue);
    printf("\n");
  }

  IN_DDR void _Anki_Log(const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
    printf("LOG - %s - ", eventName);
    printf(eventValue);
    printf("\n");
  }
#else // #ifdef USING_MOVIDIUS_GCC_COMPILER

#pragma warning(push)
#pragma warning(disable: 4100) // Unreference formal parameter
  IN_DDR void _Anki_Logf(const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
    va_list argList;
    va_start(argList, line);
    vsnprintf(renderedLogString, Anki_STRING_LENGTH, eventValue, argList);
    va_end(argList);
    _Anki_Log(eventName, renderedLogString, file, funct, line, NULL);
    //fflush(stdout);
  }

  IN_DDR void _Anki_Log(const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...)
  {
    va_list argList;
    va_start(argList, line);
    printf("LOG - %s - ", eventName);
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