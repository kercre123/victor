#include "mex.h"

#include <string.h>

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/fixedLengthList.h"

#include "anki/vision/robot/miscVisionKernels.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { printf("%s - %s\n", (eventName), (eventValue)); return;}

BoundaryDirection stringToBoundaryDirection(const std::string direction)
{
  if(_strcmpi(direction.data(), "n") == 0) {
    return BOUNDARY_N;
  } else if(_strcmpi(direction.data(), "ne") == 0) {
    return BOUNDARY_NE;
  } else if(_strcmpi(direction.data(), "e") == 0) {
    return BOUNDARY_E;
  } else if(_strcmpi(direction.data(), "se") == 0) {
    return BOUNDARY_SE;
  } else if(_strcmpi(direction.data(), "s") == 0) {
    return BOUNDARY_S;
  } else if(_strcmpi(direction.data(), "sw") == 0) {
    return BOUNDARY_SW;
  } else if(_strcmpi(direction.data(), "w") == 0) {
    return BOUNDARY_W;
  } else if(_strcmpi(direction.data(), "nw") == 0) {
    return BOUNDARY_NW;
  }

  return BOUNDARY_UNKNOWN;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  ConditionalErrorAndReturn(nrhs == 3 && nlhs == 1, "mexTraceBoundary", "Call this function as following: boundary = mexTraceBoundary(uint8(binaryImg), double(startPoint), char(initialDirection));");

  Array<u8> binaryImg = mxArrayToArray<u8>(prhs[0]);
  Array<f64> startPointMatrix = mxArrayToArray<f64>(prhs[1]);
  char * initialDirectionString = mxArrayToString(prhs[2]);

  //printf("%f %f %s\n", *startPoint.Pointer(0,0), *startPoint.Pointer(0,1), initialDirection.data());
  ConditionalErrorAndReturn(binaryImg.get_rawDataPointer() != 0, "mexTraceBoundary", "Could not allocate Matrix binaryImg");
  ConditionalErrorAndReturn(startPointMatrix.get_rawDataPointer() != 0, "mexTraceBoundary", "Could not allocate Matrix startPointMatrix");

  FixedLengthList<Point<s16> > boundary = AllocateFixedLengthListFromHeap<Point<s16> >(MAX_BOUNDARY_LENGTH);
  ConditionalErrorAndReturn(boundary.get_array().get_rawDataPointer() != 0, "mexTraceBoundary", "Could not allocate FixedLengthList boundary");
  ConditionalErrorAndReturn(initialDirectionString != 0, "mexTraceBoundary", "Could not read initialDirectionString");

  Point<s16> startPoint(static_cast<s16>(*startPointMatrix.Pointer(0,1)-1), static_cast<s16>(*startPointMatrix.Pointer(0,0)-1));
  const BoundaryDirection initialDirection = stringToBoundaryDirection(initialDirectionString);

  if(TraceInteriorBoundary(binaryImg, startPoint, initialDirection, boundary) != RESULT_OK) {
    printf("Error: mexTraceInteriorBoundary\n");
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