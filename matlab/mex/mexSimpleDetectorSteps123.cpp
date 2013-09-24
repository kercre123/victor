#include "mex.h"

#include "anki/embeddedCommon.h"
#include "anki/embeddedVision.h"

#include <string.h>
#include <vector>

#define VERBOSITY 0

using namespace Anki::Embedded;

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { printf("%s - %s\n", (eventName), (eventValue)); return;}

// image = drawExampleSquaresImage();
// scaleImage_numPyramidLevels = 6;
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
  // components2d = simpleDetector_step3_simpleRejectionTests(nrows, ncols, numRegions, area, indexList, bb, centroid, usePerimeterCheck, components2d, embeddedConversions, DEBUG_DISPLAY)

  //     IN_DDR Result SimpleDetector_Steps123(
  //       const Array<u8> &image,
  //       const s32 scaleImage_numPyramidLevels,
  //       const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
  //       const s32 component_minimumNumPixels, const s32 component_maximumNumPixels,
  //       const s32 component_sparseMultiplyThreshold, const s32 component_solidMultiplyThreshold,
  //       FixedLengthList<ConnectedComponentSegment> &extractedComponents,
  //       MemoryStack scratch1,
  //       MemoryStack scratch2)

  ConditionalErrorAndReturn(nrhs == 8 && nlhs == 1, "mexSimpleDetectorSteps123", "Call this function as following: components2d = mexSimpleDetectorSteps123(uint8(image), scaleImage_numPyramidLevels, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold);");

  Array<u8> image = mxArrayToArray<u8>(prhs[0]);
  const s32 scaleImage_numPyramidLevels = static_cast<s32>(mxGetScalar(prhs[1]));
  const s16 component1d_minComponentWidth = static_cast<s16>(mxGetScalar(prhs[2]));
  const s16 component1d_maxSkipDistance = static_cast<s16>(mxGetScalar(prhs[3]));
  const s32 component_minimumNumPixels = static_cast<s32>(mxGetScalar(prhs[4]));
  const s32 component_maximumNumPixels = static_cast<s32>(mxGetScalar(prhs[5]));
  const s32 component_sparseMultiplyThreshold = static_cast<s32>(Round(pow(2,5)*mxGetScalar(prhs[6]))); // Convert from double to SQ26.5
  const s32 component_solidMultiplyThreshold = static_cast<s32>(Round(pow(2,5)*mxGetScalar(prhs[7]))); // Convert from double to SQ26.5

  //printf("%f %f %s\n", *startPoint.Pointer(0,0), *startPoint.Pointer(0,1), initialDirection.data());
  ConditionalErrorAndReturn(image.IsValid(), "mexSimpleDetectorSteps123", "Could not allocate Matrix binaryImg");

  const u32 numBytes0 = 10000000;
  MemoryStack scratch0(calloc(numBytes0,1), numBytes0);
  ConditionalErrorAndReturn(scratch0.IsValid(), "mexSimpleDetectorSteps123", "Scratch0 could not be allocated");

  const u32 numBytes1 = 10000000;
  MemoryStack scratch1(calloc(numBytes1,1), numBytes0);
  ConditionalErrorAndReturn(scratch1.IsValid(), "mexSimpleDetectorSteps123", "Scratch1 could not be allocated");

  const u32 numBytes2 = 10000000;
  MemoryStack scratch2(calloc(numBytes2,1), numBytes2);
  ConditionalErrorAndReturn(scratch2.IsValid(), "mexSimpleDetectorSteps123", "Scratch2 could not be allocated");

  const s32 maxConnectedComponentSegments = u16_MAX;
  ConnectedComponents extractedComponents(maxConnectedComponentSegments, scratch0);

  {
    const Result result = SimpleDetector_Steps123(
      image,
      scaleImage_numPyramidLevels,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      extractedComponents,
      scratch1,
      scratch2);

    ConditionalErrorAndReturn(result == RESULT_OK, "mexSimpleDetectorSteps123", "SimpleDetector_Steps123 Failed");
  }

  const u16 numComponents = extractedComponents.get_maximumId();

  s32 * const numComponentSegments = reinterpret_cast<s32*>(scratch0.Allocate(sizeof(s32)*(numComponents+1)));

  {
    const Result result = extractedComponents.ComputeNumComponentSegmentsForEachId(numComponentSegments);

    ConditionalErrorAndReturn(result == RESULT_OK, "mexSimpleDetectorSteps123", "ComputeComponentSizes Failed");
  }

  std::vector<Array<f64> > components2d;
  components2d.resize(numComponents);
  //* = new Array<f64>[]; //reinterpret_cast<Array<f64>*>(scratch0.Allocate(numComponents*sizeof(Array<f64>)));

  s32 *components2d_currentSegment = reinterpret_cast<s32*>(scratch0.Allocate(numComponents*sizeof(s32)));
  memset(components2d_currentSegment, 0, sizeof(components2d_currentSegment[0]) * numComponents);

  for(s32 i=0; i<numComponents; i++) {
    components2d[i] = Array<f64>(numComponentSegments[i+1], 3, scratch0);
  }

  const ConnectedComponentSegment * restrict extractedComponents_constRowPointer = extractedComponents.Pointer(0);
  for(s32 i=0; i<extractedComponents.get_size(); i++) {
    const u16 id = extractedComponents_constRowPointer[i].id;

    if(id == 0)
      continue;

    f64 * restrict components2di_rowPointer = components2d[id-1].Pointer(components2d_currentSegment[id-1],0);

    components2di_rowPointer[0] = extractedComponents_constRowPointer[i].y;
    components2di_rowPointer[1] = extractedComponents_constRowPointer[i].xStart;
    components2di_rowPointer[2] = extractedComponents_constRowPointer[i].xEnd;

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
  //delete(components2d);
}