#include "mexWrappers.h"

#include "anki/common.h"
#include "anki/vision.h"

#define VERBOSITY 0

using namespace Anki;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Matrix<u8> img = mxArray2AnkiMatrix<u8>(prhs[0]);
  
  DASConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexBinomialFilter", "Could not allocate Matrix img");
        
  Matrix<u8> imgFiltered = AllocateMatrixFromHeap<u8>(img.get_size(0), img.get_size(1));
  DASConditionalErrorAndReturn(img.get_rawDataPointer() != 0, "mexBinomialFilter", "Could not allocate Matrix imgFiltered");

  const u32 numBytes = img.get_size(0) * img.get_stride() + 1000;
  MemoryStack scratch(calloc(numBytes,1), numBytes);

  if(BinomialFilter(img, imgFiltered, scratch) != RESULT_OK) {
    printf("Error: mexBinomialFilter\n");
  }
  
  plhs[0] = ankiMatrix2mxArray(imgFiltered);
  
  delete(img.get_rawDataPointer());
  delete(imgFiltered.get_rawDataPointer());
  free(scratch.get_buffer());
}