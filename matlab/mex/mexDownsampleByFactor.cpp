#include "mexWrappers.h"

#include "anki/common.h"
#include "anki/vision.h"

#define VERBOSITY 0

using namespace Anki;

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { printf("%s - %s\n", (eventName), (eventValue)); return;}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  ConditionalErrorAndReturn(nrhs == 2 && nlhs == 1, "mexDownsampleByFactor", "Call this function as following: imgDownsampled = mexDownsampleFunction(img, downsampleFactor);");
    
  Array2dUnmanaged<u8> img = mxArray2Array2dUnmanaged<u8>(prhs[0]);
  const u32 downsampleFactor = static_cast<u32>(mxGetScalar(prhs[1]));
  
  ConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexDownsampleByFactor", "Could not allocate Array2dUnmanaged img");
        
  Array2dUnmanaged<u8> imgDownsampled = AllocateArray2dUnmanagedFromHeap<u8>(img.get_size(0)/downsampleFactor, img.get_size(1)/downsampleFactor);
  ConditionalErrorAndReturn(imgDownsampled.get_rawDataPointer() != 0, "mexDownsampleByFactor", "Could not allocate Array2dUnmanaged imgDownsampled");

  const u32 numBytes = imgDownsampled.get_size(0) * imgDownsampled.get_stride() + 1000;
  MemoryStack scratch(calloc(numBytes,1), numBytes);

  if(DownsampleByFactor(img, downsampleFactor, imgDownsampled) != RESULT_OK) {
    printf("Error: mexDownsampleByFactor\n");
  }
  
  plhs[0] = array2dUnmanaged2mxArray(imgDownsampled);
  
  delete(img.get_rawDataPointer());
  delete(imgDownsampled.get_rawDataPointer());
  free(scratch.get_buffer());
}