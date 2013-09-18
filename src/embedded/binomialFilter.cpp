#include "anki/embeddedVision.h"

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
    IN_DDR Result BinomialFilter(const Array<u8> &image, Array<u8> &imageFiltered, MemoryStack scratch)
    {
      const u32 kernel[BINOMIAL_KERNEL_SIZE] = {1, 4, 6, 4, 1};
      const s32 kernelShift = 4;

#if ANKI_DEBUG_LEVEL > ANKI_DEBUG_ESSENTIAL_AND_ERROR
      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL, "BinomialFilter", "image is not valid");

      AnkiConditionalErrorAndReturnValue(imageFiltered.IsValid(),
        RESULT_FAIL, "BinomialFilter", "imageFiltered is not valid");

      assert(16 == (kernel[0] + kernel[1] + kernel[2] + kernel[3] + kernel[4]));
      assert(16 == (1 << kernelShift));

      // TODO: implement
      /*AnkiConditionalErrorAndReturnValue(AreMatricesEqual_SizeAndType<u8>(image, imageFiltered),
      RESULT_FAIL, "BinomialFilter", "size(image) != size(imageFiltered) (%dx%d != %dx%d)", image.get_size(0), image.get_size(1), image.get_size(0), image.get_size(1));

      AnkiConditionalErrorAndReturnValue(image.get_rawDataPointer() != imageFiltered.get_rawDataPointer(),
      RESULT_FAIL, "BinomialFilter", "image and imageFiltered must be different");*/
#endif // #if ANKI_DEBUG_LEVEL > ANKI_DEBUG_ESSENTIAL_AND_ERROR

      const s32 height = image.get_size(0);
      const s32 width = image.get_size(1);

      const s32 requiredScratch = image.get_size(0) * RoundUp<s32>(image.get_size(1)*sizeof(u32), MEMORY_ALIGNMENT);

#if ANKI_DEBUG_LEVEL > ANKI_DEBUG_ESSENTIAL_AND_ERROR
      AnkiConditionalErrorAndReturnValue(scratch.ComputeLargestPossibleAllocation() >= requiredScratch,
        RESULT_FAIL, "BinomialFilter", "Insufficient scratch memory");
#endif // #if ANKI_DEBUG_LEVEL > ANKI_DEBUG_ESSENTIAL_AND_ERROR

      //imageFilteredTmp = zeros(size(image), 'uint32');
      Array<u32> imageFilteredTmp(height, width, scratch);

      //% 1. Horizontally filter
      for(s32 y=0; y<height; y++) {
        const u8 * restrict image_rowPointer = image.Pointer(y, 0);
        u32 * restrict imageFilteredTmp_rowPointer = imageFilteredTmp.Pointer(y, 0);

        s32 x = 0;

        //    imageFilteredTmp(y,1) = sum(image(y,1:3) .* kernelU32(3:end)) + image(y,1)*sum(kernelU32(1:2));
        //    imageFilteredTmp(y,2) = sum(image(y,1:4) .* kernelU32(2:end)) + image(y,1)*kernelU32(1);
        imageFilteredTmp_rowPointer[x++] = static_cast<u32>(image_rowPointer[x])*kernel[2]   + static_cast<u32>(image_rowPointer[x+1])*kernel[3] + static_cast<u32>(image_rowPointer[x+2])*kernel[4] + static_cast<u32>(image_rowPointer[x])*(kernel[0]+kernel[1]);
        imageFilteredTmp_rowPointer[x++] = static_cast<u32>(image_rowPointer[x-1])*kernel[1] + static_cast<u32>(image_rowPointer[x])*kernel[2]   + static_cast<u32>(image_rowPointer[x+1])*kernel[3] + static_cast<u32>(image_rowPointer[x+2])*kernel[4] + static_cast<u32>(image_rowPointer[x-1])*kernel[0];

        // for x = 3:(size(image,2)-2)
        for(; x<(width-2); x++) {
          // imageFilteredTmp(y,x) = sum(image(y,(x-2):(x+2)) .* kernelU32);
          imageFilteredTmp_rowPointer[x] = static_cast<u32>(image_rowPointer[x-2])*kernel[0] + static_cast<u32>(image_rowPointer[x-1])*kernel[1] + static_cast<u32>(image_rowPointer[x])*kernel[2] + static_cast<u32>(image_rowPointer[x+1])*kernel[3] + static_cast<u32>(image_rowPointer[x+2])*kernel[4];
        }

        //    imageFilteredTmp(y,size(image,2)-1) = sum(image(y,(size(image,2)-3):size(image,2)) .* kernelU32(1:(end-1))) + image(y,size(image,2))*kernelU32(end);
        //    imageFilteredTmp(y,size(image,2))   = sum(image(y,(size(image,2)-2):size(image,2)) .* kernelU32(1:(end-2))) + image(y,size(image,2))*sum(kernelU32((end-1):end));
        imageFilteredTmp_rowPointer[x++] = static_cast<u32>(image_rowPointer[x-2])*kernel[0] + static_cast<u32>(image_rowPointer[x-1])*kernel[1] + static_cast<u32>(image_rowPointer[x])*kernel[2] + static_cast<u32>(image_rowPointer[x+1])*kernel[3] + static_cast<u32>(image_rowPointer[x+1])*kernel[4];
        imageFilteredTmp_rowPointer[x++] = static_cast<u32>(image_rowPointer[x-2])*kernel[0] + static_cast<u32>(image_rowPointer[x-1])*kernel[1] + static_cast<u32>(image_rowPointer[x])*kernel[2] + static_cast<u32>(image_rowPointer[x])*(kernel[3]+kernel[4]);
      }

      // std::cout << "imageFilteredTmp:\n";
      // imageFilteredTmp.Print();

      //% 2. Vertically filter
      // for y = {0,1} unrolled
      {
        const u32 * restrict imageFilteredTmp_rowPointerY0 = imageFilteredTmp.Pointer(0, 0);
        const u32 * restrict imageFilteredTmp_rowPointerY1 = imageFilteredTmp.Pointer(1, 0);
        const u32 * restrict imageFilteredTmp_rowPointerY2 = imageFilteredTmp.Pointer(2, 0);
        const u32 * restrict imageFilteredTmp_rowPointerY3 = imageFilteredTmp.Pointer(3, 0);

        u8 * restrict imageFiltered_rowPointerY0 = imageFiltered.Pointer(0, 0);
        u8 * restrict imageFiltered_rowPointerY1 = imageFiltered.Pointer(1, 0);
        for(s32 x=0; x<width; x++) {
          //filtered0 = sum(imageFilteredTmp(1:3,x) .* kernelU32(3:end)') + imageFilteredTmp(1,x)*sum(kernelU32(1:2));
          //imageFiltered(1,x) = uint8( filtered0/(kernelSum^2) );
          const u32 filtered0 = imageFilteredTmp_rowPointerY0[x]*kernel[2] + imageFilteredTmp_rowPointerY1[x]*kernel[3] + imageFilteredTmp_rowPointerY2[x]*kernel[4] +
            imageFilteredTmp_rowPointerY0[x]*(kernel[0]+kernel[1]);
          imageFiltered_rowPointerY0[x] = static_cast<u8>(filtered0 >> (2*kernelShift));

          //filtered1 = sum(imageFilteredTmp(1:4,x) .* kernelU32(2:end)') + imageFilteredTmp(1,x)*kernelU32(1);
          //imageFiltered(2,x) = uint8( filtered1/(kernelSum^2) );
          const u32 filtered1 = imageFilteredTmp_rowPointerY0[x]*kernel[1] + imageFilteredTmp_rowPointerY1[x]*kernel[2] + imageFilteredTmp_rowPointerY2[x]*kernel[3] + imageFilteredTmp_rowPointerY3[x]*kernel[4] +
            imageFilteredTmp_rowPointerY0[x]*kernel[0];
          imageFiltered_rowPointerY1[x] = static_cast<u8>(filtered1 >> (2*kernelShift));
        }
      }

      //for y = 3:(size(image,1)-2)
      for(s32 y=2; y<(height-2); y++) {
        const u32 * restrict imageFilteredTmp_rowPointerYm2 = imageFilteredTmp.Pointer(y-2, 0);
        const u32 * restrict imageFilteredTmp_rowPointerYm1 = imageFilteredTmp.Pointer(y-1, 0);
        const u32 * restrict imageFilteredTmp_rowPointerY0  = imageFilteredTmp.Pointer(y,   0);
        const u32 * restrict imageFilteredTmp_rowPointerYp1 = imageFilteredTmp.Pointer(y+1, 0);
        const u32 * restrict imageFilteredTmp_rowPointerYp2 = imageFilteredTmp.Pointer(y+2, 0);

        u8 * restrict imageFiltered_rowPointerY = imageFiltered.Pointer(y, 0);

        for(s32 x=0; x<width; x++) {
          // imageFiltered(y,x) = uint8( sum(imageFilteredTmp((y-2):(y+2),x) .* kernelU32') / (kernelSum^2) );
          const u32 filtered = imageFilteredTmp_rowPointerYm2[x]*kernel[0] + imageFilteredTmp_rowPointerYm1[x]*kernel[1] + imageFilteredTmp_rowPointerY0[x]*kernel[2] + imageFilteredTmp_rowPointerYp1[x]*kernel[3] + imageFilteredTmp_rowPointerYp2[x]*kernel[4];
          imageFiltered_rowPointerY[x] = static_cast<u8>(filtered >> (2*kernelShift));
        }
      }

      // for y = {height-2,height-1} unrolled
      {
        const u32 * restrict imageFilteredTmp_rowPointerYm4 = imageFilteredTmp.Pointer(height-4, 0);
        const u32 * restrict imageFilteredTmp_rowPointerYm3 = imageFilteredTmp.Pointer(height-3, 0);
        const u32 * restrict imageFilteredTmp_rowPointerYm2 = imageFilteredTmp.Pointer(height-2, 0);
        const u32 * restrict imageFilteredTmp_rowPointerYm1 = imageFilteredTmp.Pointer(height-1, 0);

        u8 * restrict imageFiltered_rowPointerYm2 = imageFiltered.Pointer(height-2, 0);
        u8 * restrict imageFiltered_rowPointerYm1 = imageFiltered.Pointer(height-1, 0);
        for(s32 x=0; x<width; x++) {
          //filtered0 = sum(imageFilteredTmp((size(image,1)-3):size(image,1),x) .* kernelU32(1:(end-1))') + imageFilteredTmp(size(image,1),x)*kernelU32(end);
          //imageFiltered(size(image,1)-1,x) = uint8( filtered0/(kernelSum^2) );
          const u32 filteredm1 = imageFilteredTmp_rowPointerYm3[x]*kernel[0] + imageFilteredTmp_rowPointerYm2[x]*kernel[1] + imageFilteredTmp_rowPointerYm1[x]*kernel[2] +
            imageFilteredTmp_rowPointerYm1[x]*(kernel[3]+kernel[4]);
          imageFiltered_rowPointerYm1[x] = static_cast<u8>(filteredm1 >> (2*kernelShift));

          //filtered1 = sum(imageFilteredTmp((size(image,1)-2):size(image,1),x) .* kernelU32(1:(end-2))') + imageFilteredTmp(size(image,1),x)*sum(kernelU32((end-1):end));
          //imageFiltered(size(image,1),x) = uint8( filtered1/(kernelSum^2) );
          const u32 filteredm2 = imageFilteredTmp_rowPointerYm4[x]*kernel[0] + imageFilteredTmp_rowPointerYm3[x]*kernel[1] + imageFilteredTmp_rowPointerYm2[x]*kernel[2] + imageFilteredTmp_rowPointerYm1[x]*kernel[3] +
            imageFilteredTmp_rowPointerYm1[x]*kernel[4];
          imageFiltered_rowPointerYm2[x] = static_cast<u8>(filteredm2 >> (2*kernelShift));
        }
      }

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki