#include "mexWrappers.h"
#include "mexEmbeddedWrappers.h"

#include "anki/embeddedCommon.h"
#include "anki/embeddedVision.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { printf("%s - %s\n", (eventName), (eventValue)); return;}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  ConditionalErrorAndReturn(nrhs == 2 && nlhs == 1, "mexDownsampleByFactor", "Call this function as following: imgDownsampled = mexDownsampleFunction(img, downsampleFactor);");
    
  Array_u8 img = mxArrayToArray_u8(prhs[0]);
  const u32 downsampleFactor = static_cast<u32>(mxGetScalar(prhs[1]));
  
  ConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexDownsampleByFactor", "Could not allocate Array2dUnmanaged img");
        
  Array_u8 imgDownsampled = AllocateArrayFromHeap_u8(img.get_size(0)/downsampleFactor, img.get_size(1)/downsampleFactor);
  ConditionalErrorAndReturn(imgDownsampled.get_rawDataPointer() != 0, "mexDownsampleByFactor", "Could not allocate Array2dUnmanaged imgDownsampled");

  const u32 numBytes = imgDownsampled.get_size(0) * imgDownsampled.get_stride() + 1000;
  MemoryStack scratch(calloc(numBytes,1), numBytes);

  if(DownsampleByFactor(img, downsampleFactor, imgDownsampled) != RESULT_OK) {
    printf("Error: mexDownsampleByFactor\n");
  }
  
  plhs[0] = arrayToMxArray_u8(imgDownsampled);
  
  free(img.get_rawDataPointer());
  free(imgDownsampled.get_rawDataPointer());
  free(scratch.get_buffer());
}