#include "mex.h"

#include <string.h>

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/compress.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

// inputArray = imread('Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/symbolsWithFiducials/rotated/batteries_000.png');
// compressedVector = mexCompress(uint8(inputArray));
// inputArray2 = mexDecompress(uint8(compressedVector), size(inputArray,1), size(inputArray,2));

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  AnkiConditionalErrorAndReturn(nrhs == 3 && nlhs == 1, "mexCompress", "Call this function as following: outputArray = mexCompress(uint8(compressedVector), outputHeight, outputWidth);");

  const s32 bufferSize = 10000000;
  MemoryStack memory(malloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(memory.IsValid(), "mexCompress", "Memory could not be allocated");

  const s32 outMaxLength = bufferSize / 2;
  void *outRaw = memory.Allocate(outMaxLength);

  Array<u8> inputArray = mxArrayToArray<u8>(prhs[0], memory);
  const s32 outputHeight = static_cast<s32>(mxGetScalar(prhs[1]));
  const s32 outputWidth = static_cast<s32>(mxGetScalar(prhs[2]));

  Array<u8> outputArray = Decompress<u8>(
    reinterpret_cast<void*>(inputArray.Pointer(0,0)), inputArray.get_size(1),
    outputHeight, outputWidth, Flags::Buffer(false,false,false),
    memory);

  plhs[0] = arrayToMxArray<u8>(outputArray);

  free(memory.get_buffer());
}