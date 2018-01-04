#include "mex.h"

#include "anki/common/robot/matlabInterface.h"

#include "coretech/vision/robot/fiducialMarkers.h"
#include "coretech/vision/robot/fiducialDetection.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

#define VERBOSITY 0

using namespace Anki::Embedded;


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  const s32 bufferSize = 500000;
  u8 buffer[bufferSize];
  MemoryStack scratch(buffer, bufferSize);
  AnkiConditionalErrorAndReturn(scratch.IsValid(), "mexRefineQuadrilateral", "Memory could not be allocated");
  
  s32 argIndex = 0;
  
  AnkiConditionalErrorAndReturn(mxGetClassID(prhs[0]) == mxUINT8_CLASS, "mexRefineQuadrilateral", "prhs[0] should be UINT8");
  Array<u8> image = mxArrayToArray<u8>(prhs[argIndex++], scratch);
  
  AnkiConditionalErrorAndReturn(image.IsValid(), "mexRefineQuadrilateral", "Could not create %dx%d image in %d-byte buffer.\n", mxGetM(prhs[0]), mxGetN(prhs[0]), bufferSize);
  AnkiConditionalErrorAndReturn(mxGetClassID(prhs[1]) == mxINT16_CLASS, "mexRefineQuadrilateral", "prhs[1] should be INT16");
  AnkiConditionalErrorAndReturn(mxGetM(prhs[1]) == 4 && mxGetN(prhs[1]) == 2, "mexRefineQuadrilateral", "prhs[1] should be 4x2 array");
  Array<s16> initQuadData = mxArrayToArray<s16>(prhs[argIndex++], scratch);
  AnkiConditionalErrorAndReturn(initQuadData.get_size(0) == 4 && initQuadData.get_size(1) == 2, "mexRefineQuadrilateral", "initial quad should be 4x2 array");
  Quadrilateral<s16> initialQuad(Point<s16>(*initQuadData.Pointer(0, 0), *initQuadData.Pointer(0, 1)),
                                 Point<s16>(*initQuadData.Pointer(1, 0), *initQuadData.Pointer(1, 1)),
                                 Point<s16>(*initQuadData.Pointer(2, 0), *initQuadData.Pointer(2, 1)),
                                 Point<s16>(*initQuadData.Pointer(3, 0), *initQuadData.Pointer(3, 1)));
  
  AnkiConditionalErrorAndReturn(mxGetClassID(prhs[2]) == mxSINGLE_CLASS, "mexRefineQuadrilateral", "prhs[2] should be SINGLE");
  AnkiConditionalErrorAndReturn(mxGetM(prhs[2]) == 3 && mxGetN(prhs[2]) == 3, "mexRefineQuadrilateral", "prhs[2] should be 3x3 array");
  Array<f32> initialHomography = mxArrayToArray<f32>(prhs[argIndex++], scratch);
  AnkiConditionalErrorAndReturn(initialHomography.get_size(0) == 3 && initialHomography.get_size(1) == 3, "mexRefineQuadrilateral", "initial homography should be 3x3 array");
  
  const s32 iterations          = static_cast<s32>(mxGetScalar(prhs[argIndex++]));
  AnkiConditionalErrorAndReturn(mxGetNumberOfElements(prhs[argIndex])==1, "mexRefineQuadrilateral", "Separate squareThicknesses in x/y not supported yet. prhs[%d] must be a scalar.", argIndex);
  const f32 squareWidthFraction    = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
  const f32 darkGray               = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
  const f32 brightGray             = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
  const s32 numSamples             = static_cast<s32>(mxGetScalar(prhs[argIndex++]));
  const f32 maxCornerChange        = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
  const f32 minCornerChange        = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
  AnkiConditionalErrorAndReturn(mxGetNumberOfElements(prhs[argIndex])==1, "mexRefineQuadrilateral", "Separate roundedCornerFraction in x/y not supported yet. prhs[%d] must be a scalar.", argIndex);
  const f32 roundedCornersFraction = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
  
  Quadrilateral<f32> initialQuadF32;
  Quadrilateral<f32> refinedQuad;
  Array<f32> refinedHomography(3,3,scratch);

  initialQuadF32.SetCast<s16>(initialQuad);

  Anki::Result lastResult = RefineQuadrilateral(initialQuadF32, initialHomography, image, Point<f32>(squareWidthFraction, squareWidthFraction), Point<f32>(roundedCornersFraction,roundedCornersFraction), iterations, darkGray, brightGray, numSamples, maxCornerChange, minCornerChange, refinedQuad, refinedHomography, scratch);
  
  // Create outputs before the next error check, in case it fails (for example, if homography refinement failed)
  plhs[0] = mxCreateDoubleMatrix(4,2, mxREAL);
  plhs[1] = arrayToMxArray(refinedHomography);
  
  AnkiConditionalErrorAndReturn(lastResult == Anki::RESULT_OK, "mexRefineQuadrilateral", "RefineQuadrilateral failed.");

  double* refinedQuadData_x = mxGetPr(plhs[0]);
  double* refinedQuadData_y = refinedQuadData_x + 4;
  refinedQuadData_x[0] = refinedQuad[0].x;
  refinedQuadData_y[0] = refinedQuad[0].y;
  refinedQuadData_x[1] = refinedQuad[1].x;
  refinedQuadData_y[1] = refinedQuad[1].y;
  refinedQuadData_x[2] = refinedQuad[2].x;
  refinedQuadData_y[2] = refinedQuad[2].y;
  refinedQuadData_x[3] = refinedQuad[3].x;
  refinedQuadData_y[3] = refinedQuad[3].y;
  
  return;
}
