#include <stdio.h>
#include <stdlib.h>

#include "allFunctions.h"

#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 480

int main()
{
  u8 * img = (u8*)malloc(IMAGE_WIDTH*IMAGE_HEIGHT*sizeof(u8));
  u8 * imgFiltered = (u8*)malloc(IMAGE_WIDTH*IMAGE_HEIGHT*sizeof(u8));
  u32 * imgFilteredTmp = (u32*)malloc(IMAGE_WIDTH*IMAGE_HEIGHT*sizeof(u32));

  BinomialFilter(img, imgFiltered, imgFilteredTmp, IMAGE_HEIGHT, IMAGE_WIDTH);

  free(img); img = NULL;
  free(imgFiltered); imgFiltered = NULL;
  free(imgFilteredTmp); imgFilteredTmp = NULL;

  return 0;
}
