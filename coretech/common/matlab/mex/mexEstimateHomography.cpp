
#include "anki/common/robot/fixedLengthList.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/opencvLight.h"

#include "mex.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { mexPrintf("%s - %s\n", (eventName), (eventValue)); return;}

// homography_groundTruth = [5.5, -0.3, 5.5; 0.5, 0.5, 3.3; 0.001, 0.0, 1.0];
// originalPoints = [0, 1, 1, 0; 0, 0, 1, 1; 1, 1, 1, 1];
// transformedPoints = homography_groundTruth * originalPoints; transformedPoints = transformedPoints(1:2,:) ./ repmat(transformedPoints(3,:), [2,1]);
// originalPoints = originalPoints(1:2,:)'; transformedPoints = transformedPoints';
// homography = mexEstimateHomography(originalPoints, transformedPoints);

/*
void Print_FixedLengthList_Point<f64>(FixedLengthList_Point<f64> &arr, const char * const variableName)
{
printf("%s:\n", variableName);
const Point<f64> * const rowPointer = arr.Pointer(0);
for(s32 y=0; y<arr.get_size(); y++) {
printf("(%f,%f) ", rowPointer[y].x, rowPointer[y].y);
}
printf("\n");
}
*/

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  ConditionalErrorAndReturn(nrhs == 2 && nlhs == 1,
    "mexEstimateHomography",
    "Call this function as following: homography = mexEstimateHomography(originalPoints, transformedPoints); originalPoints and transformedPoints should be double-precision and Nx2");

  Array<f64> originalPointsArray = mxArrayToArray<f64>(prhs[0]);
  Array<f64> transformedPointsArray = mxArrayToArray<f64>(prhs[1]);

  ConditionalErrorAndReturn(originalPointsArray.get_size(1) == 2 && transformedPointsArray.get_size(1) == 2 && originalPointsArray.get_size(0) == transformedPointsArray.get_size(0),
    "mexEstimateHomography",
    "originalPoints and transformedPoints should be double-precision and Nx2");

  Array<f64> homography = AllocateArrayFromHeap<f64>(3, 3);

  FixedLengthList<Point<f64> > originalPointsList = AllocateFixedLengthListFromHeap<Point<f64> >(originalPointsArray.get_size(0));
  FixedLengthList<Point<f64> > transformedPointsList = AllocateFixedLengthListFromHeap<Point<f64> >(originalPointsArray.get_size(0));

  const s32 numBytes = 10000000;
  MemoryStack scratch(calloc(numBytes,1), numBytes);

  for(s32 i=0; i<originalPointsArray.get_size(0); i++) {
    const f64 x0 = *originalPointsArray.Pointer(i,0);
    const f64 y0 = *originalPointsArray.Pointer(i,1);

    const f64 x1 = *transformedPointsArray.Pointer(i,0);
    const f64 y1 = *transformedPointsArray.Pointer(i,1);

    originalPointsList.PushBack(Point<f64>(x0, y0));
    transformedPointsList.PushBack(Point<f64>(x1, y1));
  }

  // Print_FixedLengthList_Point<f64>(originalPointsList, "originalPointsList");
  // Print_FixedLengthList_Point<f64>(transformedPointsList, "transformedPointsList");

  const Result result = EstimateHomography(originalPointsList, transformedPointsList, homography, scratch);

  ConditionalErrorAndReturn(result == RESULT_OK,
    "mexEstimateHomography",
    "EstimateHomography failed");

  plhs[0] = arrayToMxArray<f64>(homography);

  free(originalPointsArray.get_rawDataPointer());
  free(transformedPointsArray.get_rawDataPointer());
  free(originalPointsList.get_rawDataPointer());
  free(transformedPointsList.get_rawDataPointer());
  free(scratch.get_buffer());
}