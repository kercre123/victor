/**
File:
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
    void PrintfOneArray_f32(const Array<f32> &array, const char * variableName)
    {
      const s32 arrayHeight = array.get_size(0);
      const s32 arrayWidth = array.get_size(1);

      printf(variableName);
      printf(":\n");
      for(s32 y=0; y<arrayHeight; y++) {
        const f32 * const pThisData = array.Pointer(y, 0);

        // This goes every two, because the output on movidius looks more reasonable
        for(s32 x=0; x<arrayWidth-1; x+=2) {
          const f32 value1 = pThisData[x];
          const f32 value2 = pThisData[x+1];
          const s32 mulitipliedValue1 = static_cast<s32>(Round(10000.0f * value1));
          const s32 mulitipliedValue2 = static_cast<s32>(Round(10000.0f * value2));

          printf(PrintfOneArray_FORMAT_STRING_2, mulitipliedValue1, mulitipliedValue2);
        }

        if(IsOdd(arrayWidth)) {
          for(s32 x=arrayWidth-1; x<arrayWidth; x++) {
            const f32 value1 = pThisData[x];
            const s32 mulitipliedValue1 = static_cast<s32>(Round(10000.0f * value1));

            printf(PrintfOneArray_FORMAT_STRING_1, static_cast<s32>(mulitipliedValue1));
          }
        }
        printf("\n");
      }
      printf("\n");
    }

    void PrintfOneArray_f64(const Array<f64> &array, const char * variableName)
    {
      const s32 arrayHeight = array.get_size(0);
      const s32 arrayWidth = array.get_size(1);

      printf(variableName);
      printf(":\n");
      for(s32 y=0; y<arrayHeight; y++) {
        const f64 * const pThisData = array.Pointer(y, 0);

        // This goes every two, because the output on movidius looks more reasonable
        for(s32 x=0; x<arrayWidth-1; x+=2) {
          const f64 value1 = pThisData[x];
          const f64 value2 = pThisData[x+1];
          const s32 mulitipliedValue1 = static_cast<s32>(Round(10000.0 * value1));
          const s32 mulitipliedValue2 = static_cast<s32>(Round(10000.0 * value2));

          printf(PrintfOneArray_FORMAT_STRING_2, mulitipliedValue1, mulitipliedValue2);
        }

        if(IsOdd(arrayWidth)) {
          for(s32 x=arrayWidth-1; x<arrayWidth; x++) {
            const f64 value1 = pThisData[x];
            const s32 mulitipliedValue1 = static_cast<s32>(Round(10000.0 * value1));

            printf(PrintfOneArray_FORMAT_STRING_1, mulitipliedValue1);
          }
        }
        printf("\n");
      }
      printf("\n");
    }

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
  } // namespace Embedded
} // namespace Anki