#include "mex.h"

#include <string.h>

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/fixedLengthList.h"

#include "coretech/vision/robot/lucasKanade.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

// templateImage = uint8(rgb2gray(imresize(imread('C:\Anki\blockImages\blockImages00315.png'), [480,640]/8)));
// nextImage = uint8(rgb2gray(imresize(imread('C:\Anki\blockImages\blockImages00316.png'), [480,640]/8)));
// templateRegionRectangle = [29,39,11,21]; % [left, right, top, bottom]
// numPyramidLevels = 2;
// transformType = bitshift(2,8); // translation
// ridgeWeight = 1e-3;
// maxIterations = 50;
// convergenceTolerance = 0.05;
// homography = eye(3,3);
// newHomography = mexTrackLucasKanade(uint8(templateImage), double(templateRegionRectangle), double(numPyramidLevels), double(transformType), double(ridgeWeight), uint8(nextImage), double(maxIterations), double(convergenceTolerance), double(homography))

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  AnkiConditionalErrorAndReturn(nrhs == 9 && nlhs == 1, "mexTrackLucasKanade", "Call this function as following: newHomography = mexTrackLucasKanade(uint8(templateImage), double(templateRegionRectangle), double(numPyramidLevels), double(transformType), double(ridgeWeight), uint8(nextImage), double(maxIterations), double(convergenceTolerance), double(homography));");

  const s32 bufferSize = 10000000;
  MemoryStack memory(malloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(memory.IsValid(), "mexTrackLucasKanade", "Memory could not be allocated");

  Array<u8> templateImage = mxArrayToArray<u8>(prhs[0], memory);
  Array<f64> templateRegionRectangle_array = mxArrayToArray<f64>(prhs[1], memory);
  const s32 numPyramidLevels = static_cast<s32>(mxGetScalar(prhs[2]));
  const Transformations::TransformType transformType = static_cast<Transformations::TransformType>(static_cast<s32>(mxGetScalar(prhs[3])));
  const f32 ridgeWeight = static_cast<f32>(mxGetScalar(prhs[4]));
  Array<u8> nextImage = mxArrayToArray<u8>(prhs[5], memory);
  const s32 maxIterations = static_cast<s32>(mxGetScalar(prhs[6]));
  const f32 convergenceTolerance = static_cast<f32>(mxGetScalar(prhs[7]));
  Array<f64> homography_f64 = mxArrayToArray<f64>(prhs[8], memory);

  AnkiConditionalErrorAndReturn(templateImage.IsValid(), "mexTrackLucasKanade", "Could not allocate templateImage");
  AnkiConditionalErrorAndReturn(templateRegionRectangle_array.IsValid(), "mexTrackLucasKanade", "Could not allocate templateRegionRectangle_array");
  AnkiConditionalErrorAndReturn(nextImage.IsValid(), "mexTrackLucasKanade", "Could not allocate nextImage");
  AnkiConditionalErrorAndReturn(homography_f64.IsValid(), "mexTrackLucasKanade", "Could not allocate homography_f64");

  //templateImage.Show("templateImage", false);
  //nextImage.Show("nextImage", true);

  Rectangle<f32> templateRegion(
    static_cast<f32>(templateRegionRectangle_array[0][0]),
    static_cast<f32>(templateRegionRectangle_array[0][1]),
    static_cast<f32>(templateRegionRectangle_array[0][2]),
    static_cast<f32>(templateRegionRectangle_array[0][3]));

  TemplateTracker::LucasKanadeTracker_Slow tracker(templateImage, templateRegion, 1.0f, numPyramidLevels, transformType, ridgeWeight, memory);

  AnkiConditionalErrorAndReturn(tracker.IsValid(), "mexTrackLucasKanade", "Could not construct tracker");

  Transformations::PlanarTransformation_f32 initialTransform(transformType, memory);

  Array<f32> homography(3, 3, memory);
  homography.SetCast<f64>(homography_f64);

  initialTransform.set_homography(homography);
  initialTransform.set_centerOffset(tracker.get_transformation().get_centerOffset(1.0f));

  if(tracker.set_transformation(initialTransform) != Anki::RESULT_OK) {
    AnkiError("mexTrackLucasKanade", "set_transformation");
    return;
  }

  bool converged = false;
  if(tracker.UpdateTrack(nextImage, maxIterations, convergenceTolerance, true, converged, memory) != Anki::RESULT_OK) {
    AnkiError("mexTrackLucasKanade", "UpdateTrack");
    return;
  }

  // TODO: Return convergence flag?

  const Array<f32> &newHomography = tracker.get_transformation().get_homography();

  Array<f64> newHomography_f64(3, 3, memory);
  newHomography_f64.SetCast<f32>(newHomography);

  plhs[0] = arrayToMxArray<f64>(newHomography_f64);

  free(memory.get_buffer());
}
