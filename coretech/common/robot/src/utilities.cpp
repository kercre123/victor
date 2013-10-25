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
    IN_DDR void PrintfOneArray_f32(const Array<f32> &array, const char * variableName)
    {
      const s32 arrayHeight = array.get_size(0);
      const s32 arrayWidth = array.get_size(1);

      printf(variableName);
      printf(":\n");
      for(s32 y=0; y<arrayHeight; y++) {
        const f32 * const rowPointer = array.Pointer(y, 0);

        // This goes every two, because the output on movidius looks more reasonable
        for(s32 x=0; x<arrayWidth-1; x+=2) {
          const f32 value1 = rowPointer[x];
          const f32 value2 = rowPointer[x+1];
          const s32 mulitipliedValue1 = static_cast<s32>(Round(10000.0f * value1));
          const s32 mulitipliedValue2 = static_cast<s32>(Round(10000.0f * value2));

          printf(PrintfOneArray_FORMAT_STRING_2, mulitipliedValue1, mulitipliedValue2);
        }

        if(IsOdd(arrayWidth)) {
          for(s32 x=arrayWidth-1; x<arrayWidth; x++) {
            const f32 value1 = rowPointer[x];
            const s32 mulitipliedValue1 = static_cast<s32>(Round(10000.0f * value1));

            printf(PrintfOneArray_FORMAT_STRING_1, static_cast<s32>(mulitipliedValue1));
          }
        }
        printf("\n");
      }
      printf("\n");
    }

    IN_DDR void PrintfOneArray_f64(const Array<f64> &array, const char * variableName)
    {
      const s32 arrayHeight = array.get_size(0);
      const s32 arrayWidth = array.get_size(1);

      printf(variableName);
      printf(":\n");
      for(s32 y=0; y<arrayHeight; y++) {
        const f64 * const rowPointer = array.Pointer(y, 0);

        // This goes every two, because the output on movidius looks more reasonable
        for(s32 x=0; x<arrayWidth-1; x+=2) {
          const f64 value1 = rowPointer[x];
          const f64 value2 = rowPointer[x+1];
          const s32 mulitipliedValue1 = static_cast<s32>(Round(10000.0 * value1));
          const s32 mulitipliedValue2 = static_cast<s32>(Round(10000.0 * value2));

          printf(PrintfOneArray_FORMAT_STRING_2, mulitipliedValue1, mulitipliedValue2);
        }

        if(IsOdd(arrayWidth)) {
          for(s32 x=arrayWidth-1; x<arrayWidth; x++) {
            const f64 value1 = rowPointer[x];
            const s32 mulitipliedValue1 = static_cast<s32>(Round(10000.0 * value1));

            printf(PrintfOneArray_FORMAT_STRING_1, mulitipliedValue1);
          }
        }
        printf("\n");
      }
      printf("\n");
    }

    IN_DDR s32 Sum(const Array<u8> &image)
    {
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      s32 sum = 0;
      for(s32 y=0; y<imageHeight; y++) {
        const u8 * const rowPointer = image.Pointer(y, 0);
        for(s32 x=0; x<imageWidth; x++) {
          sum += rowPointer[x];
        }
      }

      return sum;
    }

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    IN_DDR int ConvertToOpenCvType(const char *typeName, size_t byteDepth)
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