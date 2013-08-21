#include "mexWrappers.h"

#include "anki/common.h"
#include "anki/vision.h"

#define VERBOSITY 0

using namespace Anki;

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { printf("%s - %s\n", (eventName), (eventValue)); return;}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  ConditionalErrorAndReturn(nrhs == 2 && nlhs == 1, "mexComputeCharacteristicScale", "Call this function as following: scaleImage = mexComputeCharacteristicScale(img, numLevels);");
    
  Matrix<u8> img = mxArray2AnkiMatrix<u8>(prhs[0]);
  const u32 numLevels = static_cast<u32>(mxGetScalar(prhs[1]));
  
  ConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexComputeCharacteristicScale", "Could not allocate Matrix img");
  ConditionalErrorAndReturn(numLevels < 8, "mexComputeCharacteristicScale", "numLevels must be less than 8");
        
  FixedPointMatrix<u32> scaleImage = AllocateFixedPointMatrixFromHeap<u32>(img.get_size(0), img.get_size(1), 16);
  ConditionalErrorAndReturn(scaleImage.get_rawDataPointer() != 0, "mexComputeCharacteristicScale", "Could not allocate FixedPointMatrix scaleImage");

  const u32 numBytes = 20 * img.get_size(0) * img.get_stride() + 1000;
  MemoryStack scratch(calloc(numBytes,1), numBytes);

  if(ComputeCharacteristicScaleImage(img, numLevels, scaleImage, scratch) != RESULT_OK) {
    printf("Error: mexComputeCharacteristicScale\n");
  }
  
  plhs[0] = ankiMatrix2mxArray(scaleImage);
  
  delete(img.get_rawDataPointer());
  delete(scaleImage.get_rawDataPointer());
  free(scratch.get_buffer());
}