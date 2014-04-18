#include "mex.h"

#include <string.h>

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/compress.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

// inputArray = imread('Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/symbolsWithFiducials/rotated/batteries_000.png');
// compressedVector = mexCompress(uint8(inputArray));

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  AnkiConditionalErrorAndReturn(nrhs == 1 && nlhs == 1, "mexCompress", "Call this function as following: compressedVector = mexCompress(uint8(inputArray));");

  const s32 bufferSize = 10000000;
  MemoryStack memory(malloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(memory.IsValid(), "mexCompress", "Memory could not be allocated");

  const s32 outMaxLength = bufferSize / 2;
  void *outRaw = memory.Allocate(outMaxLength);

  Array<u8> inputArray = mxArrayToArray<u8>(prhs[0], memory);

  s32 outCompressedLength;
  const Anki::Result result = Compress(inputArray, outRaw, outMaxLength, outCompressedLength, memory);

  Array<u8> compressedVector = Array<u8>(1, outCompressedLength, memory);
  compressedVector.Set(reinterpret_cast<u8*>(outRaw), outCompressedLength);

  plhs[0] = arrayToMxArray<u8>(compressedVector);

  free(memory.get_buffer());
}