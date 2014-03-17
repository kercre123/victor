/**
File: utilities.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/utilities.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/array2d.h"

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

#define PrintfOneArray_FORMAT_STRING_2 "%d %d "
#define PrintfOneArray_FORMAT_STRING_1 "%d "

namespace Anki
{
  namespace Embedded
  {
#if ANKICORETECH_EMBEDDED_USE_OPENCV
    int ConvertToOpenCvType(const char *typeName, size_t byteDepth)
    {
      if(typeName[0] == 'u') { //unsigned
        if(byteDepth == 1) {
          return CV_8U;
        } else if(byteDepth == 2) {
          return CV_16U;
        }
      } else if(typeName[0] == 'f' && byteDepth == 4) { //float
        return CV_32F;
      } else if(typeName[0] == 'd' && byteDepth == 8) { //double
        return CV_64F;
      } else { // signed
        if(byteDepth == 1) {
          return CV_8S;
        } else if(byteDepth == 2) {
          return CV_16S;
        } else if(byteDepth == 4) {
          return CV_32S;
        }
      }

      return -1;
    }
#endif //#if ANKICORETECH_EMBEDDED_USE_OPENCV

    s32 FindBytePattern(const void * restrict buffer, const s32 bufferLength, const u8 * restrict bytePattern, const s32 bytePatternLength)
    {
      s32 curIndex = 0;
      s32 numBytesInPatternFound = 0;

      // Check if the bytePattern is valid
#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
      for(s32 i=0; i<bytePatternLength; i++) {
        for(s32 j=0; j<bytePatternLength; j++) {
          if(i == j)
            continue;

          if(bytePattern[i] == bytePattern[j]) {
            AnkiError("FindBytePattern", "bytePattern is not unique");
            return -1;
          }
        }
      }
#endif

      const u8 * restrict bufferU8 = reinterpret_cast<const u8*>(buffer);

      while(curIndex < bufferLength) {
        if(numBytesInPatternFound == bytePatternLength) {
          return curIndex - bytePatternLength + 1;
        }

        if(bufferU8[curIndex] == bytePattern[numBytesInPatternFound]) {
          numBytesInPatternFound++;
        } else if(bufferU8[curIndex] == bytePattern[0]) {
          numBytesInPatternFound = 1;
        } else {
          numBytesInPatternFound = 0;
        }

        curIndex++;
      } // while(curIndex < bufferLength)

      return -1;
    }
  } // namespace Embedded
} // namespace Anki
