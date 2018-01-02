#include "mex.h"

#include "anki/common/types.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/config.h"
#include "anki/common/robot/matlabInterface.h"

#include "coretech/vision/robot/imageProcessing.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  const s32 bufferSize = 10000000;
  MemoryStack memory(malloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(memory.IsValid(), "mexBinomialFilter", "Memory could not be allocated");

  AnkiConditionalErrorAndReturn(nrhs == 1 && nlhs == 1, "mexBinomialFilter", "Call this function as following: imgFiltered = mexBinomialFilter(img);");

  Array<u8> img = mxArrayToArray<u8>(prhs[0], memory);

  AnkiConditionalErrorAndReturn(img.get_buffer() != 0, "mexBinomialFilter", "Could not allocate Array<u8> img");

  Array<u8> imgFiltered(img.get_size(0), img.get_size(1), memory);
  AnkiConditionalErrorAndReturn(img.get_buffer() != 0, "mexBinomialFilter", "Could not allocate Array<u8> imgFiltered");

  if(ImageProcessing::BinomialFilter<u8,u32,u8>(img, imgFiltered, memory) != Anki::RESULT_OK) {
    Anki::CoreTechPrint("Error: mexBinomialFilter\n");
  }

  plhs[0] = arrayToMxArray<u8>(imgFiltered);

  free(memory.get_buffer());
}
