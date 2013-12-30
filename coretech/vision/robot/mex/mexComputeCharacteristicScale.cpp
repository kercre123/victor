#include "mex.h"

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/fixedLengthList.h"

#include "anki/vision/robot/miscVisionKernels.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  AnkiConditionalErrorAndReturn(nrhs == 2 && nlhs == 1, "mexComputeCharacteristicScale", "Call this function as following: scaleImage = mexComputeCharacteristicScale(img, numPyramidLevels);");

  const s32 bufferSize = 10000000;
  MemoryStack memory(malloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(memory.IsValid(), "mexComputeCharacteristicScale", "Memory could not be allocated");

  Array<u8> img = mxArrayToArray<u8>(prhs[0], memory);
  const s32 numPyramidLevels = static_cast<s32>(mxGetScalar(prhs[1]));

  AnkiConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexComputeCharacteristicScale", "Could not allocate Array img");
  AnkiConditionalErrorAndReturn(numPyramidLevels < 8, "mexComputeCharacteristicScale", "numPyramidLevels must be less than 8");

  FixedPointArray<u32> scaleImage(img.get_size(0), img.get_size(1), 16, memory);
  AnkiConditionalErrorAndReturn(scaleImage.get_rawDataPointer() != 0, "mexComputeCharacteristicScale", "Could not allocate FixedPointArray scaleImage");

  if(ComputeCharacteristicScaleImage(img, numPyramidLevels, scaleImage, memory) != RESULT_OK) {
    printf("Error: mexComputeCharacteristicScale\n");
  }

  plhs[0] = arrayToMxArray<u32>(scaleImage);

  free(memory.get_buffer());
}