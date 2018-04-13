/**
 * File: markerDetector.cpp
 *
 * Author: Andrew Stein
 * Date:   08/22/2017
 *
 * Description: Detector for Vision Markers, which wraps the old Embedded implementation with an
 *              engine/basestation-friendly API.
 *
 * Copyright: Anki, Inc. 2017
 **/


#include "coretech/vision/engine/markerDetector.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/robot/fiducialDetection.h"
#include "coretech/vision/robot/fiducialMarkers.h"

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/rect_impl.h"

#include "coretech/common/robot/array2d.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"

#include <list>

#define USE_MATLAB_DETECTOR 0

namespace Anki {
namespace Vision {

// TODO: Default to false (and remove?) for VIC-945, when we've fully switched to dark cubes/charger
CONSOLE_VAR(bool, kMarkerDetector_DarkOnLight, "Vision.MarkerDetection",
#ifdef ANDROID
  true   // Continue to use dark-on-light markers with real robots until we have DVT3 cubes/chargers
#else
  false  // Go ahead and start using light-on-dark markers in simulation
#endif
);
  
struct MarkerDetector::Parameters : public Embedded::FiducialDetectionParameters
{
  s32         maxMarkers;
  f32         component_numPixelsFraction;
  f32         minSideLengthFraction;
  f32         maxSideLengthFraction;
  bool        isInitialized;
  
  // selected when (kMarkerDetector_DarkOnLight==false)
  s32 scaleImage_thresholdMultiplier_lightOnDark = static_cast<s32>(65536.f * 0.9f);
  
  Parameters();
  void Initialize(); // TODO: Initialize from Json config
  void SetComputeComponentMinNumPixels(s32 numRows, s32 numCols);
  void SetComputeComponentMaxNumPixels(s32 numRows, s32 numCols);
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// This horrible mess is leftover from the original embedded implementation where we could
// not allocate memory. "Off Chip", "On Chip", and "CCM" were different blocks of memory with different
// speeds and max sizes. Someday it would be nice to remove all that...
class MarkerDetector::Memory
{
  std::vector<u8> _offchipBuffer;
  std::vector<u8> _onchipBuffer;
  std::vector<u8> _ccmBuffer;
  
public:
  Embedded::MemoryStack _offchipScratch;
  Embedded::MemoryStack _onchipScratch;
  Embedded::MemoryStack _ccmScratch;
  
  // This still relies on Embedded data structures so is part of "Memory" instead of MarkerDetector class for now
  Embedded::FixedLengthList<Embedded::VisionMarker> _markers;
  
  Result ResetBuffers(s32 numRows, s32 numCols, s32 maxMarkers);
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result MarkerDetector::Memory::ResetBuffers(s32 numRows, s32 numCols, s32 maxMarkers)
{
  // TODO: Compute buffer sizes from number of pixels
  //  const s32 numPixels = numRows*numCols;
  //  _offchipBuffer.resize(OFFCHIP_BUFFER_MULTIPLIER * numPixels);
  //  _onchipBuffer.resize(ONCHIP_BUFFER_MULTIPLIER * numPixels);
  //  _ccmBuffer.resize(CCM_BUFFER_MULTIPLIER * numPixels);

  static const s32 OFFCHIP_BUFFER_SIZE = 4000000;
  static const s32 ONCHIP_BUFFER_SIZE  = 1600000;
  static const s32 CCM_BUFFER_SIZE     = 200000;

  _offchipBuffer.resize(OFFCHIP_BUFFER_SIZE);
  _onchipBuffer.resize(ONCHIP_BUFFER_SIZE);
  _ccmBuffer.resize(CCM_BUFFER_SIZE);
  
  _offchipScratch = Embedded::MemoryStack(_offchipBuffer.data(), Util::numeric_cast<s32>(_offchipBuffer.size()));
  _onchipScratch  = Embedded::MemoryStack(_onchipBuffer.data(),  Util::numeric_cast<s32>(_onchipBuffer.size()));
  _ccmScratch     = Embedded::MemoryStack(_ccmBuffer.data(),     Util::numeric_cast<s32>(_ccmBuffer.size()));
  
  if(!_offchipScratch.IsValid() || !_onchipScratch.IsValid() || !_ccmScratch.IsValid()) {
    PRINT_NAMED_ERROR("MarkerDetector.Memory.ResetBuffers.Failure", "Failed to initialize scratch buffers");
    return RESULT_FAIL_MEMORY;
  }
  
  _markers = Embedded::FixedLengthList<Embedded::VisionMarker>(maxMarkers, _offchipScratch);
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MarkerDetector::IsDarkOnLight()
{
  return kMarkerDetector_DarkOnLight;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MarkerDetector::MarkerDetector(const Camera& camera)
: _camera(camera)
, _params(new Parameters())
, _memory(new Memory())
{
  
# if RECOGNITION_METHOD == RECOGNITION_METHOD_NEAREST_NEIGHBOR
  // Force the NN library to load _now_, not on first use
  PRINT_NAMED_INFO("MarkerDetector.Constructor.LoadNearestNeighborLibrary",
                   "Markers generated on %s", Vision::MarkerDefinitionVersionString);
  Embedded::VisionMarker::GetNearestNeighborLibrary();
  
# elif RECOGNITION_METHOD == RECOGNITION_METHOD_CNN
  // TODO: Will need to pass in a datapath from somewhere if we ever switch back to using CNNs
  Embedded::VisionMarker::SetDataPath(dataPath);
# endif
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MarkerDetector::~MarkerDetector()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result MarkerDetector::Init(s32 numRows, s32 numCols)
{
  _params->Initialize();
  
  Result initMemoryResult = _memory->ResetBuffers(numRows, numCols, _params->maxMarkers);
  return initMemoryResult;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static Result GetImageHelper(const Vision::Image& srcImage, Embedded::Array<u8>& destArray)
{
  const s32 captureHeight = destArray.get_size(0);
  const s32 captureWidth  = destArray.get_size(1);
  
  if(srcImage.GetNumRows() != captureHeight || srcImage.GetNumCols() != captureWidth) {
    PRINT_NAMED_ERROR("MarkerDetector.GetImageHelper.MismatchedImageSizes",
                      "Source Vision::Image and destination Embedded::Array should "
                      "be the same size (source is %dx%d and destination is %dx%d)",
                      srcImage.GetNumRows(), srcImage.GetNumCols(),
                      captureHeight, captureWidth);
    return RESULT_FAIL_INVALID_SIZE;
  }
  
  memcpy(reinterpret_cast<u8*>(destArray.get_buffer()),
         srcImage.GetDataPointer(),
         captureHeight*captureWidth*sizeof(u8));
  
  return RESULT_OK;
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result MarkerDetector::Detect(const Image& inputImageGray, std::list<ObservedMarker>& observedMarkers)
{
  ANKI_CPU_PROFILE("MarkerDetector_Detect");
  
  DEV_ASSERT(_params->isInitialized, "MarkerDetector.Detect.ParamsNoInitialized");
  
  _memory->ResetBuffers(inputImageGray.GetNumRows(), inputImageGray.GetNumCols(), _params->maxMarkers);
  
  // Convert to an Embedded::Array<u8> so the old embedded methods can use the
  // image data.
  Embedded::Array<u8> grayscaleImage(inputImageGray.GetNumRows(), inputImageGray.GetNumCols(),
                                     _memory->_onchipScratch,
                                     Embedded::Flags::Buffer(false,false,false));
  
  GetImageHelper(inputImageGray, grayscaleImage);
  
  Embedded::FixedLengthList<Embedded::VisionMarker>& markers = _memory->_markers;
  const s32 maxMarkers = markers.get_maximumSize();
  
  markers.set_size(maxMarkers);
  for(s32 i=0; i<maxMarkers; i++) {
    Embedded::Array<f32> newArray(3, 3, _memory->_ccmScratch);
    markers[i].homography = newArray;
  }
  
  DEV_ASSERT(Util::IsFltGTZero(_params->fiducialThicknessFraction.x) &&
             Util::IsFltGTZero(_params->fiducialThicknessFraction.y),
             "MarkerDetector.Detect.FiducialThicknessFractionParameterNotInitialized");
  
  _params->SetComputeComponentMinNumPixels(inputImageGray.GetNumRows(), inputImageGray.GetNumCols());
  _params->SetComputeComponentMaxNumPixels(inputImageGray.GetNumRows(), inputImageGray.GetNumCols());
  
  if(!kMarkerDetector_DarkOnLight)
  {
    _params->scaleImage_thresholdMultiplier = _params->scaleImage_thresholdMultiplier_lightOnDark;
  }
  
  const Result result = DetectFiducialMarkers(grayscaleImage,
                                              markers,
                                              *_params,
                                              kMarkerDetector_DarkOnLight,
                                              _memory->_ccmScratch,
                                              _memory->_onchipScratch,
                                              _memory->_offchipScratch);
  
  if(result != RESULT_OK) {
    return result;
  }
  
  const s32 numMarkers = _memory->_markers.get_size();

  for(s32 i_marker = 0; i_marker < numMarkers; ++i_marker)
  {
    const Embedded::VisionMarker& crntMarker = _memory->_markers[i_marker];
    
    // Construct a basestation quad from an embedded one:
    Quad2f quad({crntMarker.corners[Embedded::Quadrilateral<f32>::TopLeft].x,
                 crntMarker.corners[Embedded::Quadrilateral<f32>::TopLeft].y},
                {crntMarker.corners[Embedded::Quadrilateral<f32>::BottomLeft].x,
                  crntMarker.corners[Embedded::Quadrilateral<f32>::BottomLeft].y},
                {crntMarker.corners[Embedded::Quadrilateral<f32>::TopRight].x,
                  crntMarker.corners[Embedded::Quadrilateral<f32>::TopRight].y},
                {crntMarker.corners[Embedded::Quadrilateral<f32>::BottomRight].x,
                  crntMarker.corners[Embedded::Quadrilateral<f32>::BottomRight].y});
    
    observedMarkers.emplace_back(inputImageGray.GetTimestamp(),
                                 crntMarker.markerType,
                                 quad, _camera);
  } // for(each marker)

  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MarkerDetector::Parameters::Parameters()
: isInitialized(false)
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MarkerDetector::Parameters::SetComputeComponentMinNumPixels(s32 numRows, s32 numCols)
{
  const f32 minSideLength = minSideLengthFraction*static_cast<f32>(MIN(numRows,numCols));
  const f32 interiorLength = component_numPixelsFraction*minSideLength;
  component_minimumNumPixels = std::round(minSideLength*minSideLength - (interiorLength*interiorLength));
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MarkerDetector::Parameters::SetComputeComponentMaxNumPixels(s32 numRows, s32 numCols)
{
  const f32 maxSideLength = maxSideLengthFraction*static_cast<f32>(MAX(numRows,numCols));
  const f32 interiorLength = component_numPixelsFraction*maxSideLength;
  component_maximumNumPixels = std::round(maxSideLength*maxSideLength - (interiorLength*interiorLength));
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MarkerDetector::Parameters::Initialize()
{
  //
  // Initialize Embedded base class members:
  //
  
  useIntegralImageFiltering = true;
  useIlluminationNormalization = true;
  
  scaleImage_thresholdMultiplier = static_cast<s32>(65536.f * 0.8f);
  //scaleImage_thresholdMultiplier = 65536; // 1.0*(2^16)=65536
  //scaleImage_thresholdMultiplier = 49152; // 0.75*(2^16)=49152
  
  scaleImage_numPyramidLevels = 1;
  imagePyramid_baseScale = 4;
  
  component1d_minComponentWidth = 0;
  component1d_maxSkipDistance = 0;
  
  component_sparseMultiplyThreshold = 1000 << 5;
  component_solidMultiplyThreshold = 2 << 5;
  
  component_minHollowRatio = 1.0f;
  
  cornerMethod = Embedded::CORNER_METHOD_LINE_FITS; // {CORNER_METHOD_LAPLACIAN_PEAKS, CORNER_METHOD_LINE_FITS};
  
  // Ratio of 4th to 5th biggest Laplacian peak must be greater than this
  // for a quad to be extracted from a connected component
  minLaplacianPeakRatio = 5;
  
  maxExtractedQuads = 1000/2;
  quads_minQuadArea = 100/4;
  quads_quadSymmetryThreshold = 512; // ANS: corresponds to 2.0, loosened from 384 (1.5), for large mat markers at extreme perspective distortion
  quads_minDistanceFromImageEdge = 2;
  
  decode_minContrastRatio = 1.01f;
  
  maxConnectedComponentSegments = 39000; // 322*240/2 = 38640
  
  // Maximum number of refinement iterations (i.e. if convergence is not
  // detected in the meantime according to minCornerChange parameter below)
  refine_quadRefinementIterations = 25;
  
  // TODO: Could this be fewer samples?
  refine_numRefinementSamples = 100;
  
  // If quad refinment moves any corner by more than this (in pixels), the
  // original quad/homography are restored.
  refine_quadRefinementMaxCornerChange = 5.f;
  
  // If quad refinement moves all corners by less than this (in pixels),
  // the refinment is considered converged and stops immediately
  refine_quadRefinementMinCornerChange = 0.005f;
  
  doCodeExtraction = true;
  
  // Return unknown/unverified markers (e.g. for display)
  returnInvalidMarkers = false;
  
  // Thickness of the fiducial rectangle, relative to its width/height
  fiducialThicknessFraction.x = fiducialThicknessFraction.y = 0.1f;
  
  // Radius of rounds as a fraction of the height/width of the fiducial rectangle
  roundedCornersFraction.x = roundedCornersFraction.y = 0.15f;

  //
  // Initialize members added by this derived class
  //
  
  maxMarkers = 100;
  
  minSideLengthFraction = 0.03f;
  maxSideLengthFraction = 0.97f;
  
  component_numPixelsFraction = 0.8f;
  
  isInitialized = true;
}
  
} // namespace Vision
} // namesapce Anki
