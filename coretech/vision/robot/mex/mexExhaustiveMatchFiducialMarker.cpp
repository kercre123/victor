#include "mex.h"

#include "anki/common/robot/matlabInterface.h"

#include "coretech/vision/robot/fiducialMarkers.h"
#include "coretech/vision/robot/fiducialDetection.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

#include <string.h>
#include <vector>

using namespace Anki::Embedded;

// First call mexLoadExhaustiveMatchDatabase to get {numDatabaseImages, databaseImageHeight, databaseImageWidth, databaseImages, databaseLabelIndexes}

// [markerName, orientation, matchQuality] = mexExhaustiveMatchFiducialMarker(uint8(image), quad, numDatabaseImages, databaseImageHeight, databaseImageWidth, databaseImages, databaseLabelIndexes);

// Example:
// im = imread('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/../images/cozmo_date2014_06_04_time16_52_38_frame0.png');
// quad = [64.1757,  46.0676; 61.9054,  114.8243; 134.8784, 44.7703; 133.9054, 115.1486];
// [markerName, orientation, matchQuality] = mexExhaustiveMatchFiducialMarker(uint8(im), quad, numDatabaseImages, databaseImageHeight, databaseImageWidth, databaseImages, databaseLabelIndexes)

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  //const s32 bufferSize = 100000000;
  const s32 bufferSize = 1000000;

  AnkiConditionalErrorAndReturn(nrhs == 7 && nlhs == 3, "mexExhaustiveMatchFiducialMarker", "Call this function as following: [markerName, orientation, matchQuality] = mexExhaustiveMatchFiducialMarker(uint8(image), quad, numDatabaseImages, databaseImageHeight, databaseImageWidth, databaseImages, databaseLabelIndexes);");

  MemoryStack memory1(mxMalloc(bufferSize), bufferSize);
  MemoryStack memory2(mxMalloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(AreValid(memory1, memory2), "mexExhaustiveMatchFiducialMarker", "Memory could not be allocated");

  Array<u8> image               = mxArrayToArray<u8>(prhs[0], memory1);
  Array<f64> quadF64            = mxArrayToArray<f64>(prhs[1], memory1);
  const s32 numDatabaseImages   = saturate_cast<s32>(mxGetScalar(prhs[2]));
  const s32 databaseImageHeight = saturate_cast<s32>(mxGetScalar(prhs[3]));
  const s32 databaseImageWidth  = saturate_cast<s32>(mxGetScalar(prhs[4]));
  u8 * const pDatabaseImages    = reinterpret_cast<u8 *>( mxGetData(prhs[5]) );
  Anki::Vision::MarkerType * const pDatabaseLabelIndexes = reinterpret_cast<Anki::Vision::MarkerType *>( mxGetData(prhs[6]) );

  AnkiConditionalErrorAndReturn(AreValid(image) && pDatabaseImages && pDatabaseLabelIndexes, "mexExhaustiveMatchFiducialMarker", "Could not allocate inputs");

  AnkiConditionalErrorAndReturn(AreEqualSize(4, 2, quadF64), "mexExhaustiveMatchFiducialMarker", "quad must be 4x2");

  Quadrilateral<f32> quad;
  for(s32 i=0; i<4; i++) {
    quad[i] = Point<f32>(static_cast<f32>(quadF64[i][0]), static_cast<f32>(quadF64[i][1]));
  }

  VisionMarkerImages allMarkerImages(
    numDatabaseImages,
    databaseImageHeight,
    databaseImageWidth,
    pDatabaseImages,
    pDatabaseLabelIndexes);

  VisionMarker extractedMarker;
  f32 matchQuality;
  allMarkerImages.MatchExhaustive(image, quad, extractedMarker, matchQuality, memory1, memory2);

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
