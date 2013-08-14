
#include "anki/vision.h"

using namespace Anki;

//% img is UQ8.0
//% imgFiltered is UQ8.0
//% Handles edges by replicating the border pixel
//function imgFiltered = binomialFilter(img)
Result BinomialFilter(const Matrix<u8> &img, Matrix<u8> &filteredImg, MemoryStack scratch)
{
  #define KERNEL_SIZE 5
  const u32 kernel[KERNEL_SIZE] = {1, 4, 6, 4, 1};
  const u32 kernelSum = 16;
  const u32 kernelShift = 4;

  assert(kernelSum == (kernel[0] + kernel[1] + kernel[2] + kernel[3] + kernel[4]));
  assert(kernelSum == (1 << kernelShift));

  DASConditionalErrorAndReturn(AreMatricesEqual_SizeAndType<u8>(img, filteredImg), RESULT_FAIL,
                               "BinomialFilter", "size(img) != size(filteredImg) (%dx%d != %dx%d)", img.get_size(0), img.get_size(1), img.get_size(0), img.get_size(1));

  DASConditionalErrorAndReturn(img.get_data() != filteredImg.get_data(), RESULT_FAIL,
                               "BinomialFilter", "img and filteredImg must be different");

  const u32 height = img.get_size(0);
  const u32 width = img.get_size(1);

  const u32 requiredScratch = img.get_size(0) * RoundUp(img.get_size(1)*sizeof(u32), Anki::MEMORY_ALIGNMENT);

  DASConditionalErrorAndReturn(scratch.LargestPossibleAllocation() < requiredScratch, RESULT_FAIL,
                               "BinomialFilter", "Insufficient scratch memory");
  
  //imgFilteredTmp = zeros(size(img), 'uint32');
  Matrix<u32> filteredImgTmp(height, width, scratch);

  //% 1. Horizontally filter
  for(u32 y=0; y<height; y++) {
    const u8 * restrict img_rowPointer = img.Pointer(y, 0);
    u32 * restrict filteredImgTmp_rowPointer = filteredImgTmp.Pointer(y, 0);

    u32 x = 0;

  //    imgFilteredTmp(y,1) = sum(img(y,1:3) .* kernelU32(3:end)) + img(y,1)*sum(kernelU32(1:2));
  //    imgFilteredTmp(y,2) = sum(img(y,1:4) .* kernelU32(2:end)) + img(y,1)*kernelU32(1);
    filteredImgTmp_rowPointer[x++] = img_rowPointer[x]*kernel[2]   + img_rowPointer[x+1]*kernel[3] + img_rowPointer[x+2]*kernel[4] + img_rowPointer[x]*(kernel[0]+kernel[1]);
    filteredImgTmp_rowPointer[x++] = img_rowPointer[x-1]*kernel[1] + img_rowPointer[x]*kernel[2]   + img_rowPointer[x+1]*kernel[3] + img_rowPointer[x+2]*kernel[4] + img_rowPointer[x-1]*kernel[0];

    // for x = 3:(size(img,2)-2)
    for(; x<(width-2); x++) {
      // imgFilteredTmp(y,x) = sum(img(y,(x-2):(x+2)) .* kernelU32);
      filteredImgTmp_rowPointer[0] = img_rowPointer[x-2]*kernel[0] + img_rowPointer[x-1]*kernel[1] + img_rowPointer[x]*kernel[2] + img_rowPointer[x+1]*kernel[3] + img_rowPointer[x+2]*kernel[4];
    }

  //    imgFilteredTmp(y,size(img,2)-1) = sum(img(y,(size(img,2)-3):size(img,2)) .* kernelU32(1:(end-1))) + img(y,size(img,2))*kernelU32(end);
  //    imgFilteredTmp(y,size(img,2))   = sum(img(y,(size(img,2)-2):size(img,2)) .* kernelU32(1:(end-2))) + img(y,size(img,2))*sum(kernelU32((end-1):end));
    filteredImgTmp_rowPointer[x++] = img_rowPointer[x-2]*kernel[0] + img_rowPointer[x-1]*kernel[1] + img_rowPointer[x]*kernel[2] + img_rowPointer[x+1]*kernel[3] + img_rowPointer[x+1]*kernel[4];
    filteredImgTmp_rowPointer[x++] = img_rowPointer[x-2]*kernel[0] + img_rowPointer[x-1]*kernel[1] + img_rowPointer[x]*kernel[2] + img_rowPointer[x]*(kernel[3]+kernel[4]);
  }

  //% 2. Vertically filter
  // for y = {0,1} unrolled
  {
    const u32 * restrict filteredImgTmp_rowPointerY0 = filteredImgTmp.Pointer(0, 0);
    const u32 * restrict filteredImgTmp_rowPointerY1 = filteredImgTmp.Pointer(1, 0);
    const u32 * restrict filteredImgTmp_rowPointerY2 = filteredImgTmp.Pointer(2, 0);
    const u32 * restrict filteredImgTmp_rowPointerY3 = filteredImgTmp.Pointer(3, 0);

    u8 * restrict filteredImg_rowPointerY0 = filteredImg.Pointer(0, 0);
    u8 * restrict filteredImg_rowPointerY1 = filteredImg.Pointer(1, 0);
    for(u32 x=0; x<width; x++) {
      //filtered0 = sum(imgFilteredTmp(1:3,x) .* kernelU32(3:end)') + imgFilteredTmp(1,x)*sum(kernelU32(1:2));
      //imgFiltered(1,x) = uint8( filtered0/(kernelSum^2) );
      const u32 filtered0 = filteredImgTmp_rowPointerY0[x]*kernel[2] + filteredImgTmp_rowPointerY1[x]*kernel[3] + filteredImgTmp_rowPointerY2[x]*kernel[4] +
                            filteredImgTmp_rowPointerY0[x]*(kernel[0]+kernel[1]);
      filteredImg_rowPointerY0[x] = filtered0 >> kernelShift;

      //filtered1 = sum(imgFilteredTmp(1:4,x) .* kernelU32(2:end)') + imgFilteredTmp(1,x)*kernelU32(1);
      //imgFiltered(2,x) = uint8( filtered1/(kernelSum^2) );
      const u32 filtered1 = filteredImgTmp_rowPointerY0[x]*kernel[1] + filteredImgTmp_rowPointerY1[x]*kernel[2] + filteredImgTmp_rowPointerY2[x]*kernel[3] + filteredImgTmp_rowPointerY3[x]*kernel[4] +
                            filteredImgTmp_rowPointerY0[x]*kernel[0];
      filteredImg_rowPointerY1[x] = filtered1 >> kernelShift;
    }
  }

  //for y = 3:(size(img,1)-2)
  for(u32 y=2; y<(height-2); y++) {
    const u32 * restrict filteredImgTmp_rowPointerYm2 = filteredImgTmp.Pointer(y-2, 0);
    const u32 * restrict filteredImgTmp_rowPointerYm1 = filteredImgTmp.Pointer(y-1, 0);
    const u32 * restrict filteredImgTmp_rowPointerY0  = filteredImgTmp.Pointer(y,   0);
    const u32 * restrict filteredImgTmp_rowPointerYp1 = filteredImgTmp.Pointer(y+1, 0);
    const u32 * restrict filteredImgTmp_rowPointerYp2 = filteredImgTmp.Pointer(y+2, 0);

    u8 * restrict filteredImg_rowPointerY = filteredImg.Pointer(y, 0);

    for(u32 x=0; x<width; x++) {
      // imgFiltered(y,x) = uint8( sum(imgFilteredTmp((y-2):(y+2),x) .* kernelU32') / (kernelSum^2) );
      const u32 filtered = filteredImgTmp_rowPointerYm2[x]*kernel[0] + filteredImgTmp_rowPointerYm1[x]*kernel[1] + filteredImgTmp_rowPointerY0[x]*kernel[2] + filteredImgTmp_rowPointerYp1[x]*kernel[3] + filteredImgTmp_rowPointerYp2[x]*kernel[4];
      filteredImg_rowPointerY[x] = filtered >> kernelShift;
    }
  }

  // for y = {height-2,height-1} unrolled
  {
    const u32 * restrict filteredImgTmp_rowPointerYm4 = filteredImgTmp.Pointer(height-4, 0);
    const u32 * restrict filteredImgTmp_rowPointerYm3 = filteredImgTmp.Pointer(height-3, 0);
    const u32 * restrict filteredImgTmp_rowPointerYm2 = filteredImgTmp.Pointer(height-2, 0);
    const u32 * restrict filteredImgTmp_rowPointerYm1 = filteredImgTmp.Pointer(height-1, 0);

    u8 * restrict filteredImg_rowPointerYm2 = filteredImg.Pointer(height-2, 0);
    u8 * restrict filteredImg_rowPointerYm1 = filteredImg.Pointer(height-1, 0);
    for(u32 x=0; x<width; x++) {
      //filtered0 = sum(imgFilteredTmp((size(img,1)-3):size(img,1),x) .* kernelU32(1:(end-1))') + imgFilteredTmp(size(img,1),x)*kernelU32(end);
      //imgFiltered(size(img,1)-1,x) = uint8( filtered0/(kernelSum^2) );
      const u32 filteredm1 = filteredImgTmp_rowPointerYm3[x]*kernel[0] + filteredImgTmp_rowPointerYm2[x]*kernel[1] + filteredImgTmp_rowPointerYm1[x]*kernel[2] +
                             filteredImgTmp_rowPointerYm1[x]*(kernel[3]+kernel[4]);
      filteredImg_rowPointerYm1[x] = filteredm1 >> kernelShift;

      //filtered1 = sum(imgFilteredTmp((size(img,1)-2):size(img,1),x) .* kernelU32(1:(end-2))') + imgFilteredTmp(size(img,1),x)*sum(kernelU32((end-1):end));
      //imgFiltered(size(img,1),x) = uint8( filtered1/(kernelSum^2) );
      const u32 filteredm2 = filteredImgTmp_rowPointerYm4[x]*kernel[0] + filteredImgTmp_rowPointerYm3[x]*kernel[1] + filteredImgTmp_rowPointerYm2[x]*kernel[2] + filteredImgTmp_rowPointerYm1[x]*kernel[3] +
                             filteredImgTmp_rowPointerYm1[x]*kernel[4];
      filteredImg_rowPointerYm2[x] = filteredm2 >> kernelShift;
    }
  }

  return RESULT_OK;
}
