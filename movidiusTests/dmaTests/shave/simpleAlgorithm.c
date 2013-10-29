#include "shaveShared.h"

void simpleAdd(const u8 * restrict im1, const u8 * restrict im2, u8 * restrict out, int numElements)
{
  s32 i;

  for(i=0; i<numElements; i++) {
    out[i] = im1[i] + im2[i];
  }
}