#include "mex.h"

#include "anki/embeddedCommon.h"
#include "anki/embeddedVision.h"

#include <string.h>

#define VERBOSITY 0

using namespace Anki::Embedded;

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { printf("%s - %s\n", (eventName), (eventValue)); return;}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  // components2d = simpleDetector_step3_simpleRejectionTests(nrows, ncols, numRegions, area, indexList, bb, centroid, usePerimeterCheck, components2d, embeddedConversions, DEBUG_DISPLAY)
    
//     IN_DDR Result SimpleDetector_Steps123(
//       const Array<u8> &image,
//       const s32 scaleImage_numPyramidLevels,
//       const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
//       const s32 component_minimumNumPixels, const s32 component_maximumNumPixels,
//       const s32 component_sparseMultiplyThreshold, const s32 component_solidMultiplyThreshold,
//       MemoryStack scratch1,
//       MemoryStack scratch2)
    

    
  ConditionalErrorAndReturn(nrhs == 8 && nlhs == 1, "mexSimpleDetectorSteps123", "Call this function as following: components2d = mexSimpleDetectorSteps123(uint8(image), scaleImage_numPyramidLevels, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold);");

  Array<u8> binaryImg = mxArrayToArray<u8>(prhs[0]);
  Array<f64> startPointMatrix = mxArrayToArray<f64>(prhs[1]);
  char * initialDirectionString = mxArrayToString(prhs[2]);

  //printf("%f %f %s\n", *startPoint.Pointer(0,0), *startPoint.Pointer(0,1), initialDirection.data());
  ConditionalErrorAndReturn(binaryImg.get_rawDataPointer() != 0, "mexTraceBoundary", "Could not allocate Matrix binaryImg");
  ConditionalErrorAndReturn(startPointMatrix.get_rawDataPointer() != 0, "mexTraceBoundary", "Could not allocate Matrix startPointMatrix");

  FixedLengthList<Point<s16> > boundary = AllocateFixedLengthListFromHeap<Point<s16> >(MAX_BOUNDARY_LENGTH);
  ConditionalErrorAndReturn(boundary.get_rawDataPointer() != 0, "mexTraceBoundary", "Could not allocate FixedLengthList boundary");
  ConditionalErrorAndReturn(initialDirectionString != 0, "mexTraceBoundary", "Could not read initialDirectionString");

  Point<s16> startPoint(static_cast<s16>(*startPointMatrix.Pointer(0,1)-1), static_cast<s16>(*startPointMatrix.Pointer(0,0)-1));
  const BoundaryDirection initialDirection = stringToBoundaryDirection(initialDirectionString);

  if(TraceBoundary(binaryImg, startPoint, initialDirection, boundary) != RESULT_OK) {
    printf("Error: mexTraceBoundary\n");
  }

  Array<f64> boundaryMatrix = AllocateArrayFromHeap<f64>(boundary.get_size(), 2);

  for(s32 i=0; i<boundary.get_size(); i++) {
    *boundaryMatrix.Pointer(i,1) = boundary.Pointer(i)->x + 1;
    *boundaryMatrix.Pointer(i,0) = boundary.Pointer(i)->y + 1;
  }

  plhs[0] = arrayToMxArray<f64>(boundaryMatrix);

  free(binaryImg.get_rawDataPointer());
  free(startPointMatrix.get_rawDataPointer());
  free(boundaryMatrix.get_rawDataPointer());
  mxFree(initialDirectionString);
}