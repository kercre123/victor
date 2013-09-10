#include "mexWrappers.h"
#include "mexEmbeddedWrappers.h"

#include "anki/embeddedCommon.h"
#include "anki/embeddedVision.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { printf("%s - %s\n", (eventName), (eventValue)); return;}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  ConditionalErrorAndReturn(nrhs == 2 && nlhs == 1, "mexComputeCharacteristicScale", "Call this function as following: scaleImage = mexComputeCharacteristicScale(img, numLevels);");
    
  Array_u8 img = mxArrayToArray_u8(prhs[0]);
  const u32 numLevels = static_cast<u32>(mxGetScalar(prhs[1]));
  
  ConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexComputeCharacteristicScale", "Could not allocate Array2dUnmanaged img");
  ConditionalErrorAndReturn(numLevels < 8, "mexComputeCharacteristicScale", "numLevels must be less than 8");
        
  Array_u32 scaleImage = AllocateArrayFromHeap_u32(img.get_size(0), img.get_size(1), 16);
  ConditionalErrorAndReturn(scaleImage.get_rawDataPointer() != 0, "mexComputeCharacteristicScale", "Could not allocate Array2dUnmanaged scaleImage");

  const u32 numBytes = 20 * img.get_size(0) * img.get_stride() + 1000;
  MemoryStack scratch(calloc(numBytes,1), numBytes);

  if(ComputeCharacteristicScaleImage(img, numLevels, scaleImage, scratch) != RESULT_OK) {
    printf("Error: mexComputeCharacteristicScale\n");
  }
  
  plhs[0] = arrayToMxArray_u32(scaleImage);
  
  free(img.get_rawDataPointer());
  free(scaleImage.get_rawDataPointer());
  free(scratch.get_buffer());
}