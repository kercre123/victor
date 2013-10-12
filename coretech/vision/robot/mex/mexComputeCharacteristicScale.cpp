#include "mex.h"

#include "anki/embeddedCommon.h"
#include "anki/embeddedVision.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { mexPrintf("%s - %s\n", (eventName), (eventValue)); return;}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  ConditionalErrorAndReturn(nrhs == 2 && nlhs == 1, "mexComputeCharacteristicScale", "Call this function as following: scaleImage = mexComputeCharacteristicScale(img, numPyramidLevels);");
  
  Array<u8> img = mxArrayToArray<u8>(prhs[0]);
  const s32 numPyramidLevels = static_cast<s32>(mxGetScalar(prhs[1]));

  ConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexComputeCharacteristicScale", "Could not allocate Array img");
  ConditionalErrorAndReturn(numPyramidLevels < 8, "mexComputeCharacteristicScale", "numPyramidLevels must be less than 8");

  FixedPointArray<u32> scaleImage = AllocateFixedPointArrayFromHeap<u32>(img.get_size(0), img.get_size(1), 16);
  ConditionalErrorAndReturn(scaleImage.get_rawDataPointer() != 0, "mexComputeCharacteristicScale", "Could not allocate FixedPointArray scaleImage");

  const u32 numBytes = 40 * img.get_size(0) * img.get_stride() + 1000;
  MemoryStack scratch(calloc(numBytes,1), numBytes);

  ConditionalErrorAndReturn(scratch.IsValid(), "mexComputeCharacteristicScale", "Scratch could not be allocated");
  
  if(ComputeCharacteristicScaleImage(img, numPyramidLevels, scaleImage, scratch) != RESULT_OK) {
    printf("Error: mexComputeCharacteristicScale\n");
  }

  plhs[0] = arrayToMxArray<u32>(scaleImage);

  free(img.get_rawDataPointer());
  free(scaleImage.get_rawDataPointer());
  free(scratch.get_buffer());
   
}