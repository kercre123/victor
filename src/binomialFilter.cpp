#include "anki/vision.h"

namespace Anki
{
#define BINOMIAL_KERNEL_SIZE 5

  //% img is UQ8.0
  //% imgFiltered is UQ8.0
  //% Handles edges by replicating the border pixel
  //function imgFiltered = binomialFilter(img)
  //Result BinomialFilter(const Matrix<u8> &img, Matrix<u8> &imgFiltered, MemoryStack scratch)
  Result BinomialFilter(const Matrix<u8> &img, Matrix<u8> &imgFiltered, MemoryStack scratch)
  {
    const u32 kernel[BINOMIAL_KERNEL_SIZE] = {1, 4, 6, 4, 1};
    const u32 kernelSum = 16;
    const u32 kernelShift = 4;

    assert(kernelSum == (kernel[0] + kernel[1] + kernel[2] + kernel[3] + kernel[4]));
    assert(kernelSum == (1 << kernelShift));

    DASConditionalErrorAndReturnValue(AreMatricesEqual_SizeAndType<u8>(img, imgFiltered),
      RESULT_FAIL, "BinomialFilter", "size(img) != size(imgFiltered) (%dx%d != %dx%d)", img.get_size(0), img.get_size(1), img.get_size(0), img.get_size(1));

    DASConditionalErrorAndReturnValue(img.get_rawDataPointer() != imgFiltered.get_rawDataPointer(),
      RESULT_FAIL, "BinomialFilter", "img and imgFiltered must be different");

    const u32 height = img.get_size(0);
    const u32 width = img.get_size(1);

    const u32 requiredScratch = img.get_size(0) * RoundUp(img.get_size(1)*sizeof(u32), MEMORY_ALIGNMENT);

    DASConditionalErrorAndReturnValue(scratch.LargestPossibleAllocation() >= requiredScratch,
      RESULT_FAIL, "BinomialFilter", "Insufficient scratch memory");

    //imgFilteredTmp = zeros(size(img), 'uint32');
    Matrix<u32> imgFilteredTmp(height, width, scratch);

    //% 1. Horizontally filter
    for(u32 y=0; y<height; y++) {
      const u8 * restrict img_rowPointer = img.Pointer(y, 0);
      u32 * restrict imgFilteredTmp_rowPointer = imgFilteredTmp.Pointer(y, 0);

      u32 x = 0;

      //    imgFilteredTmp(y,1) = sum(img(y,1:3) .* kernelU32(3:end)) + img(y,1)*sum(kernelU32(1:2));
      //    imgFilteredTmp(y,2) = sum(img(y,1:4) .* kernelU32(2:end)) + img(y,1)*kernelU32(1);
      imgFilteredTmp_rowPointer[x++] = static_cast<u32>(img_rowPointer[x])*kernel[2]   + static_cast<u32>(img_rowPointer[x+1])*kernel[3] + static_cast<u32>(img_rowPointer[x+2])*kernel[4] + static_cast<u32>(img_rowPointer[x])*(kernel[0]+kernel[1]);
      imgFilteredTmp_rowPointer[x++] = static_cast<u32>(img_rowPointer[x-1])*kernel[1] + static_cast<u32>(img_rowPointer[x])*kernel[2]   + static_cast<u32>(img_rowPointer[x+1])*kernel[3] + static_cast<u32>(img_rowPointer[x+2])*kernel[4] + static_cast<u32>(img_rowPointer[x-1])*kernel[0];

      // for x = 3:(size(img,2)-2)
      for(; x<(width-2); x++) {
        // imgFilteredTmp(y,x) = sum(img(y,(x-2):(x+2)) .* kernelU32);
        imgFilteredTmp_rowPointer[x] = static_cast<u32>(img_rowPointer[x-2])*kernel[0] + static_cast<u32>(img_rowPointer[x-1])*kernel[1] + static_cast<u32>(img_rowPointer[x])*kernel[2] + static_cast<u32>(img_rowPointer[x+1])*kernel[3] + static_cast<u32>(img_rowPointer[x+2])*kernel[4];
      }

      //    imgFilteredTmp(y,size(img,2)-1) = sum(img(y,(size(img,2)-3):size(img,2)) .* kernelU32(1:(end-1))) + img(y,size(img,2))*kernelU32(end);
      //    imgFilteredTmp(y,size(img,2))   = sum(img(y,(size(img,2)-2):size(img,2)) .* kernelU32(1:(end-2))) + img(y,size(img,2))*sum(kernelU32((end-1):end));
      imgFilteredTmp_rowPointer[x++] = static_cast<u32>(img_rowPointer[x-2])*kernel[0] + static_cast<u32>(img_rowPointer[x-1])*kernel[1] + static_cast<u32>(img_rowPointer[x])*kernel[2] + static_cast<u32>(img_rowPointer[x+1])*kernel[3] + static_cast<u32>(img_rowPointer[x+1])*kernel[4];
      imgFilteredTmp_rowPointer[x++] = static_cast<u32>(img_rowPointer[x-2])*kernel[0] + static_cast<u32>(img_rowPointer[x-1])*kernel[1] + static_cast<u32>(img_rowPointer[x])*kernel[2] + static_cast<u32>(img_rowPointer[x])*(kernel[3]+kernel[4]);
    }

    // std::cout << "imgFilteredTmp:\n";
    // imgFilteredTmp.Print();

    //% 2. Vertically filter
    // for y = {0,1} unrolled
    {
      const u32 * restrict imgFilteredTmp_rowPointerY0 = imgFilteredTmp.Pointer(0, 0);
      const u32 * restrict imgFilteredTmp_rowPointerY1 = imgFilteredTmp.Pointer(1, 0);
      const u32 * restrict imgFilteredTmp_rowPointerY2 = imgFilteredTmp.Pointer(2, 0);
      const u32 * restrict imgFilteredTmp_rowPointerY3 = imgFilteredTmp.Pointer(3, 0);

      u8 * restrict imgFiltered_rowPointerY0 = imgFiltered.Pointer(0, 0);
      u8 * restrict imgFiltered_rowPointerY1 = imgFiltered.Pointer(1, 0);
      for(u32 x=0; x<width; x++) {
        //filtered0 = sum(imgFilteredTmp(1:3,x) .* kernelU32(3:end)') + imgFilteredTmp(1,x)*sum(kernelU32(1:2));
        //imgFiltered(1,x) = uint8( filtered0/(kernelSum^2) );
        const u32 filtered0 = imgFilteredTmp_rowPointerY0[x]*kernel[2] + imgFilteredTmp_rowPointerY1[x]*kernel[3] + imgFilteredTmp_rowPointerY2[x]*kernel[4] +
          imgFilteredTmp_rowPointerY0[x]*(kernel[0]+kernel[1]);
        imgFiltered_rowPointerY0[x] = filtered0 >> (2*kernelShift);

        //filtered1 = sum(imgFilteredTmp(1:4,x) .* kernelU32(2:end)') + imgFilteredTmp(1,x)*kernelU32(1);
        //imgFiltered(2,x) = uint8( filtered1/(kernelSum^2) );
        const u32 filtered1 = imgFilteredTmp_rowPointerY0[x]*kernel[1] + imgFilteredTmp_rowPointerY1[x]*kernel[2] + imgFilteredTmp_rowPointerY2[x]*kernel[3] + imgFilteredTmp_rowPointerY3[x]*kernel[4] +
          imgFilteredTmp_rowPointerY0[x]*kernel[0];
        imgFiltered_rowPointerY1[x] = filtered1 >> (2*kernelShift);
      }
    }

    //for y = 3:(size(img,1)-2)
    for(u32 y=2; y<(height-2); y++) {
      const u32 * restrict imgFilteredTmp_rowPointerYm2 = imgFilteredTmp.Pointer(y-2, 0);
      const u32 * restrict imgFilteredTmp_rowPointerYm1 = imgFilteredTmp.Pointer(y-1, 0);
      const u32 * restrict imgFilteredTmp_rowPointerY0  = imgFilteredTmp.Pointer(y,   0);
      const u32 * restrict imgFilteredTmp_rowPointerYp1 = imgFilteredTmp.Pointer(y+1, 0);
      const u32 * restrict imgFilteredTmp_rowPointerYp2 = imgFilteredTmp.Pointer(y+2, 0);

      u8 * restrict imgFiltered_rowPointerY = imgFiltered.Pointer(y, 0);

      for(u32 x=0; x<width; x++) {
        // imgFiltered(y,x) = uint8( sum(imgFilteredTmp((y-2):(y+2),x) .* kernelU32') / (kernelSum^2) );
        const u32 filtered = imgFilteredTmp_rowPointerYm2[x]*kernel[0] + imgFilteredTmp_rowPointerYm1[x]*kernel[1] + imgFilteredTmp_rowPointerY0[x]*kernel[2] + imgFilteredTmp_rowPointerYp1[x]*kernel[3] + imgFilteredTmp_rowPointerYp2[x]*kernel[4];
        imgFiltered_rowPointerY[x] = filtered >> (2*kernelShift);
      }
    }

    // for y = {height-2,height-1} unrolled
    {
      const u32 * restrict imgFilteredTmp_rowPointerYm4 = imgFilteredTmp.Pointer(height-4, 0);
      const u32 * restrict imgFilteredTmp_rowPointerYm3 = imgFilteredTmp.Pointer(height-3, 0);
      const u32 * restrict imgFilteredTmp_rowPointerYm2 = imgFilteredTmp.Pointer(height-2, 0);
      const u32 * restrict imgFilteredTmp_rowPointerYm1 = imgFilteredTmp.Pointer(height-1, 0);

      u8 * restrict imgFiltered_rowPointerYm2 = imgFiltered.Pointer(height-2, 0);
      u8 * restrict imgFiltered_rowPointerYm1 = imgFiltered.Pointer(height-1, 0);
      for(u32 x=0; x<width; x++) {
        //filtered0 = sum(imgFilteredTmp((size(img,1)-3):size(img,1),x) .* kernelU32(1:(end-1))') + imgFilteredTmp(size(img,1),x)*kernelU32(end);
        //imgFiltered(size(img,1)-1,x) = uint8( filtered0/(kernelSum^2) );
        const u32 filteredm1 = imgFilteredTmp_rowPointerYm3[x]*kernel[0] + imgFilteredTmp_rowPointerYm2[x]*kernel[1] + imgFilteredTmp_rowPointerYm1[x]*kernel[2] +
          imgFilteredTmp_rowPointerYm1[x]*(kernel[3]+kernel[4]);
        imgFiltered_rowPointerYm1[x] = filteredm1 >> (2*kernelShift);

        //filtered1 = sum(imgFilteredTmp((size(img,1)-2):size(img,1),x) .* kernelU32(1:(end-2))') + imgFilteredTmp(size(img,1),x)*sum(kernelU32((end-1):end));
        //imgFiltered(size(img,1),x) = uint8( filtered1/(kernelSum^2) );
        const u32 filteredm2 = imgFilteredTmp_rowPointerYm4[x]*kernel[0] + imgFilteredTmp_rowPointerYm3[x]*kernel[1] + imgFilteredTmp_rowPointerYm2[x]*kernel[2] + imgFilteredTmp_rowPointerYm1[x]*kernel[3] +
          imgFilteredTmp_rowPointerYm1[x]*kernel[4];
        imgFiltered_rowPointerYm2[x] = filteredm2 >> (2*kernelShift);
      }
    }

    return RESULT_OK;
  }
} // namespace Anki