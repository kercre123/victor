#include "mexWrappers.h"

#include "anki/common.h"
#include "anki/vision.h"

#define VERBOSITY 0

using namespace Anki;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  DASConditionalErrorAndReturn(nrhs == 2 && nlhs == 1, "mexDownsampleByFactor", "Call this function as following: imgDownsampled = mexDownsampleFunction(img, downsampleFactor);");
    
  Matrix<u8> img = mxArray2AnkiMatrix<u8>(prhs[0]);
  const u32 downsampleFactor = static_cast<u32>(mxGetScalar(prhs[1]));
  
  DASConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexDownsampleByFactor", "Could not allocate Matrix img");
        
  Matrix<u8> imgDownsampled = AllocateMatrixFromHeap<u8>(img.get_size(0)/downsampleFactor, img.get_size(1)/downsampleFactor);
  DASConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexDownsampleByFactor", "Could not allocate Matrix imgDownsampled");

  const u32 numBytes = imgDownsampled.get_size(0) * imgDownsampled.get_stride() + 1000;
  MemoryStack scratch(calloc(numBytes,1), numBytes);

  if(DownsampleByFactor(img, downsampleFactor, imgDownsampled) != RESULT_OK) {
    printf("Error: mexDownsampleByFactor\n");
  }
  
  plhs[0] = ankiMatrix2mxArray(imgDownsampled);
  
  delete(img.get_rawDataPointer());
  delete(imgDownsampled.get_rawDataPointer());
  free(scratch.get_buffer());
}