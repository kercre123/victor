/**
File: binomialFilter.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
#define BINOMIAL_KERNEL_SIZE 5

    //% image is UQ8.0
    //% imageFiltered is UQ8.0
    //% Handles edges by replicating the border pixel
    //function imageFiltered = binomialFilter(image)
    //Result BinomialFilter(const Array<u8> &image, Array<u8> &imageFiltered, MemoryStack scratch)
    Result BinomialFilter(const Array<u8> &image, Array<u8> &imageFiltered, MemoryStack scratch)
    {
      u32 kernel[BINOMIAL_KERNEL_SIZE];
      kernel[0] = 1; kernel[1] = 4; kernel[2] = 6; kernel[3] = 4; kernel[4] = 1;
      const s32 kernelShift = 4;

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "BinomialFilter", "image is not valid");

      AnkiConditionalErrorAndReturnValue(imageFiltered.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "BinomialFilter", "imageFiltered is not valid");

      AnkiConditionalWarnAndReturnValue(16 == (kernel[0] + kernel[1] + kernel[2] + kernel[3] + kernel[4]),
        RESULT_FAIL, "BinomialFilter", "Kernel count is wrong");

      AnkiConditionalWarnAndReturnValue(16 == (1 << kernelShift),
        RESULT_FAIL, "BinomialFilter", "Kernel count is wrong");

      AnkiConditionalErrorAndReturnValue(imageHeight == imageFiltered.get_size(0) && imageWidth == imageFiltered.get_size(1),
        RESULT_FAIL_INVALID_SIZE, "BinomialFilter", "size(image) != size(imageFiltered) (%dx%d != %dx%d)", imageHeight, imageWidth, imageHeight, imageWidth);

      AnkiConditionalErrorAndReturnValue(image.get_rawDataPointer() != imageFiltered.get_rawDataPointer(),
        RESULT_FAIL_ALIASED_MEMORY, "BinomialFilter", "image and imageFiltered must be different");

      const s32 requiredScratch = imageHeight * RoundUp<s32>(imageWidth*sizeof(u32), MEMORY_ALIGNMENT);

      AnkiConditionalErrorAndReturnValue(scratch.ComputeLargestPossibleAllocation() >= requiredScratch,
        RESULT_FAIL_OUT_OF_MEMORY, "BinomialFilter", "Insufficient scratch memory");

      //imageFilteredTmp = zeros(size(image), 'uint32');
      Array<u32> imageFilteredTmp(imageHeight, imageWidth, scratch);

      //% 1. Horizontally filter
      for(s32 y=0; y<imageHeight; y++) {
        const u8 * restrict pImage = image.Pointer(y, 0);
        u32 * restrict pImageFilteredTmp = imageFilteredTmp.Pointer(y, 0);

        s32 x = 0;

        //    imageFilteredTmp(y,1) = sum(image(y,1:3) .* kernelU32(3:end)) + image(y,1)*sum(kernelU32(1:2));
        //    imageFilteredTmp(y,2) = sum(image(y,1:4) .* kernelU32(2:end)) + image(y,1)*kernelU32(1);
        pImageFilteredTmp[x++] = static_cast<u32>(pImage[x])*kernel[2]   + static_cast<u32>(pImage[x+1])*kernel[3] + static_cast<u32>(pImage[x+2])*kernel[4] + static_cast<u32>(pImage[x])*(kernel[0]+kernel[1]);
        pImageFilteredTmp[x++] = static_cast<u32>(pImage[x-1])*kernel[1] + static_cast<u32>(pImage[x])*kernel[2]   + static_cast<u32>(pImage[x+1])*kernel[3] + static_cast<u32>(pImage[x+2])*kernel[4] + static_cast<u32>(pImage[x-1])*kernel[0];

        // for x = 3:(size(image,2)-2)
        for(; x<(imageWidth-2); x++) {
          // imageFilteredTmp(y,x) = sum(image(y,(x-2):(x+2)) .* kernelU32);
          pImageFilteredTmp[x] = static_cast<u32>(pImage[x-2])*kernel[0] + static_cast<u32>(pImage[x-1])*kernel[1] + static_cast<u32>(pImage[x])*kernel[2] + static_cast<u32>(pImage[x+1])*kernel[3] + static_cast<u32>(pImage[x+2])*kernel[4];
        }

        //    imageFilteredTmp(y,size(image,2)-1) = sum(image(y,(size(image,2)-3):size(image,2)) .* kernelU32(1:(end-1))) + image(y,size(image,2))*kernelU32(end);
        //    imageFilteredTmp(y,size(image,2))   = sum(image(y,(size(image,2)-2):size(image,2)) .* kernelU32(1:(end-2))) + image(y,size(image,2))*sum(kernelU32((end-1):end));
        pImageFilteredTmp[x++] = static_cast<u32>(pImage[x-2])*kernel[0] + static_cast<u32>(pImage[x-1])*kernel[1] + static_cast<u32>(pImage[x])*kernel[2] + static_cast<u32>(pImage[x+1])*kernel[3] + static_cast<u32>(pImage[x+1])*kernel[4];
        pImageFilteredTmp[x++] = static_cast<u32>(pImage[x-2])*kernel[0] + static_cast<u32>(pImage[x-1])*kernel[1] + static_cast<u32>(pImage[x])*kernel[2] + static_cast<u32>(pImage[x])*(kernel[3]+kernel[4]);
      }

      // std::cout << "imageFilteredTmp:\n";
      // imageFilteredTmp.Print();

      //% 2. Vertically filter
      // for y = {0,1} unrolled
      {
        const u32 * restrict pImageFilteredTmpY0 = imageFilteredTmp.Pointer(0, 0);
        const u32 * restrict pImageFilteredTmpY1 = imageFilteredTmp.Pointer(1, 0);
        const u32 * restrict pImageFilteredTmpY2 = imageFilteredTmp.Pointer(2, 0);
        const u32 * restrict pImageFilteredTmpY3 = imageFilteredTmp.Pointer(3, 0);

        u8 * restrict pImageFiltered_y0 = imageFiltered.Pointer(0, 0);
        u8 * restrict pImageFiltered_y1 = imageFiltered.Pointer(1, 0);
        for(s32 x=0; x<imageWidth; x++) {
          //filtered0 = sum(imageFilteredTmp(1:3,x) .* kernelU32(3:end)') + imageFilteredTmp(1,x)*sum(kernelU32(1:2));
          //imageFiltered(1,x) = uint8( filtered0/(kernelSum^2) );
          const u32 filtered0 = pImageFilteredTmpY0[x]*kernel[2] + pImageFilteredTmpY1[x]*kernel[3] + pImageFilteredTmpY2[x]*kernel[4] +
            pImageFilteredTmpY0[x]*(kernel[0]+kernel[1]);
          pImageFiltered_y0[x] = static_cast<u8>(filtered0 >> (2*kernelShift));

          //filtered1 = sum(imageFilteredTmp(1:4,x) .* kernelU32(2:end)') + imageFilteredTmp(1,x)*kernelU32(1);
          //imageFiltered(2,x) = uint8( filtered1/(kernelSum^2) );
          const u32 filtered1 = pImageFilteredTmpY0[x]*kernel[1] + pImageFilteredTmpY1[x]*kernel[2] + pImageFilteredTmpY2[x]*kernel[3] + pImageFilteredTmpY3[x]*kernel[4] +
            pImageFilteredTmpY0[x]*kernel[0];
          pImageFiltered_y1[x] = static_cast<u8>(filtered1 >> (2*kernelShift));
        }
      }

      //for y = 3:(size(image,1)-2)
      for(s32 y=2; y<(imageHeight-2); y++) {
        const u32 * restrict pImageFilteredTmpYm2 = imageFilteredTmp.Pointer(y-2, 0);
        const u32 * restrict pImageFilteredTmpYm1 = imageFilteredTmp.Pointer(y-1, 0);
        const u32 * restrict pImageFilteredTmpY0  = imageFilteredTmp.Pointer(y,   0);
        const u32 * restrict pImageFilteredTmpYp1 = imageFilteredTmp.Pointer(y+1, 0);
        const u32 * restrict pImageFilteredTmpYp2 = imageFilteredTmp.Pointer(y+2, 0);

        u8 * restrict pImageFiltered = imageFiltered.Pointer(y, 0);

        for(s32 x=0; x<imageWidth; x++) {
          // imageFiltered(y,x) = uint8( sum(imageFilteredTmp((y-2):(y+2),x) .* kernelU32') / (kernelSum^2) );
          const u32 filtered = pImageFilteredTmpYm2[x]*kernel[0] + pImageFilteredTmpYm1[x]*kernel[1] + pImageFilteredTmpY0[x]*kernel[2] + pImageFilteredTmpYp1[x]*kernel[3] + pImageFilteredTmpYp2[x]*kernel[4];
          pImageFiltered[x] = static_cast<u8>(filtered >> (2*kernelShift));
        }
      }

      // for y = {imageHeight-2,imageHeight-1} unrolled
      {
        const u32 * restrict pImageFilteredTmpYm4 = imageFilteredTmp.Pointer(imageHeight-4, 0);
        const u32 * restrict pImageFilteredTmpYm3 = imageFilteredTmp.Pointer(imageHeight-3, 0);
        const u32 * restrict pImageFilteredTmpYm2 = imageFilteredTmp.Pointer(imageHeight-2, 0);
        const u32 * restrict pImageFilteredTmpYm1 = imageFilteredTmp.Pointer(imageHeight-1, 0);

        u8 * restrict pImageFiltered_ym2 = imageFiltered.Pointer(imageHeight-2, 0);
        u8 * restrict pImageFiltered_ym1 = imageFiltered.Pointer(imageHeight-1, 0);
        for(s32 x=0; x<imageWidth; x++) {
          //filtered0 = sum(imageFilteredTmp((size(image,1)-3):size(image,1),x) .* kernelU32(1:(end-1))') + imageFilteredTmp(size(image,1),x)*kernelU32(end);
          //imageFiltered(size(image,1)-1,x) = uint8( filtered0/(kernelSum^2) );
          const u32 filteredm1 = pImageFilteredTmpYm3[x]*kernel[0] + pImageFilteredTmpYm2[x]*kernel[1] + pImageFilteredTmpYm1[x]*kernel[2] +
            pImageFilteredTmpYm1[x]*(kernel[3]+kernel[4]);
          pImageFiltered_ym1[x] = static_cast<u8>(filteredm1 >> (2*kernelShift));

          //filtered1 = sum(imageFilteredTmp((size(image,1)-2):size(image,1),x) .* kernelU32(1:(end-2))') + imageFilteredTmp(size(image,1),x)*sum(kernelU32((end-1):end));
          //imageFiltered(size(image,1),x) = uint8( filtered1/(kernelSum^2) );
          const u32 filteredm2 = pImageFilteredTmpYm4[x]*kernel[0] + pImageFilteredTmpYm3[x]*kernel[1] + pImageFilteredTmpYm2[x]*kernel[2] + pImageFilteredTmpYm1[x]*kernel[3] +
            pImageFilteredTmpYm1[x]*kernel[4];
          pImageFiltered_ym2[x] = static_cast<u8>(filteredm2 >> (2*kernelShift));
        }
      }

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki