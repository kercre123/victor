#include "mex.h"

#include "anki/common/types.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/config.h"
#include "anki/common/robot/matlabInterface.h"

#include "anki/vision/robot/imageProcessing.h"
#include "anki/vision/robot/miscVisionKernels.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  AnkiConditionalErrorAndReturn(nrhs == 1 && nlhs == 1, "mexBinomialFilter", "Call this function as following: imgFiltered = mexBinomialFilter(img);");

  Array<u8> img = mxArrayToArray<u8>(prhs[0]);

  AnkiConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexBinomialFilter", "Could not allocate Array<u8> img");

  Array<u8> imgFiltered = AllocateArrayFromHeap<u8>(img.get_size(0), img.get_size(1));
  AnkiConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexBinomialFilter", "Could not allocate Array<u8> imgFiltered");

  const u32 numBytes = img.get_size(0) * img.get_stride() + 1000;
  MemoryStack scratch(calloc(numBytes,1), numBytes);

  if(ImageProcessing::BinomialFilter<u8,u32,u8>(img, imgFiltered, scratch) != RESULT_OK) {
    printf("Error: mexBinomialFilter\n");
  }

  plhs[0] = arrayToMxArray<u8>(imgFiltered);

  free(img.get_rawDataPointer());
  free(imgFiltered.get_rawDataPointer());
  free(scratch.get_buffer());
}