/**
File: utilities.cpp
Author: Kevin Yoon
Created: 2014

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include <stdio.h>
#include "anki/common/shared/utilities_shared.h"

namespace Anki
{
    namespace {
      int (*CoreTechPrintFunc)(const char * format, va_list) = vprintf;
    }
    
    int CoreTechPrint(const char * format, ...)
    {
      int printed = 0;
      if (CoreTechPrintFunc) {
        va_list ap;
        va_start(ap, format);
        printed = (*CoreTechPrintFunc)(format, ap);
        va_end(ap);
      }
      return printed;
    }

    int CoreTechPrint(const char * format, va_list argList)
    {
      int printed = 0;
      if (CoreTechPrintFunc) {
        printed = (*CoreTechPrintFunc)(format, argList);
      }
      return printed;
    }
    
    void SetCoreTechPrintFunctionPtr( int (*fp)(const char * format, va_list))
    {
      CoreTechPrintFunc = fp;
    }
  
} // namespace Anki
