

//#define IMAGE_POINTER(img, y, x, stride) ((void*) ((char*)(img+(x)) + (y)*(stride) ))
#define IMAGE_POINTER(img, y, x, stride) ((void*) ((char*)(img+(x))))

#define BINOMIAL_KERNEL_SIZE 5

#include "allFunctions.h"

//% img is UQ8.0
//% imgFiltered is UQ8.0
//% Handles edges by replicating the border pixel
//function imgFiltered = binomialFilter(img)
//Result BinomialFilter(const Array_u8 &img, Array_u8 &imgFiltered, MemoryStack scratch)
s32 BinomialFilter(const u8 * restrict img, u8 * restrict imgFiltered, u32 * restrict imgFilteredTmp, const s32 height, const s32 width)
{
  const u32 kernel[BINOMIAL_KERNEL_SIZE] = {1, 4, 6, 4, 1};
  const s32 kernelShift = 4;
	s32 y;

  //% 1. Horizontally filter
  for(y=0; y<height; y++) {
    //const u8 * restrict img_rowPointer = img.Pointer(y, 0);
    //u32 * restrict imgFilteredTmp_rowPointer = imgFilteredTmp.Pointer(y, 0);
    const u8 * restrict img_rowPointer = IMAGE_POINTER(img, y, 0, width);
    u32 * restrict imgFilteredTmp_rowPointer = IMAGE_POINTER(imgFilteredTmp, y, 0, width*sizeof(u32));

    s32 x = 0;

    imgFilteredTmp_rowPointer[x++] = (u32)(img_rowPointer[x])*kernel[2]   + (u32)(img_rowPointer[x+1])*kernel[3] + (u32)(img_rowPointer[x+2])*kernel[4] + (u32)(img_rowPointer[x])*(kernel[0]+kernel[1]);
    imgFilteredTmp_rowPointer[x++] = (u32)(img_rowPointer[x-1])*kernel[1] + (u32)(img_rowPointer[x])*kernel[2]   + (u32)(img_rowPointer[x+1])*kernel[3] + (u32)(img_rowPointer[x+2])*kernel[4] + (u32)(img_rowPointer[x-1])*kernel[0];

    for(; x<(width-2); x++) {
      imgFilteredTmp_rowPointer[x] = (u32)(img_rowPointer[x-2])*kernel[0] + (u32)(img_rowPointer[x-1])*kernel[1] + (u32)(img_rowPointer[x])*kernel[2] + (u32)(img_rowPointer[x+1])*kernel[3] + (u32)(img_rowPointer[x+2])*kernel[4];
    }

    imgFilteredTmp_rowPointer[x++] = (u32)(img_rowPointer[x-2])*kernel[0] + (u32)(img_rowPointer[x-1])*kernel[1] + (u32)(img_rowPointer[x])*kernel[2] + (u32)(img_rowPointer[x+1])*kernel[3] + (u32)(img_rowPointer[x+1])*kernel[4];
    imgFilteredTmp_rowPointer[x++] = (u32)(img_rowPointer[x-2])*kernel[0] + (u32)(img_rowPointer[x-1])*kernel[1] + (u32)(img_rowPointer[x])*kernel[2] + (u32)(img_rowPointer[x])*(kernel[3]+kernel[4]);
  }

  //% 2. Vertically filter
  {
		s32 x;
    //const u32 * restrict imgFilteredTmp_rowPointerY0 = imgFilteredTmp.Pointer(0, 0);
    //const u32 * restrict imgFilteredTmp_rowPointerY1 = imgFilteredTmp.Pointer(1, 0);
    //const u32 * restrict imgFilteredTmp_rowPointerY2 = imgFilteredTmp.Pointer(2, 0);
    //const u32 * restrict imgFilteredTmp_rowPointerY3 = imgFilteredTmp.Pointer(3, 0);
    const u32 * restrict imgFilteredTmp_rowPointerY0 = IMAGE_POINTER(imgFilteredTmp, 0, 0, width*sizeof(u32));
    const u32 * restrict imgFilteredTmp_rowPointerY1 = IMAGE_POINTER(imgFilteredTmp, 1, 0, width*sizeof(u32));
    const u32 * restrict imgFilteredTmp_rowPointerY2 = IMAGE_POINTER(imgFilteredTmp, 2, 0, width*sizeof(u32));
    const u32 * restrict imgFilteredTmp_rowPointerY3 = IMAGE_POINTER(imgFilteredTmp, 3, 0, width*sizeof(u32));

    //u8 * restrict imgFiltered_rowPointerY0 = imgFiltered.Pointer(0, 0);
    //u8 * restrict imgFiltered_rowPointerY1 = imgFiltered.Pointer(1, 0);
    u8 * restrict imgFiltered_rowPointerY0 = IMAGE_POINTER(imgFiltered, 0, 0, width);
    u8 * restrict imgFiltered_rowPointerY1 = IMAGE_POINTER(imgFiltered, 1, 0, width);
    for(x=0; x<width; x++) {
			u32 filtered1;
      const u32 filtered0 = imgFilteredTmp_rowPointerY0[x]*kernel[2] + imgFilteredTmp_rowPointerY1[x]*kernel[3] + imgFilteredTmp_rowPointerY2[x]*kernel[4] +
        imgFilteredTmp_rowPointerY0[x]*(kernel[0]+kernel[1]);
      imgFiltered_rowPointerY0[x] = (u8)(filtered0 >> (2*kernelShift));

      filtered1 = imgFilteredTmp_rowPointerY0[x]*kernel[1] + imgFilteredTmp_rowPointerY1[x]*kernel[2] + imgFilteredTmp_rowPointerY2[x]*kernel[3] + imgFilteredTmp_rowPointerY3[x]*kernel[4] +
        imgFilteredTmp_rowPointerY0[x]*kernel[0];
      imgFiltered_rowPointerY1[x] = (u8)(filtered1 >> (2*kernelShift));
    }
  }

  for(y=2; y<(height-2); y++) {
		s32 x;
    //const u32 * restrict imgFilteredTmp_rowPointerYm2 = imgFilteredTmp.Pointer(y-2, 0);
    //const u32 * restrict imgFilteredTmp_rowPointerYm1 = imgFilteredTmp.Pointer(y-1, 0);
    //const u32 * restrict imgFilteredTmp_rowPointerY0  = imgFilteredTmp.Pointer(y,   0);
    //const u32 * restrict imgFilteredTmp_rowPointerYp1 = imgFilteredTmp.Pointer(y+1, 0);
    //const u32 * restrict imgFilteredTmp_rowPointerYp2 = imgFilteredTmp.Pointer(y+2, 0);
    const u32 * restrict imgFilteredTmp_rowPointerYm2 = IMAGE_POINTER(imgFilteredTmp, y-2, 0, width*sizeof(u32));
    const u32 * restrict imgFilteredTmp_rowPointerYm1 = IMAGE_POINTER(imgFilteredTmp, y-1, 0, width*sizeof(u32));
    const u32 * restrict imgFilteredTmp_rowPointerY0  = IMAGE_POINTER(imgFilteredTmp, y,   0, width*sizeof(u32));
    const u32 * restrict imgFilteredTmp_rowPointerYp1 = IMAGE_POINTER(imgFilteredTmp, y+1, 0, width*sizeof(u32));
    const u32 * restrict imgFilteredTmp_rowPointerYp2 = IMAGE_POINTER(imgFilteredTmp, y+2, 0, width*sizeof(u32));

    //u8 * restrict imgFiltered_rowPointerY = imgFiltered.Pointer(y, 0);
    u8 * restrict imgFiltered_rowPointerY = IMAGE_POINTER(imgFiltered, y, 0, width);

    for(x=0; x<width; x++) {
      const u32 filtered = imgFilteredTmp_rowPointerYm2[x]*kernel[0] + imgFilteredTmp_rowPointerYm1[x]*kernel[1] + imgFilteredTmp_rowPointerY0[x]*kernel[2] + imgFilteredTmp_rowPointerYp1[x]*kernel[3] + imgFilteredTmp_rowPointerYp2[x]*kernel[4];
      imgFiltered_rowPointerY[x] = (u8)(filtered >> (2*kernelShift));
    }
  }

  // for y = {height-2,height-1} unrolled
  {
		s32 x;
    //const u32 * restrict imgFilteredTmp_rowPointerYm4 = imgFilteredTmp.Pointer(height-4, 0);
    //const u32 * restrict imgFilteredTmp_rowPointerYm3 = imgFilteredTmp.Pointer(height-3, 0);
    //const u32 * restrict imgFilteredTmp_rowPointerYm2 = imgFilteredTmp.Pointer(height-2, 0);
    //const u32 * restrict imgFilteredTmp_rowPointerYm1 = imgFilteredTmp.Pointer(height-1, 0);
    const u32 * restrict imgFilteredTmp_rowPointerYm4 = IMAGE_POINTER(imgFilteredTmp, height-4, 0, width*sizeof(u32));
    const u32 * restrict imgFilteredTmp_rowPointerYm3 = IMAGE_POINTER(imgFilteredTmp, height-3, 0, width*sizeof(u32));
    const u32 * restrict imgFilteredTmp_rowPointerYm2 = IMAGE_POINTER(imgFilteredTmp, height-2, 0, width*sizeof(u32));
    const u32 * restrict imgFilteredTmp_rowPointerYm1 = IMAGE_POINTER(imgFilteredTmp, height-1, 0, width*sizeof(u32));

    //u8 * restrict imgFiltered_rowPointerYm2 = imgFiltered.Pointer(height-2, 0);
    //u8 * restrict imgFiltered_rowPointerYm1 = imgFiltered.Pointer(height-1, 0);
    u8 * restrict imgFiltered_rowPointerYm2 = IMAGE_POINTER(imgFiltered, height-2, 0, width);
    u8 * restrict imgFiltered_rowPointerYm1 = IMAGE_POINTER(imgFiltered, height-1, 0, width);

    for(x=0; x<width; x++) {
			u32 filteredm2 ;
      const u32 filteredm1 = imgFilteredTmp_rowPointerYm3[x]*kernel[0] + imgFilteredTmp_rowPointerYm2[x]*kernel[1] + imgFilteredTmp_rowPointerYm1[x]*kernel[2] +
        imgFilteredTmp_rowPointerYm1[x]*(kernel[3]+kernel[4]);
      imgFiltered_rowPointerYm1[x] = (u8)(filteredm1 >> (2*kernelShift));

      filteredm2 = imgFilteredTmp_rowPointerYm4[x]*kernel[0] + imgFilteredTmp_rowPointerYm3[x]*kernel[1] + imgFilteredTmp_rowPointerYm2[x]*kernel[2] + imgFilteredTmp_rowPointerYm1[x]*kernel[3] +
        imgFilteredTmp_rowPointerYm1[x]*kernel[4];
      imgFiltered_rowPointerYm2[x] = (u8)(filteredm2 >> (2*kernelShift));
    }
  }

  return 0;
}
