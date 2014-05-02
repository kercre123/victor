/**
File: utilities.h
Author: Kevin Yoon
Created: 2014

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_SHARED_UTILITIES_H_
#define _ANKICORETECHEMBEDDED_COMMON_SHARED_UTILITIES_H_

#define CORETECH_PRINT(...) CoreTechPrint(__VA_ARGS__)

#include <stdarg.h>

namespace Anki
{
  // For printing throughout Coretech libraries.
  // Calls printf() by default, but can be made to use an externally
  // defined function via SetCoreTechPrintFunctionPtr().
  int CoreTechPrint(const char * format, ...);
  
  // Sets the function pointer that CoreTechPrint() uses.
  // Convenient for on-robot print which could be more complex
  // than just printf().
  void SetCoreTechPrintFunctionPtr( int (*fp)(const char * format, va_list) = 0);
    
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_SHARED_UTILITIES_H_
