// TODO: update this mex file to the new API
#if 0

#include "mex.h"

#include "anki/common/robot/matlabInterface.h"

#include "coretech/vision/robot/lucasKanade.h"
#include "coretech/vision/robot/fiducialMarkers.h"
#include "coretech/vision/robot/fiducialDetection.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

#include <string.h>
#include <vector>

#define VERBOSITY 0

using namespace Anki::Embedded;

// decode_minContrastRatio = 1.25;
// refine_quadRefinementIterations = 5;
// refine_numRefinementSamples = 100;
// returnInvalidMarkers = 0;
// refine_quadRefinementMaxCornerChange = 2;
// refine_quadRefinementMinCornerChange = 0.005;
// [quads, markerTypes, markerNames, markerValidity] = mexDetectFiducialMarkers_quadInput(image, quadsCellArray, decode_minContrastRatio, refine_quadRefinementIterations, refine_numRefinementSamples, refine_quadRefinementMaxCornerChange, refine_quadRefinementMinCornerChange, returnInvalidMarkers);
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  const s32 bufferSize = 10000000;

  AnkiConditionalErrorAndReturn(nrhs == 11 && nlhs >= 2 && nlhs <= 4, "mexDetectFiducialMarkers_quadInput", "Call this function as following: [quads, markerTypes, <markerNames>, <markerValidity>] = mexDetectFiducialMarkers_quadInput(uint8(image), quadsCellArray, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, refine_quadRefinementIterations, refine_numRefinementSamples, refine_quadRefinementMaxCornerChange, refine_quadRefinementMinCornerChange, returnInvalidMarkers);");

  MemoryStack memory(mxMalloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(memory.IsValid(), "mexDetectFiducialMarkers_quadInput", "Memory could not be allocated");

  Array<u8> image                                = mxArrayToArray<u8>(prhs[0], memory);
  Array<Array<f64> > quadsF64                    = mxCellArrayToArray<f64>(prhs[1], memory);
  const s32 quads_minQuadArea                    = static_cast<s32>(mxGetScalar(prhs[2]));
  const s32 quads_quadSymmetryThreshold          = Round<s32>(pow(2,8)*mxGetScalar(prhs[3])); // Convert from double to SQ23.8
  const s32 quads_minDistanceFromImageEdge       = static_cast<s32>(mxGetScalar(prhs[4]));
  const f32 decode_minContrastRatio              = static_cast<f32>(mxGetScalar(prhs[5]));
  const s32 refine_quadRefinementIterations      = static_cast<s32>(mxGetScalar(prhs[6]));
  const s32 refine_numRefinementSamples          = static_cast<s32>(mxGetScalar(prhs[7]));
  const f32 refine_quadRefinementMaxCornerChange = static_cast<f32>(mxGetScalar(prhs[8]));
  const f32 refine_quadRefinementMinCornerChange = static_cast<f32>(mxGetScalar(prhs[9]));
  const bool returnInvalidMarkers                = static_cast<bool>(Round<s32>(mxGetScalar(prhs[10])));

  AnkiConditionalErrorAndReturn(image.IsValid(), "mexDetectFiducialMarkers_quadInput", "Could not allocate image");

  AnkiConditionalErrorAndReturn(quadsF64.get_size(0) == 1 || quadsF64.get_size(1) == 1,  "mexDetectFiducialMarkers_quadInput", "Quads must be 1 dimensional");

  const s32 numQuads = MAX(quadsF64.get_size(0), quadsF64.get_size(1));

  FixedLengthList<Quadrilateral<s16> > quadsS16 = FixedLengthList<Quadrilateral<s16> >(numQuads, memory, Flags::Buffer(true, false, true));

  for(s32 iQuad=0; iQuad<numQuads; iQuad++) {
    const Array<f64> *curQuad;

    if(quadsF64.get_size(0) == 1) {
      curQuad = &quadsF64[0][iQuad];
    } else {
      curQuad = &quadsF64[iQuad][0];
    }

    for(s32 iCorner=0; iCorner<4; iCorner++) {
      quadsS16[iQuad].corners[iCorner].x = saturate_cast<s16>((*curQuad)[iCorner][0]);
      quadsS16[iQuad].corners[iCorner].y = saturate_cast<s16>((*curQuad)[iCorner][1]);
    }
  }

  FixedLengthList<VisionMarker> markers(numQuads+1, memory);
  FixedLengthList<Array<f32>> homographies(numQuads+1, memory);

  markers.set_size(numQuads);
  homographies.set_size(numQuads);

  for(s32 i=0; i<numQuads; i++) {
    Array<f32> newArray(3, 3, memory);
    homographies[i] = newArray;
  }

  // DetectFiducialMarkers step 4b and 5: Decode fiducial markers from the candidate quadrilaterals

  markers.set_size(quadsS16.get_size());

  Anki::Result lastResult;

  // refinedHomography and meanGrayvalueThreshold are computed by currentMarker.RefineCorners(), then used by currentMarker.Extract()
  Array<f32> refinedHomography(3, 3, memory);
  u8 meanGrayvalueThreshold;
  
  for(s32 iQuad=0; iQuad<quadsS16.get_size(); iQuad++) {
    Array<f32> &currentHomography = homographies[iQuad];
    const Quadrilateral<s16> &currentQuad = quadsS16[iQuad];
    VisionMarker &currentMarker = markers[iQuad];

    bool areCornersDisordered;
    const bool isReasonable = IsQuadrilateralReasonable(currentQuad, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, image.get_size(0), image.get_size(1), areCornersDisordered);

    if(!isReasonable) {
      currentMarker.validity = VisionMarker::WEIRD_SHAPE;
      continue;
    }

    bool numericalFailure;
    if((lastResult = Transformations::ComputeHomographyFromQuad(currentQuad, currentHomography, numericalFailure, memory)) != Anki::RESULT_OK) {
      return;
    }
    
    currentMarker = VisionMarker(currentQuad, VisionMarker::UNKNOWN);

    if(numericalFailure) {
      currentMarker.validity = VisionMarker::NUMERICAL_FAILURE;
      continue;
    }
    
    if((lastResult = currentMarker.RefineCorners(image, currentHomography, decode_minContrastRatio,
                                                 refine_quadRefinementIterations, refine_numRefinementSamples,
                                                 refine_quadRefinementMaxCornerChange, refine_quadRefinementMinCornerChange,
                                                 quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
                                                 refinedHomography, meanGrayvalueThreshold, //< Computed for Extract()
                                                 memory)) != Anki::RESULT_OK)
    {
      return;
    }

    if(currentMarker.validity == VisionMarker::LOW_CONTRAST) {
      currentMarker.markerType = Anki::Vision::MARKER_UNKNOWN;
    } else {
      if((lastResult = currentMarker.Extract(image,
                                             refinedHomography,
                                             meanGrayvalueThreshold, //< Computed by RefineCorners()
                                             decode_minContrastRatio,
                                             memory)) != Anki::RESULT_OK)
      {
        return;
      }
    }
  } // for each quad

  // Remove invalid markers from the list
  if(!returnInvalidMarkers) {
    for(s32 iQuad=0; iQuad<quadsS16.get_size(); iQuad++) {
      if(markers[iQuad].validity != VisionMarker::VALID) {
        for(s32 jQuad=iQuad; jQuad<quadsS16.get_size(); jQuad++) {
          markers[jQuad] = markers[jQuad+1];
          homographies[jQuad] = homographies[jQuad+1];
        }
        quadsS16.set_size(quadsS16.get_size()-1);
        markers.set_size(markers.get_size()-1);
        homographies.set_size(homographies.get_size()-1);
        iQuad--;
      }
    }
  }

  const s32 numMarkers = markers.get_size();

  if(numMarkers != 0) {
    std::vector<Array<f64> > quads;
    quads.resize(numMarkers);

    Array<f64> markerTypes(1, numMarkers, memory);
    Array<f64> orientations(1, numMarkers, memory);

    for(s32 i=0; i<numMarkers; i++) {
      quads[i] = Array<f64>(4, 2, memory);

      for(s32 y=0; y<4; y++) {
        quads[i][y][0] = markers[i].corners[y].x;
        quads[i][y][1] = markers[i].corners[y].y;
      }

      markerTypes[0][i] = markers[i].markerType;
    }

    const mwSize markersMatlab_ndim = 2;
    const mwSize markersMatlab_dims[] = {1, static_cast<mwSize>(numMarkers)};
    mxArray *quadsMatlab = mxCreateCellArray(markersMatlab_ndim, markersMatlab_dims);

    for(s32 i=0; i<numMarkers; i++) {
      mxSetCell(quadsMatlab, i, arrayToMxArray<f64>(quads[i]));
    }

    mxArray *markerTypesMatlab = arrayToMxArray<f64>(markerTypes);

    plhs[0] = quadsMatlab;
    plhs[1] = markerTypesMatlab;

    if(nlhs >= 3) {
      mxArray* markerNamesMatlab = mxCreateCellArray(markersMatlab_ndim, markersMatlab_dims);
      for(s32 i=0; i<numMarkers; ++i) {
        AnkiAssert(markers[i].markerType >= 0 && markers[i].markerType <= Anki::Vision::NUM_MARKER_TYPES);
        mxSetCell(markerNamesMatlab, i, mxCreateString(Anki::Vision::MarkerTypeStrings[markers[i].markerType]));
      }
      plhs[2] = markerNamesMatlab;
    }

    if(nlhs >= 4) {
      Array<s32> markerValidity(1, numMarkers, memory);

      for(s32 i=0; i<numMarkers; ++i) {
        markerValidity[0][i] = markers[i].validity;
      }

      plhs[3] = arrayToMxArray<s32>(markerValidity);
    }
  } else { // if(numMarkers != 0)
    const mwSize markersMatlab_ndim = 2;
    const mwSize markersMatlab_dims[] = {0, 0};
    mxArray *quadsMatlab = mxCreateCellArray(markersMatlab_ndim, markersMatlab_dims);
    mxArray *markerTypesMatlab = mxCreateNumericArray(2, markersMatlab_dims, mxDOUBLE_CLASS, mxREAL);

    plhs[0] = quadsMatlab;
    plhs[1] = markerTypesMatlab;

    if(nlhs >= 3) {
      plhs[2] = mxCreateCellArray(markersMatlab_ndim, markersMatlab_dims);
    }

    if(nlhs >= 4) {
      plhs[3] = mxCreateNumericArray(2, markersMatlab_dims, mxINT32_CLASS, mxREAL);
    }
  } // if(numMarkers != 0) ... else

  mxFree(memory.get_buffer());
}

#endif // TODO: update this mex file to the new API
