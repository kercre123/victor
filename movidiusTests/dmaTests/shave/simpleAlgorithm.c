#include "shaveShared.h"

void simpleAdd(const s32 * restrict im1, const s32 * restrict im2, s32 * restrict out, int numElements)
{
  s32 i;

  for(i=0; i<numElements; i++) {
    out[i] = im1[i] + im2[i];
  }
}