#include "mex.h"

#include "anki/common/robot/matlabInterface.h"

#include "anki/vision/robot/fiducialMarkers.h"
#include "anki/vision/robot/fiducialDetection.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

#include <string.h>
#include <vector>

using namespace Anki::Embedded;

// First call mexLoadExhaustiveMatchDatabase to get {numDatabaseImages, databaseImageHeight, databaseImageWidth, databaseImages, databaseLabelIndexes}

// [markerName, orientation, matchQuality] = mexExhaustiveMatchFiducialMarker(uint8(image), quad, numDatabaseImages, databaseImageHeight, databaseImageWidth, databaseImages, databaseLabelIndexes);

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  const s32 bufferSize = 100000000;

  AnkiConditionalErrorAndReturn(nrhs == 7 && nlhs == 3, "mexExhaustiveMatchFiducialMarker", "Call this function as following: [markerName, orientation, matchQuality] = mexExhaustiveMatchFiducialMarker(uint8(image), quad, numDatabaseImages, databaseImageHeight, databaseImageWidth, databaseImages, databaseLabelIndexes);");

  MemoryStack memory1(mxMalloc(bufferSize), bufferSize);
  MemoryStack memory2(mxMalloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(AreValid(memory1, memory2), "mexExhaustiveMatchFiducialMarker", "Memory could not be allocated");

  Array<u8> image                       = mxArrayToArray<u8>(prhs[0], memory1);
  Array<f64> quadF64                    = mxArrayToArray<f64>(prhs[1], memory1);
  const s32 numDatabaseImages           = saturate_cast<s32>(mxGetScalar(prhs[2]));
  const s32 databaseImageHeight         = saturate_cast<s32>(mxGetScalar(prhs[3]));
  const s32 databaseImageWidth          = saturate_cast<s32>(mxGetScalar(prhs[4]));
  const Array<u8> databaseImages        = mxArrayToArray<u8>(prhs[5], memory1);
  const Array<s32> databaseLabelIndexes = mxArrayToArray<s32>(prhs[6], memory1);

  AnkiConditionalErrorAndReturn(AreValid(image, databaseImages, databaseLabelIndexes), "mexExhaustiveMatchFiducialMarker", "Could not allocate inputs");

  AnkiConditionalErrorAndReturn(AreEqualSize(4, 2, quadF64), "mexExhaustiveMatchFiducialMarker", "quad must be 4x2");

  Quadrilateral<f32> quad;
  for(s32 i=0; i<4; i++) {
    quad[i] = Point<f32>(static_cast<f32>(quadF64[i][0]), static_cast<f32>(quadF64[i][1]));
  }

  VisionMarkerImages allMarkerImages(
    numDatabaseImages,
    databaseImageHeight,
    databaseImageWidth,
    const_cast<u8*>(reinterpret_cast<const u8*>(databaseImages.get_buffer())),
    const_cast<Anki::Vision::MarkerType*>(reinterpret_cast<const Anki::Vision::MarkerType*>(databaseLabelIndexes.get_buffer())));

  VisionMarker extractedMarker;
  f32 matchQuality;
  const Anki::Result result = allMarkerImages.MatchExhaustive(image, quad, extractedMarker, matchQuality, memory1, memory2);

  plhs[0] = mxCreateString(Anki::Vision::MarkerTypeStrings[extractedMarker.markerType]);

  {
    Array<f32> oneOutput(1,1,memory1);
    *oneOutput.Pointer(0,0) = extractedMarker.observedOrientation;
    plhs[1] = arrayToMxArray(oneOutput);
  }

  {
    Array<f32> oneOutput(1,1,memory1);
    *oneOutput.Pointer(0,0) = matchQuality;
    plhs[2] = arrayToMxArray(oneOutput);
  }

  mxFree(memory1.get_buffer());
  mxFree(memory2.get_buffer());
}