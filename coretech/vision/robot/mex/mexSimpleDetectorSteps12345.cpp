#include "mex.h"

#include "anki/common/robot/matlabInterface.h"

#include "anki/vision/robot/miscVisionKernels.h"


#include <string.h>
#include <vector>

#define VERBOSITY 0

using namespace Anki::Embedded;

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { printf("%s - %s\n", (eventName), (eventValue)); return;}

// image = drawExampleSquaresImage();
// imageSize = size(image);

// imageMarker = rgb2gray(imread('C:\Anki\blockImages\fiducial105_6ContrastReduced.png'));
// image = 255*ones(480,640,'uint8');
// image(101:(100+size(imageMarker,1)-2), 101:(100+size(imageMarker,2)-2)) = imageMarker(2:(end-1), 2:(end-1));
// imageSize = [480,640];

// scaleImage_numPyramidLevels = 6;
// component1d_minComponentWidth = 0;
// component1d_maxSkipDistance = 0;
// minSideLength = round(0.03*max(imageSize(1),imageSize(2)));
// maxSideLength = round(0.97*min(imageSize(1),imageSize(2)));
// component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
// component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
// component_sparseMultiplyThreshold = 1000.0;
// component_solidMultiplyThreshold = 2.0;
// component_percentHorizontal = 0.5;
// component_percentVertical = 0.5;
// quads_minQuadArea = 100;
// quads_quadSymmetryThreshold = 1.5;
// quads_minDistanceFromImageEdge = 2;
// decode_minContrastRatio = 1.25;
// [quads, blockTypes, faceTypes, orientations] = mexSimpleDetectorSteps12345(image, scaleImage_numPyramidLevels, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_percentHorizontal, component_percentVertical, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio);
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  ConditionalErrorAndReturn(nrhs == 14 && nlhs == 4, "mexSimpleDetectorSteps12345", "Call this function as following: [quads, blockTypes, faceTypes, orientations] = mexSimpleDetectorSteps1234(uint8(image), scaleImage_numPyramidLevels, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_percentHorizontal, component_percentVertical, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio);");

  Array<u8> image = mxArrayToArray<u8>(prhs[0]);
  const s32 scaleImage_numPyramidLevels = static_cast<s32>(mxGetScalar(prhs[1]));
  const s16 component1d_minComponentWidth = static_cast<s16>(mxGetScalar(prhs[2]));
  const s16 component1d_maxSkipDistance = static_cast<s16>(mxGetScalar(prhs[3]));
  const s32 component_minimumNumPixels = static_cast<s32>(mxGetScalar(prhs[4]));
  const s32 component_maximumNumPixels = static_cast<s32>(mxGetScalar(prhs[5]));
  const s32 component_sparseMultiplyThreshold = static_cast<s32>(Round(pow(2,5)*mxGetScalar(prhs[6]))); // Convert from double to SQ26.5
  const s32 component_solidMultiplyThreshold = static_cast<s32>(Round(pow(2,5)*mxGetScalar(prhs[7]))); // Convert from double to SQ26.5
  const s32 component_percentHorizontal = static_cast<s32>(Round(pow(2,8)*mxGetScalar(prhs[8]))); // Convert from double to SQ23.8
  const s32 component_percentVertical = static_cast<s32>(Round(pow(2,8)*mxGetScalar(prhs[9]))); // Convert from double to SQ23.8
  const s32 quads_minQuadArea = static_cast<s32>(mxGetScalar(prhs[10]));
  const s32 quads_quadSymmetryThreshold = static_cast<s32>(Round(pow(2,8)*mxGetScalar(prhs[11]))); // Convert from double to SQ23.8
  const s32 quads_minDistanceFromImageEdge = static_cast<s32>(mxGetScalar(prhs[12]));
  const f32 decode_minContrastRatio = static_cast<f32>(mxGetScalar(prhs[13]));

  ConditionalErrorAndReturn(image.IsValid(), "mexSimpleDetectorSteps12345", "Could not allocate image");

  const u32 numBytes0 = 10000000;
  MemoryStack scratch0(calloc(numBytes0,1), numBytes0);
  ConditionalErrorAndReturn(scratch0.IsValid(), "mexSimpleDetectorSteps12345", "Scratch0 could not be allocated");

  const u32 numBytes1 = 10000000;
  MemoryStack scratch1(calloc(numBytes1,1), numBytes0);
  ConditionalErrorAndReturn(scratch1.IsValid(), "mexSimpleDetectorSteps12345", "Scratch1 could not be allocated");

  const u32 numBytes2 = 10000000;
  MemoryStack scratch2(calloc(numBytes2,1), numBytes2);
  ConditionalErrorAndReturn(scratch2.IsValid(), "mexSimpleDetectorSteps12345", "Scratch2 could not be allocated");

  const s32 maxMarkers = 100;
  FixedLengthList<BlockMarker> markers(maxMarkers, scratch0);
  FixedLengthList<Array<f64>> homographies(maxMarkers, scratch0);

  markers.set_size(maxMarkers);
  homographies.set_size(maxMarkers);

  for(s32 i=0; i<maxMarkers; i++) {
    Array<f64> newArray(3, 3, scratch0);
    homographies[i] = newArray;
  }

  {
    const Result result = SimpleDetector_Steps12345(
      image,
      markers,
      homographies,
      scaleImage_numPyramidLevels,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      component_percentHorizontal, component_percentVertical,
      quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
      decode_minContrastRatio,
      scratch1,
      scratch2);

    ConditionalErrorAndReturn(result == RESULT_OK, "mexSimpleDetectorSteps1234", "SimpleDetector_Steps1234 Failed");
  }

  const s32 numMarkers = markers.get_size();

  std::vector<Array<f64> > quads;
  quads.resize(numMarkers);

  Array<f64> blockTypes(1, numMarkers, scratch0);
  Array<f64> faceTypes(1, numMarkers, scratch0);
  Array<f64> orientations(1, numMarkers, scratch0);

  for(s32 i=0; i<numMarkers; i++) {
    quads[i] = Array<f64>(4, 2, scratch0);

    for(s32 y=0; y<4; y++) {
      quads[i][y][0] = markers[i].corners[y].x;
      quads[i][y][1] = markers[i].corners[y].y;
    }

    blockTypes[0][i] = markers[i].blockType;
    faceTypes[0][i] = markers[i].faceType;

    if(markers[i].orientation == BlockMarker::ORIENTATION_UP) {
      orientations[0][i] = 0.0;
    } else if(markers[i].orientation == BlockMarker::ORIENTATION_DOWN) {
      orientations[0][i] = PI;
    } else if(markers[i].orientation == BlockMarker::ORIENTATION_LEFT) {
      orientations[0][i] = PI / 2.0;
    } else if(markers[i].orientation == BlockMarker::ORIENTATION_RIGHT) {
      orientations[0][i] = 3.0 * PI / 2.0;
    } else {
      orientations[0][i] = -100000.0; // Invalid
    }
  }

  const mwSize markersMatlab_ndim = 2;
  const mwSize markersMatlab_dims[] = {1, static_cast<mwSize>(numMarkers)};
  mxArray *quadsMatlab = mxCreateCellArray(markersMatlab_ndim, markersMatlab_dims);

  for(s32 i=0; i<numMarkers; i++) {
    mxSetCell(quadsMatlab, i, arrayToMxArray<f64>(quads[i]));
  }

  mxArray *blockTypesMatlab = arrayToMxArray<f64>(blockTypes);
  mxArray *faceTypesMatlab = arrayToMxArray<f64>(faceTypes);
  mxArray *orientationsMatlab = arrayToMxArray<f64>(orientations);

  plhs[0] = quadsMatlab;
  plhs[1] = blockTypesMatlab;
  plhs[2] = faceTypesMatlab;
  plhs[3] = orientationsMatlab;

  free(scratch0.get_buffer());
  free(scratch1.get_buffer());
  free(scratch2.get_buffer());
}