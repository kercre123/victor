#include "mex.h"

#include "anki/common/robot/matlabInterface.h"

#include "anki/vision/robot/miscVisionKernels.h"

#include <string.h>
#include <vector>

#define VERBOSITY 0

using namespace Anki::Embedded;

// image = drawExampleSquaresImage();
// scaleImage_useWhichAlgorithm = 1;
// scaleImage_numPyramidLevels = 4;
// scaleImage_thresholdMultiplier = 0.75;
// component1d_minComponentWidth = 0;
// component1d_maxSkipDistance = 0;
// minSideLength = round(0.03*max(size(image,1),size(image,2)));
// maxSideLength = round(0.9*min(size(image,1),size(image,2)));
// component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
// component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
// component_sparseMultiplyThreshold = 1000.0;
// component_solidMultiplyThreshold = 2.0;
// components2d = mexSimpleDetectorSteps123(image, scaleImage_numPyramidLevels, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold);
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  //     Result SimpleDetector_Steps123(
  //       const Array<u8> &image,
  //       const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier,
  //       const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
  //       const s32 component_minimumNumPixels, const s32 component_maximumNumPixels,
  //       const s32 component_sparseMultiplyThreshold, const s32 component_solidMultiplyThreshold,
  //       FixedLengthList<ConnectedComponentSegment> &extractedComponents,
  //       MemoryStack scratch1,
  //       MemoryStack scratch2)

  AnkiConditionalErrorAndReturn(nrhs == 10 && nlhs == 1, "mexSimpleDetectorSteps123", "Call this function as following: components2d = mexSimpleDetectorSteps123(uint8(image), scaleImage_useWhichAlgorithm, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold);");

  Array<u8> image = mxArrayToArray<u8>(prhs[0]);
  const CharacteristicScaleAlgorithm scaleImage_useWhichAlgorithm = static_cast<CharacteristicScaleAlgorithm>(static_cast<s32>(mxGetScalar(prhs[1])));
  const s32 scaleImage_numPyramidLevels = static_cast<s32>(mxGetScalar(prhs[2]));
  const s32 scaleImage_thresholdMultiplier = static_cast<s32>(Round(pow(2,16)*mxGetScalar(prhs[3]))); // Convert from double to SQ15.16
  const s16 component1d_minComponentWidth = static_cast<s16>(mxGetScalar(prhs[4]));
  const s16 component1d_maxSkipDistance = static_cast<s16>(mxGetScalar(prhs[5]));
  const s32 component_minimumNumPixels = static_cast<s32>(mxGetScalar(prhs[6]));
  const s32 component_maximumNumPixels = static_cast<s32>(mxGetScalar(prhs[7]));
  const s32 component_sparseMultiplyThreshold = static_cast<s32>(Round(pow(2,5)*mxGetScalar(prhs[8]))); // Convert from double to SQ26.5
  const s32 component_solidMultiplyThreshold = static_cast<s32>(Round(pow(2,5)*mxGetScalar(prhs[9]))); // Convert from double to SQ26.5

  AnkiConditionalErrorAndReturn(image.IsValid(), "mexSimpleDetectorSteps123", "Could not allocate image");

  const u32 numBytes0 = 10000000;
  MemoryStack scratch0(calloc(numBytes0,1), numBytes0);
  AnkiConditionalErrorAndReturn(scratch0.IsValid(), "mexSimpleDetectorSteps123", "Scratch0 could not be allocated");

  const u32 numBytes1 = 10000000;
  MemoryStack scratch1(calloc(numBytes1,1), numBytes0);
  AnkiConditionalErrorAndReturn(scratch1.IsValid(), "mexSimpleDetectorSteps123", "Scratch1 could not be allocated");

  const u32 numBytes2 = 10000000;
  MemoryStack scratch2(calloc(numBytes2,1), numBytes2);
  AnkiConditionalErrorAndReturn(scratch2.IsValid(), "mexSimpleDetectorSteps123", "Scratch2 could not be allocated");

  const s32 maxConnectedComponentSegments = u16_MAX;
  ConnectedComponents extractedComponents(maxConnectedComponentSegments, image.get_size(1), scratch0);

  {
    const Result result = SimpleDetector_Steps123(
      image,
      scaleImage_useWhichAlgorithm,
      scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      extractedComponents,
      scratch1,
      scratch2);

    AnkiConditionalErrorAndReturn(result == RESULT_OK, "mexSimpleDetectorSteps123", "SimpleDetector_Steps123 Failed");
  }

  const u16 numComponents = extractedComponents.get_maximumId();

  FixedLengthList<s32> numComponentSegments(numComponents+1, scratch0);
  {
    const Result result = extractedComponents.ComputeNumComponentSegmentsForEachId(numComponentSegments);

    AnkiConditionalErrorAndReturn(result == RESULT_OK, "mexSimpleDetectorSteps123", "ComputeComponentSizes Failed");
  }

  std::vector<Array<f64> > components2d;
  components2d.resize(numComponents);

  s32 *components2d_currentSegment = reinterpret_cast<s32*>(scratch0.Allocate(numComponents*sizeof(s32)));
  memset(components2d_currentSegment, 0, sizeof(components2d_currentSegment[0]) * numComponents);

  for(s32 i=0; i<numComponents; i++) {
    components2d[i] = Array<f64>(numComponentSegments[i+1], 3, scratch0);
  }

  const ConnectedComponentSegment * restrict pConstExtractedComponents = extractedComponents.Pointer(0);
  for(s32 i=0; i<extractedComponents.get_size(); i++) {
    const u16 id = pConstExtractedComponents[i].id;

    if(id == 0)
      continue;

    f64 * restrict pComponents2di = components2d[id-1].Pointer(components2d_currentSegment[id-1],0);

    pComponents2di[0] = pConstExtractedComponents[i].y;
    pComponents2di[1] = pConstExtractedComponents[i].xStart;
    pComponents2di[2] = pConstExtractedComponents[i].xEnd;

    components2d_currentSegment[id-1]++;
  }

  const mwSize components2dMatlab_ndim = 2;
  const mwSize components2dMatlab_dims[] = {1, numComponents};
  mxArray *components2dMatlab = mxCreateCellArray(components2dMatlab_ndim, components2dMatlab_dims);

  for(s32 i=0; i<numComponents; i++) {
    mxSetCell(components2dMatlab, i, arrayToMxArray<f64>(components2d[i]));
  }

  plhs[0] = components2dMatlab;

  free(scratch0.get_buffer());
  free(scratch1.get_buffer());
  free(scratch2.get_buffer());
}