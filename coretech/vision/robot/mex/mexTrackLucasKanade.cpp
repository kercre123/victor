#include "mex.h"

#include <string.h>

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/fixedLengthList.h"

#include "anki/vision/robot/lucasKanade.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

// templateImage = uint8(rgb2gray(imresize(imread('C:\Anki\blockImages\blockImages00315.png'), [480,640]/8)));
// nextImage = uint8(rgb2gray(imresize(imread('C:\Anki\blockImages\blockImages00316.png'), [480,640]/8)));
// templateRegionRectangle = [29,39,11,21];
// numPyramidLevels = 2;
// transformType = 1;
// ridgeWeight = 1e-3;
// maxIterations = 50;
// convergenceTolerance = 0.05;
// homography = eye(3,3);
// newHomography = mexTrackLucasKanade(uint8(templateImage), double(templateRegionRectangle), double(numPyramidLevels), double(transformType), double(ridgeWeight), uint8(nextImage), double(maxIterations), double(convergenceTolerance), double(homography))

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])

{
  AnkiConditionalErrorAndReturn(nrhs == 9 && nlhs == 1, "mexTrackLucasKanade", "Call this function as following: newHomography = mexTrackLucasKanade(uint8(templateImage), double(templateRegionRectangle), double(numPyramidLevels), double(transformType), double(ridgeWeight), uint8(nextImage), double(maxIterations), double(convergenceTolerance), double(homography));");

  Array<u8> templateImage = mxArrayToArray<u8>(prhs[0]);
  Array<f64> templateRegionRectangle_array = mxArrayToArray<f64>(prhs[1]);
  const s32 numPyramidLevels = static_cast<s32>(mxGetScalar(prhs[2]));
  const TemplateTracker::TransformType transformType = static_cast<TemplateTracker::TransformType>(static_cast<s32>(mxGetScalar(prhs[3])));
  const f32 ridgeWeight = static_cast<f32>(mxGetScalar(prhs[4]));
  Array<u8> nextImage = mxArrayToArray<u8>(prhs[5]);
  const s32 maxIterations = static_cast<s32>(mxGetScalar(prhs[6]));
  const f32 convergenceTolerance = static_cast<f32>(mxGetScalar(prhs[7]));
  Array<f64> homography_f64 = mxArrayToArray<f64>(prhs[8]);

  AnkiConditionalErrorAndReturn(templateImage.IsValid(), "mexTrackLucasKanade", "Could not allocate templateImage");
  AnkiConditionalErrorAndReturn(templateRegionRectangle_array.IsValid(), "mexTrackLucasKanade", "Could not allocate templateRegionRectangle_array");
  AnkiConditionalErrorAndReturn(nextImage.IsValid(), "mexTrackLucasKanade", "Could not allocate nextImage");
  AnkiConditionalErrorAndReturn(homography_f64.IsValid(), "mexTrackLucasKanade", "Could not allocate homography_f64");

  const u32 numBytes0 = 10000000;
  MemoryStack scratch0(calloc(numBytes0,1), numBytes0);
  AnkiConditionalErrorAndReturn(scratch0.IsValid(), "mexTrackLucasKanade", "Scratch0 could not be allocated");

  TemplateTracker::LucasKanadeTracker_f32 tracker(templateImage.get_size(0), templateImage.get_size(1), numPyramidLevels, transformType, ridgeWeight, scratch0);

  AnkiConditionalErrorAndReturn(tracker.IsValid(), "mexTrackLucasKanade", "Could not construct tracker");

  Rectangle<f32> templateRegion(
    static_cast<f32>(templateRegionRectangle_array[0][0]),
    static_cast<f32>(templateRegionRectangle_array[0][1]),
    static_cast<f32>(templateRegionRectangle_array[0][2]),
    static_cast<f32>(templateRegionRectangle_array[0][3]));

  if(tracker.InitializeTemplate(templateImage, templateRegion, scratch0) != RESULT_OK) {
    printf("Error: mexTrackLucasKanade.InitializeTemplate\n");
    return;
  }

  TemplateTracker::PlanarTransformation_f32 initialTransform(transformType, scratch0);

  Array<f32> homography(3, 3, scratch0);
  homography.SetCast<f64>(homography_f64);

  initialTransform.set_homography(homography);

  if(tracker.set_transformation(initialTransform) != RESULT_OK) {
    printf("Error: mexTrackLucasKanade.set_transformation\n");
    return;
  }

  if(tracker.UpdateTrack(nextImage, maxIterations, convergenceTolerance, scratch0) != RESULT_OK) {
    printf("Error: mexTrackLucasKanade.UpdateTrack\n");
    return;
  }

  const Array<f32> &newHomography = tracker.get_transformation().get_homography();

  Array<f64> newHomography_f64(3, 3, scratch0);
  newHomography_f64.SetCast<f32>(newHomography);

  plhs[0] = arrayToMxArray<f64>(newHomography_f64);

  free(templateImage.get_rawDataPointer());
  free(templateRegionRectangle_array.get_rawDataPointer());
  free(nextImage.get_rawDataPointer());
  free(homography_f64.get_rawDataPointer());
  free(scratch0.get_buffer());
}