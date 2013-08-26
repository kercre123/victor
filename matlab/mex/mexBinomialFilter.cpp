#include "mexWrappers.h"

#include "anki/common.h"
#include "anki/vision.h"

#define VERBOSITY 0

using namespace Anki;

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { printf("%s - %s\n", (eventName), (eventValue)); return;}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  ConditionalErrorAndReturn(nrhs == 1 && nlhs == 1, "mexBinomialFilter", "Call this function as following: imgFiltered = mexBinomialFilter(img);");
    
  Array2dUnmanaged<u8> img = mxArray2Array2dUnmanaged<u8>(prhs[0]);
  
  ConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexBinomialFilter", "Could not allocate Array2dUnmanaged img");
        
  Array2dUnmanaged<u8> imgFiltered = AllocateArray2dUnmanagedFromHeap<u8>(img.get_size(0), img.get_size(1));
  ConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexBinomialFilter", "Could not allocate Array2dUnmanaged imgFiltered");

  const u32 numBytes = img.get_size(0) * img.get_stride() + 1000;
  MemoryStack scratch(calloc(numBytes,1), numBytes);

  if(BinomialFilter(img, imgFiltered, scratch) != RESULT_OK) {
    printf("Error: mexBinomialFilter\n");
  }
  
  plhs[0] = array2dUnmanaged2mxArray(imgFiltered);
  
  delete(img.get_rawDataPointer());
  delete(imgFiltered.get_rawDataPointer());
  free(scratch.get_buffer());
}