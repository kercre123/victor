/**
 * File: visionSystem.cpp [Basestation]
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: High-level module that controls the basestation vision system
 *              Runs on its own thread inside VisionProcessingThread.
 *
 *  NOTE: Current implementation is basically a copy of the Embedded vision system
 *    on the robot, so we can first see if vision-over-WiFi is feasible before a
 *    native Basestation implementation of everything.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "visionSystem.h"

#include "anki/cozmo/basestation/comms/robot/robotMessages.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/mailbox_impl.h"
#include "anki/vision/basestation/image_impl.h"

#include "anki/common/basestation/platformPathManager.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

//
// Embedded implementation holdovers:
//  (these should probably all go away once basestation vision is natively implemented)

#include "anki/common/robot/config.h"
// Coretech Vision Includes
#include "anki/vision/MarkerCodeDefinitions.h"
#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/fiducialMarkers.h"
#include "anki/vision/robot/imageProcessing.h"
#include "anki/vision/robot/perspectivePoseEstimation.h"
#include "anki/vision/robot/classifier.h"
#include "anki/vision/robot/lbpcascade_frontalface.h"
#include "anki/vision/robot/cameraImagingPipeline.h"

// CoreTech Common Includes
#include "anki/common/shared/radians.h"
#include "anki/common/robot/benchmarking.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/utilities.h"

// Cozmo-Specific Library Includes
#include "anki/cozmo/shared/cozmoConfig.h"
//#include "anki/cozmo/robot/hal.h"

// Local Cozmo Includes
//#include "headController.h"
//#include "imuFilter.h"
//#include "matlabVisualization.h"
//#include "localization.h"
//#include "visionDebugStream.h"

#if USE_MATLAB_TRACKER || USE_MATLAB_DETECTOR
#include "matlabVisionProcessor.h"
#endif

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE && !USE_APPROXIMATE_DOCKING_ERROR_SIGNAL
#error Affine tracker requires that USE_APPROXIMATE_DOCKING_ERROR_SIGNAL = 1.
#endif

namespace Anki {
namespace Cozmo {
  
  using namespace Embedded;
  
  VisionSystem::VisionSystem()
  : _isInitialized(false)
  , _headCamInfo(nullptr)
  {
    
  } // VisionSystem()
  
  VisionSystem::~VisionSystem()
  {
    if(_headCamInfo != nullptr) {
      delete _headCamInfo;
    }
  }
  
  
  // WARNING: ResetBuffers should be used with caution
  Result VisionSystem::VisionMemory::ResetBuffers()
  {
    _offchipScratch = MemoryStack(offchipBuffer, OFFCHIP_BUFFER_SIZE);
    _onchipScratch  = MemoryStack(onchipBuffer, ONCHIP_BUFFER_SIZE);
    _ccmScratch     = MemoryStack(ccmBuffer, CCM_BUFFER_SIZE);
    
    if(!_offchipScratch.IsValid() || !_onchipScratch.IsValid() || !_ccmScratch.IsValid()) {
      PRINT_STREAM_INFO("VisionSystem.VisionMemory.ResetBuffers", "Error: InitializeScratchBuffers");
      return RESULT_FAIL;
    }
    
    _markers = FixedLengthList<VisionMarker>(VisionMemory::MAX_MARKERS, _offchipScratch);
    
    return RESULT_OK;
  }
  
  Result VisionSystem::VisionMemory::Initialize()
  {
    return ResetBuffers();
  }
  
  
  //
  // Implementation of MarkerToTrack methods:
  //
  
  VisionSystem::MarkerToTrack::MarkerToTrack()
  {
    Clear();
  }
  
  void VisionSystem::MarkerToTrack::Clear() {
    type        = Anki::Vision::MARKER_UNKNOWN;
    width_mm    = 0;
    imageCenter = Embedded::Point2f(-1.f, -1.f);
    imageSearchRadius = -1.f;
    checkAngleX = true;
  }
  
  bool VisionSystem::MarkerToTrack::Matches(const VisionMarker& marker) const
  {
    bool doesMatch = false;
    
    if(marker.markerType == this->type) {
      if(this->imageCenter.x >= 0.f && this->imageCenter.y >= 0.f &&
         this->imageSearchRadius > 0.f)
      {
        // There is an image position specified, check to see if the
        // marker's centroid is close enough to it
        Embedded::Point2f centroid = marker.corners.ComputeCenter<f32>();
        if( (centroid - this->imageCenter).Length() < this->imageSearchRadius ) {
          doesMatch = true;
        }
      } else {
        // No image position specified, just return true since the
        // types match
        doesMatch = true;
      }
    }
    
    return doesMatch;
  } // MarkerToTrack::Matches()
  
#if 0
#pragma mark --- Mode Controls ---
#endif
  
  void VisionSystem::EnableModeHelper(Mode mode)
  {
    const bool modeAlreadyEnabled = _mode & mode;
    if(!modeAlreadyEnabled) {
      PRINT_NAMED_INFO("VisionSystem.EnableModeHelper",
                       "Adding mode %s to current mode %s.",
                       VisionSystem::GetModeName(static_cast<Mode>(mode)).c_str(),
                       VisionSystem::GetModeName(static_cast<Mode>(_mode)).c_str());
      
      _mode |= mode;
    }
  }
  
  void VisionSystem::DisableModeHelper(Mode mode)
  {
    _mode &= ~mode;
  }
  
  void VisionSystem::StartMarkerDetection()
  {
    EnableModeHelper(LOOKING_FOR_MARKERS);
  }
  
  void VisionSystem::StopMarkerDetection()
  {
    DisableModeHelper(LOOKING_FOR_MARKERS);
  }
  
  void VisionSystem::StopTracking()
  {
    SetMarkerToTrack(Vision::MARKER_UNKNOWN, 0.f, true);
    DisableModeHelper(TRACKING);
    
    // Restore whatever we were doing before tracking
    _mode = _modeBeforeTracking;
  }
  
  Result VisionSystem::StartDetectingFaces()
  {
    EnableModeHelper(DETECTING_FACES);
    return RESULT_OK;
  }
  
  Result VisionSystem::StopDetectingFaces()
  {
    DisableModeHelper(DETECTING_FACES);
    return RESULT_OK;
  }
  
#if 0
#pragma mark --- Simulator-Related Definitions ---
#endif
  // This little namespace is just for simulated processing time for
  // tracking and detection (since those run far faster in simulation on
  // a PC than they do on embedded hardware. Basically, this is used by
  // Update() below to wait until a frame is ready before proceeding.
  namespace Simulator {

    static Result Initialize() { return RESULT_OK; }
    static bool IsFrameReady() { return true; }
    static void SetDetectionReadyTime() { }
    static void SetTrackingReadyTime() { }
    static void SetFaceDetectionReadyTime() {}

  } // namespace Simulator
  
  
  Embedded::Quadrilateral<f32> VisionSystem::GetTrackerQuad(MemoryStack scratch)
  {
#if USE_MATLAB_TRACKER
    return MatlabVisionProcessor::GetTrackerQuad();
#else
    return _tracker.get_transformation().get_transformedCorners(scratch);
#endif
  } // GetTrackerQuad()
  
  Result VisionSystem::UpdateRobotState(const MessageRobotState newRobotState)
  {
    _prevRobotState = _robotState;
    _robotState     = newRobotState;
    
    if(_wasCalledOnce) {
      _havePreviousRobotState = true;
    } else {
      _wasCalledOnce = true;
    }
    
    return RESULT_OK;
  } // UpdateRobotState()
  
  
  void VisionSystem::GetPoseChange(f32& xChange, f32& yChange, Radians& angleChange)
  {
    AnkiAssert(_havePreviousRobotState);
    
    angleChange = Radians(_robotState.pose_angle) - Radians(_prevRobotState.pose_angle);
    
    //PRINT_STREAM_INFO("angleChange = %.1f", angleChange.getDegrees());
    
    // Position change in world (mat) coordinates
    const f32 dx = _robotState.pose_x - _prevRobotState.pose_x;
    const f32 dy = _robotState.pose_y - _prevRobotState.pose_y;
    
    // Get change in robot coordinates
    const f32 cosAngle = cosf(-_prevRobotState.pose_angle);
    const f32 sinAngle = sinf(-_prevRobotState.pose_angle);
    xChange = dx*cosAngle - dy*sinAngle;
    yChange = dx*sinAngle + dy*cosAngle;
  } // GetPoseChange()
  
  
  // This function actually swaps in the new marker to track, and should
  // not be made available as part of the public API since it could get
  // interrupted by main and we want all this stuff updated at once.
  Result VisionSystem::UpdateMarkerToTrack()
  {
    if(_newMarkerToTrackWasProvided) {
      
      _mode              |= LOOKING_FOR_MARKERS;
      _mode              &= ~TRACKING; // disable tracking mode
      _numTrackFailures  =  0;
      
      _markerToTrack = _newMarkerToTrack;
      
      if(_markerToTrack.IsSpecified()) {
        
        AnkiConditionalErrorAndReturnValue(_markerToTrack.width_mm > 0.f,
                                           RESULT_FAIL_INVALID_PARAMETER,
                                           "VisionSystem::UpdateMarkerToTrack()",
                                           "Invalid marker width specified.");
        
        // Set canonical 3D marker's corner coordinates
        const P3P_PRECISION markerHalfWidth = _markerToTrack.width_mm * P3P_PRECISION(0.5);
        _canonicalMarker3d[0] = Embedded::Point3<P3P_PRECISION>(-markerHalfWidth, -markerHalfWidth, 0);
        _canonicalMarker3d[1] = Embedded::Point3<P3P_PRECISION>(-markerHalfWidth,  markerHalfWidth, 0);
        _canonicalMarker3d[2] = Embedded::Point3<P3P_PRECISION>( markerHalfWidth, -markerHalfWidth, 0);
        _canonicalMarker3d[3] = Embedded::Point3<P3P_PRECISION>( markerHalfWidth,  markerHalfWidth, 0);
      } // if markerToTrack is valid
      
      _newMarkerToTrack.Clear();
      _newMarkerToTrackWasProvided = false;
    } // if newMarker provided
    
    return RESULT_OK;
    
  } // UpdateMarkerToTrack()
  
  
  Radians VisionSystem::GetCurrentHeadAngle()
  {
    return _robotState.headAngle;
  }
  
  
  Radians VisionSystem::GetPreviousHeadAngle()
  {
    return _prevRobotState.headAngle;
  }
  

  bool VisionSystem::CheckMailbox(MessageDockingErrorSignal&  msg)
  {
    bool retVal = false;
    if(IsInitialized()) {
      retVal = _dockingMailbox.getMessage(msg);
    }
    return retVal;
  }
  
  bool VisionSystem::CheckMailbox(MessageFaceDetection&       msg)
  {
    bool retVal = false;
    if(IsInitialized()) {
      retVal = _faceDetectMailbox.getMessage(msg);
    }
    return retVal;
  }
  
  bool VisionSystem::CheckMailbox(MessageVisionMarker&        msg)
  {
    bool retVal = false;
    if(IsInitialized()) {
      retVal = _visionMarkerMailbox.getMessage(msg);
    }
    return retVal;
  }
  
  bool VisionSystem::CheckMailbox(MessageTrackerQuad&         msg)
  {
    bool retVal = false;
    if(IsInitialized()) {
      retVal = _trackerMailbox.getMessage(msg);
    }
    return retVal;
  }
  
  bool VisionSystem::CheckMailbox(MessagePanAndTiltHead&         msg)
  {
    bool retVal = false;
    if(IsInitialized()) {
      retVal = _panTiltMailbox.getMessage(msg);
    }
    return retVal;
  }
  
  bool VisionSystem::IsInitialized() const
  {
    bool retVal = _isInitialized;
#   if ANKI_COZMO_USE_MATLAB_VISION
    retVal &= _matlab.ep != NULL;
#   endif
    return retVal;
  }
  
  Result VisionSystem::LookForMarkers(
                                      const Array<u8> &grayscaleImage,
                                      const DetectFiducialMarkersParameters &parameters,
                                      FixedLengthList<VisionMarker> &markers,
                                      MemoryStack ccmScratch,
                                      MemoryStack onchipScratch,
                                      MemoryStack offchipScratch)
  {
    BeginBenchmark("VisionSystem_LookForMarkers");
    
    AnkiAssert(parameters.isInitialized);
    
    const s32 maxMarkers = markers.get_maximumSize();
    
    FixedLengthList<Array<f32> > homographies(maxMarkers, ccmScratch);
    
    markers.set_size(maxMarkers);
    homographies.set_size(maxMarkers);
    
    for(s32 i=0; i<maxMarkers; i++) {
      Array<f32> newArray(3, 3, ccmScratch);
      homographies[i] = newArray;
    }
    
    // TODO: Re-enable DebugStream for Basestation
    //MatlabVisualization::ResetFiducialDetection(grayscaleImage);
    
#if USE_MATLAB_DETECTOR
    const Result result = MatlabVisionProcessor::DetectMarkers(grayscaleImage, markers, homographies, ccmScratch);
#else
    const CornerMethod cornerMethod = CORNER_METHOD_LAPLACIAN_PEAKS; // {CORNER_METHOD_LAPLACIAN_PEAKS, CORNER_METHOD_LINE_FITS};
    
    const Result result = DetectFiducialMarkers(
                                                grayscaleImage,
                                                markers,
                                                homographies,
                                                parameters.scaleImage_numPyramidLevels, parameters.scaleImage_thresholdMultiplier,
                                                parameters.component1d_minComponentWidth, parameters.component1d_maxSkipDistance,
                                                parameters.component_minimumNumPixels, parameters.component_maximumNumPixels,
                                                parameters.component_sparseMultiplyThreshold, parameters.component_solidMultiplyThreshold,
                                                parameters.component_minHollowRatio,
                                                cornerMethod, parameters.minLaplacianPeakRatio,
                                                parameters.quads_minQuadArea, parameters.quads_quadSymmetryThreshold, parameters.quads_minDistanceFromImageEdge,
                                                parameters.decode_minContrastRatio,
                                                parameters.maxConnectedComponentSegments,
                                                parameters.maxExtractedQuads,
                                                parameters.quadRefinementIterations,
                                                parameters.numRefinementSamples,
                                                parameters.quadRefinementMaxCornerChange,
                                                parameters.quadRefinementMinCornerChange,
                                                parameters.keepUnverifiedMarkers,
                                                ccmScratch, onchipScratch, offchipScratch);
#endif
    
    if(result != RESULT_OK) {
      return result;
    }
    
    EndBenchmark("VisionSystem_LookForMarkers");
    
    // TODO: Re-enable DebugStream for Basestation
    /*
     DebugStream::SendFiducialDetection(grayscaleImage, markers, ccmScratch, onchipScratch, offchipScratch);
     
     for(s32 i_marker = 0; i_marker < markers.get_size(); ++i_marker) {
     const VisionMarker crntMarker = markers[i_marker];
     
     MatlabVisualization::SendFiducialDetection(crntMarker.corners, crntMarker.markerType);
     }
     
     MatlabVisualization::SendDrawNow();
     */
    
    return RESULT_OK;
  } // LookForMarkers()
  
  
  
  // Divide image by mean of whatever is inside the trackingQuad
  static Result BrightnessNormalizeImage(Embedded::Array<u8>& image,
                                         const Embedded::Quadrilateral<f32>& quad)
  {
    //Debug: image.Show("OriginalImage", false);
    
#define USE_VARIANCE 0
    
    // Compute mean of data inside the bounding box of the tracking quad
    const Embedded::Rectangle<s32> bbox = quad.ComputeBoundingRectangle<s32>();
    
    ConstArraySlice<u8> imageROI = image(bbox.top, bbox.bottom, bbox.left, bbox.right);
    
#if USE_VARIANCE
    // Playing with normalizing using std. deviation as well
    s32 mean, var;
    Matrix::MeanAndVar<u8, s32>(imageROI, mean, var);
    const f32 stddev = sqrt(static_cast<f32>(var));
    const f32 oneTwentyEightOverStdDev = 128.f / stddev;
    //PRINT("Initial mean/std = %d / %.2f", mean, sqrt(static_cast<f32>(var)));
#else
    const u8 mean = Embedded::Matrix::Mean<u8, u32>(imageROI);
    //PRINT("Initial mean = %d", mean);
#endif
    
    //PRINT("quad mean = %d", mean);
    //const f32 oneOverMean = 1.f / static_cast<f32>(mean);
    
    // Remove mean (and variance) from image
    for(s32 i=0; i<image.get_size(0); ++i)
    {
      u8 * restrict img_i = image.Pointer(i, 0);
      
      for(s32 j=0; j<image.get_size(1); ++j)
      {
        f32 value = static_cast<f32>(img_i[j]);
        value -= static_cast<f32>(mean);
#if USE_VARIANCE
        value *= oneTwentyEightOverStdDev;
#endif
        value += 128.f;
        img_i[j] = saturate_cast<u8>(value) ;
      }
    }
    
    // Debug:
    /*
     #if USE_VARIANCE
     Matrix::MeanAndVar<u8, s32>(imageROI, mean, var);
     PRINT("Final mean/std = %d / %.2f", mean, sqrt(static_cast<f32>(var)));
     #else
     PRINT("Final mean = %d", Matrix::Mean<u8,u32>(imageROI));
     #endif
     */
    
    //Debug: image.Show("NormalizedImage", true);
    
#undef USE_VARIANCE
    return RESULT_OK;
    
  } // BrightnessNormalizeImage()
  
  
  static Result BrightnessNormalizeImage(Array<u8>& image, const Embedded::Quadrilateral<f32>& quad,
                                         const f32 filterWidthFraction,
                                         MemoryStack scratch)
  {
    if(filterWidthFraction > 0.f) {
      //Debug:
      image.Show("OriginalImage", false);
      
      // TODO: Add the ability to only normalize within the vicinity of the quad
      // Note that this requires templateQuad to be sorted!
      const s32 filterWidth = static_cast<s32>(filterWidthFraction*((quad[3] - quad[0]).Length()));
      AnkiAssert(filterWidth > 0.f);
      
      Array<u8> imageNormalized(image.get_size(0), image.get_size(1), scratch);
      
      AnkiConditionalErrorAndReturnValue(imageNormalized.IsValid(),
                                         RESULT_FAIL_OUT_OF_MEMORY,
                                         "VisionSystem::BrightnessNormalizeImage",
                                         "Out of memory allocating imageNormalized.");
      
      BeginBenchmark("BoxFilterNormalize");
      
      ImageProcessing::BoxFilterNormalize(image, filterWidth, static_cast<u8>(128),
                                          imageNormalized, scratch);
      
      EndBenchmark("BoxFilterNormalize");
      
      { // DEBUG
        /*
         static Matlab matlab(false);
         matlab.PutArray(grayscaleImage, "grayscaleImage");
         matlab.PutArray(grayscaleImageNormalized, "grayscaleImageNormalized");
         matlab.EvalString("subplot(121), imagesc(grayscaleImage), axis image, colorbar, "
         "subplot(122), imagesc(grayscaleImageNormalized), colorbar, axis image, "
         "colormap(gray)");
         */
        
        //image.Show("GrayscaleImage", false);
        //imageNormalized.Show("GrayscaleImageNormalized", false);
      }
      
      image.Set(imageNormalized);
      
      //Debug:
      //image.Show("NormalizedImage", true);
      
    } // if(filterWidthFraction > 0)
    
    return RESULT_OK;
  } // BrightnessNormalizeImage()
  
  Result VisionSystem::InitTemplate(const Array<u8> &grayscaleImage,
                                    const Embedded::Quadrilateral<f32> &trackingQuad,
                                    const TrackerParameters &parameters,
                                    Tracker &tracker,
                                    MemoryStack ccmScratch,
                                    MemoryStack &onchipMemory, //< NOTE: onchip is a reference
                                    MemoryStack &offchipMemory)
  {
    AnkiAssert(parameters.isInitialized);
    AnkiAssert(_markerToTrack.width_mm > 0);
    
    _trackingIteration = 0;
    
#if USE_MATLAB_TRACKER
    return MatlabVisionProcessor::InitTemplate(grayscaleImage, trackingQuad, ccmScratch);
#endif
    
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
    // TODO: At some point template initialization should happen at full detection resolution but for
    //       now, we have to downsample to tracking resolution
    
    Array<u8> grayscaleImageSmall(parameters.trackingImageHeight, parameters.trackingImageWidth, ccmScratch);
    u32 downsampleFactor = DownsampleHelper(grayscaleImage, grayscaleImageSmall, ccmScratch);
    
    AnkiAssert(downsampleFactor > 0);
    // Note that the templateRegion and the trackingQuad are both at DETECTION_RESOLUTION, not
    // necessarily the resolution of the frame.
    //const u32 downsampleFactor = parameters.detectionWidth / parameters.trackingImageWidth;
    //const u32 downsamplePower = Log2u32(downsampleFactor);
    
    /*Quadrilateral<f32> trackingQuadSmall;
     
     for(s32 i=0; i<4; ++i) {
     trackingQuadSmall[i].x = trackingQuad[i].x / downsampleFactor;
     trackingQuadSmall[i].y = trackingQuad[i].y / downsampleFactor;
     }*/
    
#endif // #if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
    
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW
    tracker = TemplateTracker::LucasKanadeTracker_Slow(
                                                       grayscaleImageSmall,
                                                       trackingQuad,
                                                       parameters.scaleTemplateRegionPercent,
                                                       parameters.numPyramidLevels,
                                                       Transformations::TRANSFORM_TRANSLATION,
                                                       0.0,
                                                       onchipMemory);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
    tracker = TemplateTracker::LucasKanadeTracker_Affine(
                                                         grayscaleImageSmall,
                                                         trackingQuad,
                                                         parameters.scaleTemplateRegionPercent,
                                                         parameters.numPyramidLevels,
                                                         Transformations::TRANSFORM_AFFINE,
                                                         onchipMemory);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
    tracker = TemplateTracker::LucasKanadeTracker_Projective(
                                                             grayscaleImageSmall,
                                                             trackingQuad,
                                                             parameters.scaleTemplateRegionPercent,
                                                             parameters.numPyramidLevels,
                                                             Transformations::TRANSFORM_PROJECTIVE,
                                                             onchipMemory);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE
    tracker = TemplateTracker::LucasKanadeTracker_SampledProjective(
                                                                    grayscaleImage,
                                                                    trackingQuad,
                                                                    parameters.scaleTemplateRegionPercent,
                                                                    parameters.numPyramidLevels,
                                                                    Transformations::TRANSFORM_PROJECTIVE,
                                                                    parameters.maxSamplesAtBaseLevel,
                                                                    ccmScratch,
                                                                    onchipMemory,
                                                                    offchipMemory);
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
#ifdef USE_HEADER_TEMPLATE
    tracker = TemplateTracker::BinaryTracker(
                                             Vision::MARKER_BATTERIES,
                                             grayscaleImage,
                                             trackingQuad,
                                             parameters.scaleTemplateRegionPercent,
                                             parameters.edgeDetectionParams_template,
                                             onchipMemory, offchipMemory);
#else // #ifdef USE_HEADER_TEMPLATE
    tracker = TemplateTracker::BinaryTracker(
                                             grayscaleImage,
                                             trackingQuad,
                                             parameters.scaleTemplateRegionPercent,
                                             parameters.edgeDetectionParams_template,
                                             onchipMemory, offchipMemory);
#endif // #ifdef USE_HEADER_TEMPLATE ... #else
    
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
    
    tracker = TemplateTracker::LucasKanadeTracker_SampledPlanar6dof(grayscaleImage,
                                                                    trackingQuad,
                                                                    parameters.scaleTemplateRegionPercent,
                                                                    parameters.numPyramidLevels,
                                                                    Transformations::TRANSFORM_PROJECTIVE,
                                                                    parameters.numFiducialEdgeSamples,
                                                                    FIDUCIAL_SQUARE_WIDTH_FRACTION,
                                                                    parameters.numInteriorSamples,
                                                                    parameters.numSamplingRegions,
                                                                    _headCamInfo->focalLength_x,
                                                                    _headCamInfo->focalLength_y,
                                                                    _headCamInfo->center_x,
                                                                    _headCamInfo->center_y,
                                                                    _markerToTrack.width_mm,
                                                                    ccmScratch,
                                                                    onchipMemory,
                                                                    offchipMemory);
    
    /*
     // TODO: Set this elsewhere
     const f32 Kp_min = 0.05f;
     const f32 Kp_max = 0.75f;
     const f32 tz_min = 30.f;
     const f32 tz_max = 150.f;
     tracker.SetGainScheduling(tz_min, tz_max, Kp_min, Kp_max);
     */
#else
#error Unknown DOCKING_ALGORITHM.
#endif
    
    if(!tracker.IsValid()) {
      PRINT_NAMED_ERROR("VisionSystem.InitTemplate", "Failed to initialize valid tracker.");
      return RESULT_FAIL;
    }
    
    /*
     // TODO: Re-enable visualization/debugstream on basestation
     MatlabVisualization::SendTrackInit(grayscaleImage, tracker, onchipMemory);
     
     #if DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
     DebugStream::SendBinaryTracker(tracker, ccmScratch, onchipMemory, offchipMemory);
     #endif
     */
    
    return RESULT_OK;
  } // InitTemplate()
  
  
  
  Result VisionSystem::TrackTemplate(const Array<u8> &grayscaleImage,
                                     const Embedded::Quadrilateral<f32> &trackingQuad,
                                     const TrackerParameters &parameters,
                                     Tracker &tracker,
                                     bool &trackingSucceeded,
                                     MemoryStack ccmScratch,
                                     MemoryStack onchipScratch,
                                     MemoryStack offchipScratch)
  {
    BeginBenchmark("VisionSystem_TrackTemplate");
    
    AnkiAssert(parameters.isInitialized);
    
#if USE_MATLAB_TRACKER
    return MatlabVisionProcessor::TrackTemplate(grayscaleImage, converged, ccmScratch);
#endif
    
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
    // TODO: At some point template initialization should happen at full detection resolution
    //       but for now, we have to downsample to tracking resolution
    Array<u8> grayscaleImageSmall(parameters.trackingImageHeight, parameters.trackingImageWidth, ccmScratch);
    DownsampleHelper(grayscaleImage, grayscaleImageSmall, ccmScratch);
    
    //DebugStream::SendArray(grayscaleImageSmall);
#endif
    
    trackingSucceeded = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;
    
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW
    const Result trackerResult = tracker.UpdateTrack(
                                                     grayscaleImage,
                                                     parameters.maxIterations,
                                                     parameters.convergenceTolerance,
                                                     parameters.useWeights,
                                                     trackingSucceeded,
                                                     onchipScratch);
    
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
    const Result trackerResult = tracker.UpdateTrack(
                                                     grayscaleImageSmall,
                                                     parameters.maxIterations,
                                                     parameters.convergenceTolerance,
                                                     parameters.verify_maxPixelDifference,
                                                     trackingSucceeded,
                                                     verify_meanAbsoluteDifference,
                                                     verify_numInBounds,
                                                     verify_numSimilarPixels,
                                                     onchipScratch);
    
    //tracker.get_transformation().Print("track");
    
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
    const Result trackerResult = tracker.UpdateTrack(
                                                     grayscaleImageSmall,
                                                     parameters.maxIterations,
                                                     parameters.convergenceTolerance,
                                                     parameters.verify_maxPixelDifference,
                                                     trackingSucceeded,
                                                     verify_meanAbsoluteDifference,
                                                     verify_numInBounds,
                                                     verify_numSimilarPixels,
                                                     onchipScratch);
    
    //tracker.get_transformation().Print("track");
    
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE
    
    const Result trackerResult = tracker.UpdateTrack(
                                                     grayscaleImage,
                                                     parameters.maxIterations,
                                                     parameters.convergenceTolerance,
                                                     parameters.verify_maxPixelDifference,
                                                     trackingSucceeded,
                                                     verify_meanAbsoluteDifference,
                                                     verify_numInBounds,
                                                     verify_numSimilarPixels,
                                                     onchipScratch);
    
    //tracker.get_transformation().Print("track");
    
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
    s32 numMatches = -1;
    
    const Result trackerResult = tracker.UpdateTrack_Normal(
                                                            grayscaleImage,
                                                            parameters.edgeDetectionParams_update,
                                                            parameters.matching_maxTranslationDistance,
                                                            parameters.matching_maxProjectiveDistance,
                                                            parameters.verify_maxTranslationDistance,
                                                            parameters.verify_maxPixelDifference,
                                                            parameters.verify_coordinateIncrement,
                                                            numMatches,
                                                            verify_meanAbsoluteDifference,
                                                            verify_numInBounds,
                                                            verify_numSimilarPixels,
                                                            ccmScratch, offchipScratch);
    
    //tracker.get_transformation().Print("track");
    
    const s32 numTemplatePixels = tracker.get_numTemplatePixels();
    
    const f32 percentMatchedPixels = static_cast<f32>(numMatches) / static_cast<f32>(numTemplatePixels);
    
    if(percentMatchedPixels >= parameters.percentMatchedPixelsThreshold) {
      trackingSucceeded = true;
    } else {
      trackingSucceeded = false;
    }
    
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
    
    const Radians initAngleX(tracker.get_angleX());
    const Radians initAngleY(tracker.get_angleY());
    const Radians initAngleZ(tracker.get_angleZ());
    const Embedded::Point3<f32>& initTranslation = tracker.GetTranslation();
    
    bool converged = false;
    ++_trackingIteration;
    const Result trackerResult = tracker.UpdateTrack(grayscaleImage,
                                                     parameters.maxIterations,
                                                     parameters.convergenceTolerance_angle,
                                                     parameters.convergenceTolerance_distance,
                                                     parameters.verify_maxPixelDifference,
                                                     converged,
                                                     verify_meanAbsoluteDifference,
                                                     verify_numInBounds,
                                                     verify_numSimilarPixels,
                                                     onchipScratch);
    
    // TODO: Do we care if converged == false?
    
    //
    // Go through a bunch of checks to see whether the tracking succeeded
    //
    
    if(fabs((initAngleX - tracker.get_angleX()).ToFloat()) > parameters.successTolerance_angle ||
       fabs((initAngleY - tracker.get_angleY()).ToFloat()) > parameters.successTolerance_angle ||
       fabs((initAngleZ - tracker.get_angleZ()).ToFloat()) > parameters.successTolerance_angle)
    {
      PRINT_STREAM_INFO("VisionSystem.TrackTemplate", "Tracker failed: angle(s) changed too much.");
      trackingSucceeded = false;
    }
    else if(tracker.GetTranslation().z < TrackerParameters::MIN_TRACKER_DISTANCE)
    {
      PRINT_STREAM_INFO("VisionSystem.TrackTemplate", "Tracker failed: final distance too close.");
      trackingSucceeded = false;
    }
    else if(tracker.GetTranslation().z > TrackerParameters::MAX_TRACKER_DISTANCE)
    {
      PRINT_STREAM_INFO("VisionSystem.TrackTemplate", "Tracker failed: final distance too far away.");
      trackingSucceeded = false;
    }
    else if((initTranslation - tracker.GetTranslation()).Length() > parameters.successTolerance_distance)
    {
      PRINT_STREAM_INFO("VisionSystem.TrackTemplate", "Tracker failed: position changed too much.");
      trackingSucceeded = false;
    }
    else if(_markerToTrack.checkAngleX && fabs(tracker.get_angleX()) > TrackerParameters::MAX_BLOCK_DOCKING_ANGLE)
    {
      PRINT_STREAM_INFO("VisionSystem.TrackTemplate", "Tracker failed: target X angle too large.");
      trackingSucceeded = false;
    }
    else if(fabs(tracker.get_angleY()) > TrackerParameters::MAX_BLOCK_DOCKING_ANGLE)
    {
      PRINT_STREAM_INFO("VisionSystem.TrackTemplate", "Tracker failed: target Y angle too large.");
      trackingSucceeded = false;
    }
    else if(fabs(tracker.get_angleZ()) > TrackerParameters::MAX_BLOCK_DOCKING_ANGLE)
    {
      PRINT_STREAM_INFO("VisionSystem.TrackTemplate", "Tracker failed: target Z angle too large.");
      trackingSucceeded = false;
    }
    else if(atan_fast(fabs(tracker.GetTranslation().x) / tracker.GetTranslation().z) > TrackerParameters::MAX_DOCKING_FOV_ANGLE)
    {
      PRINT_STREAM_INFO("VisionSystem.TrackTemplate", "Tracker failed: FOV angle too large.");
      trackingSucceeded = false;
    }
    else if( (static_cast<f32>(verify_numSimilarPixels) /
              static_cast<f32>(verify_numInBounds)) < parameters.successTolerance_matchingPixelsFraction)
    {
      PRINT_STREAM_INFO("VisionSystem.TrackTemplate", "Tracker failed: too many in-bounds pixels failed intensity verification (" << verify_numSimilarPixels << " / " << verify_numInBounds << " < " << parameters.successTolerance_matchingPixelsFraction << ").");
      trackingSucceeded = false;
    }
    else {
      // Everything seems ok!
      PRINT_STREAM_INFO("Tracker succeeded", _trackingIteration);
      trackingSucceeded = true;
    }
    
#else
#error Unknown DOCKING_ALGORITHM!
#endif
    
    if(trackerResult != RESULT_OK) {
      return RESULT_FAIL;
    }
    
    // Sanity check on tracker result
#if DOCKING_ALGORITHM != DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
    
    // Check for a super shrunk or super large template
    // (I don't think this works for planar 6dof homographies?  Try dividing by h22?)
#warning broken
    /*{
     // TODO: make not hacky
     const Array<f32> &homography = tracker.get_transformation().get_homography();
     
     const s32 numValues = 4;
     const s32 numMaxValues = 2;
     f32 values[numValues] = {ABS(homography[0][0]), ABS(homography[0][1]), ABS(homography[1][0]), ABS(homography[1][1])};
     s32 maxInds[numMaxValues] = {0, 1};
     for(s32 i=1; i<numValues; i++) {
     if(values[i] > values[maxInds[0]]) {
     maxInds[0] = i;
     }
     }
     
     for(s32 i=0; i<numValues; i++) {
     if(i == maxInds[0])
     continue;
     
     if(values[i] > values[maxInds[1]]) {
     maxInds[1] = i;
     }
     }
     
     const f32 secondValue = values[maxInds[1]];
     
     if(secondValue < 0.1f || secondValue > 40.0f) {
     converged = false;
     }
     }*/
#endif // #if DOCKING_ALGORITHM != DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
    
    EndBenchmark("VisionSystem_TrackTemplate");
    
    // TODO: Re-enable tracker debugstream/vizualization on basestation
    /*
     MatlabVisualization::SendTrack(grayscaleImage, tracker, trackingSucceeded, offchipScratch);
     
     //MatlabVisualization::SendTrackerPrediction_Compare(tracker, offchipScratch);
     
     DebugStream::SendTrackingUpdate(grayscaleImage, tracker, parameters, verify_meanAbsoluteDifference, static_cast<f32>(verify_numSimilarPixels) / static_cast<f32>(verify_numInBounds), ccmScratch, onchipScratch, offchipScratch);
     */
    
    return RESULT_OK;
  } // TrackTemplate()
  
  template<typename T>
  static void GetVizQuad(const Embedded::Quadrilateral<T>&  embeddedQuad,
                         Anki::Quadrilateral<2, T>&         vizQuad)
  {
    vizQuad[Quad::TopLeft].x() = embeddedQuad[Quad::TopLeft].x;
    vizQuad[Quad::TopLeft].y() = embeddedQuad[Quad::TopLeft].y;
    
    vizQuad[Quad::TopRight].x() = embeddedQuad[Quad::TopRight].x;
    vizQuad[Quad::TopRight].y() = embeddedQuad[Quad::TopRight].y;
    
    vizQuad[Quad::BottomLeft].x() = embeddedQuad[Quad::BottomLeft].x;
    vizQuad[Quad::BottomLeft].y() = embeddedQuad[Quad::BottomLeft].y;
    
    vizQuad[Quad::BottomRight].x() = embeddedQuad[Quad::BottomRight].x;
    vizQuad[Quad::BottomRight].y() = embeddedQuad[Quad::BottomRight].y;
  }
  
  
  //
  // Tracker Prediction
  //
  // Adjust the tracker transformation by approximately how much we
  // think we've moved since the last tracking call.
  //
  Result VisionSystem::TrackerPredictionUpdate(const Array<u8>& grayscaleImage, MemoryStack scratch)
  {
    Result result = RESULT_OK;
    
    const Embedded::Quadrilateral<f32> currentQuad = GetTrackerQuad(scratch);
    
    // TODO: Re-enable tracker prediction viz on Basestation
    // MatlabVisualization::SendTrackerPrediction_Before(grayscaleImage, currentQuad);
    Anki::Quad2f vizQuad;
    GetVizQuad(currentQuad, vizQuad);
    VizManager::getInstance()->DrawCameraQuad(9999, vizQuad, NamedColors::BLUE);
    
    // Ask VisionState how much we've moved since last call (in robot coordinates)
    Radians theta_robot;
    f32 T_fwd_robot, T_hor_robot;
    
    GetPoseChange(T_fwd_robot, T_hor_robot, theta_robot);
    
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
    
#if USE_MATLAB_TRACKER
    MatlabVisionProcessor::UpdateTracker(T_fwd_robot, T_hor_robot,
                                         theta_robot, theta_head);
#else
    Radians theta_head2 = GetCurrentHeadAngle();
    Radians theta_head1 = GetPreviousHeadAngle();
    
    const f32 cH1 = cosf(theta_head1.ToFloat());
    const f32 sH1 = sinf(theta_head1.ToFloat());
    
    const f32 cH2 = cosf(theta_head2.ToFloat());
    const f32 sH2 = sinf(theta_head2.ToFloat());
    
    const f32 cR = cosf(theta_robot.ToFloat());
    const f32 sR = sinf(theta_robot.ToFloat());
    
    // NOTE: these "geometry" entries were computed symbolically with Sage
    // In the derivation, it was assumed the head and neck positions' Y
    // components are zero.
    //
    // From Sage:
    // [cos(thetaR)                 sin(thetaH1)*sin(thetaR)       cos(thetaH1)*sin(thetaR)]
    // [-sin(thetaH2)*sin(thetaR)   cos(thetaR)*sin(thetaH1)*sin(thetaH2) + cos(thetaH1)*cH2  cos(thetaH1)*cos(thetaR)*sin(thetaH2) - cos(thetaH2)*sin(thetaH1)]
    // [-cos(thetaH2)*sin(thetaR)   cos(thetaH2)*cos(thetaR)*sin(thetaH1) - cos(thetaH1)*sin(thetaH2) cos(thetaH1)*cos(thetaH2)*cos(thetaR) + sin(thetaH1)*sin(thetaH2)]
    //
    // T_blockRelHead_new =
    // [T_hor*cos(thetaR) + (Hx*cos(thetaH1) - Hz*sin(thetaH1) + Nx)*sin(thetaR) - T_fwd*sin(thetaR)]
    // [(Hx*cos(thetaH1) - Hz*sin(thetaH1) + Nx)*cos(thetaR)*sin(thetaH2) - (Hz*cos(thetaH1) + Hx*sin(thetaH1) + Nz)*cos(thetaH2) + (Hz*cos(thetaH2) + Hx*sin(thetaH2) + Nz)*cos(thetaH2) - (Hx*cos(thetaH2) - Hz*sin(thetaH2) + Nx)*sin(thetaH2) - (T_fwd*cos(thetaR) + T_hor*sin(thetaR))*sin(thetaH2)]
    // [(Hx*cos(thetaH1) - Hz*sin(thetaH1) + Nx)*cos(thetaH2)*cos(thetaR) - (Hx*cos(thetaH2) - Hz*sin(thetaH2) + Nx)*cos(thetaH2) - (T_fwd*cos(thetaR) + T_hor*sin(thetaR))*cos(thetaH2) + (Hz*cos(thetaH1) + Hx*sin(thetaH1) + Nz)*sin(thetaH2) - (Hz*cos(thetaH2) + Hx*sin(thetaH2) + Nz)*sin(thetaH2)]
    
    AnkiAssert(HEAD_CAM_POSITION[1] == 0.f && NECK_JOINT_POSITION[1] == 0.f);
    Array<f32> R_geometry = Array<f32>(3,3,scratch);
    R_geometry[0][0] = cR;     R_geometry[0][1] = sH1*sR;             R_geometry[0][2] = cH1*sR;
    R_geometry[1][0] = -sH2*sR; R_geometry[1][1] = cR*sH1*sH2 + cH1*cH2;  R_geometry[1][2] = cH1*cR*sH2 - cH2*sH1;
    R_geometry[2][0] = -cH2*sR; R_geometry[2][1] = cH2*cR*sH1 - cH1*sH2;  R_geometry[2][2] = cH1*cH2*cR + sH1*sH2;
    
    const f32 term1 = (HEAD_CAM_POSITION[0]*cH1 - HEAD_CAM_POSITION[2]*sH1 + NECK_JOINT_POSITION[0]);
    const f32 term2 = (HEAD_CAM_POSITION[2]*cH1 + HEAD_CAM_POSITION[0]*sH1 + NECK_JOINT_POSITION[2]);
    const f32 term3 = (HEAD_CAM_POSITION[2]*cH2 + HEAD_CAM_POSITION[0]*sH2 + NECK_JOINT_POSITION[2]);
    const f32 term4 = (HEAD_CAM_POSITION[0]*cH2 - HEAD_CAM_POSITION[2]*sH2 + NECK_JOINT_POSITION[0]);
    const f32 term5 = (T_fwd_robot*cR + T_hor_robot*sR);
    
    Embedded::Point3<f32> T_geometry(T_hor_robot*cR + term1*sR - T_fwd_robot*sR,
                                     term1*cR*sH2 - term2*cH2 + term3*cH2 - term4*sH2 - term5*sH2,
                                     term1*cH2*cR - term4*cH2 - term5*cH2 + term2*sH2 - term3*sH2);
    
    Array<f32> R_blockRelHead = Array<f32>(3,3,scratch);
    _tracker.GetRotationMatrix(R_blockRelHead);
    const Embedded::Point3<f32>& T_blockRelHead = _tracker.GetTranslation();
    
    Array<f32> R_blockRelHead_new = Array<f32>(3,3,scratch);
    Embedded::Matrix::Multiply(R_geometry, R_blockRelHead, R_blockRelHead_new);
    
    Embedded::Point3<f32> T_blockRelHead_new = R_geometry*T_blockRelHead + T_geometry;
    
    if(_tracker.UpdateRotationAndTranslation(R_blockRelHead_new,
                                             T_blockRelHead_new,
                                             scratch) == RESULT_OK)
    {
      result = RESULT_OK;
    }
    
#endif // #if USE_MATLAB_TRACKER
    
#else
    const Quadrilateral<f32> sortedQuad  = currentQuad.ComputeClockwiseCorners<f32>();
    
    f32 dx = sortedQuad[3].x - sortedQuad[0].x;
    f32 dy = sortedQuad[3].y - sortedQuad[0].y;
    const f32 observedVerticalSize_pix = sqrtf( dx*dx + dy*dy );
    
    // Compare observed vertical size to actual block marker size (projected
    // to be orthogonal to optical axis, using head angle) to approximate the
    // distance to the marker along the camera's optical axis
    Radians theta_head = GetCurrentHeadAngle();
    const f32 cosHeadAngle = cosf(theta_head.ToFloat());
    const f32 sinHeadAngle = sinf(theta_head.ToFloat());
    const f32 d = (trackingMarkerWidth_mm* cosHeadAngle *
                   _headCamInfo->focalLength_y /
                   observedVerticalSize_pix);
    
    // Convert to how much we've moved along (and orthogonal to) the camera's optical axis
    const f32 T_fwd_cam =  T_fwd_robot*cosHeadAngle;
    const f32 T_ver_cam = -T_fwd_robot*sinHeadAngle;
    
    // Predict approximate horizontal shift from two things:
    // 1. The rotation of the robot
    //    Compute pixel-per-degree of the camera and multiply by degrees rotated
    // 2. Convert horizontal shift of the robot to pixel shift, using
    //    focal length
    f32 horizontalShift_pix = (static_cast<f32>(_headCamInfo->ncols/2) * theta_robot.ToFloat() /
                               _headCamFOV_hor) + (T_hor_robot*_headCamInfo->focalLength_x/d);
    
    // Predict approximate scale change by comparing the distance to the
    // object before and after forward motion
    const f32 scaleChange = d / (d - T_fwd_cam);
    
    // Predict approximate vertical shift in the camera plane by comparing
    // vertical motion (orthogonal to camera's optical axis) to the focal
    // length
    const f32 verticalShift_pix = T_ver_cam * _headCamInfo->focalLength_y/d;
    
    PRINT("Adjusting transformation: %.3fpix H shift for %.3fdeg rotation, "
          "%.3f scaling and %.3f V shift for %.3f translation forward (%.3f cam)\n",
          horizontalShift_pix, theta_robot.getDegrees(), scaleChange,
          verticalShift_pix, T_fwd_robot, T_fwd_cam);
    
    // Adjust the Transformation
    // Note: UpdateTransformation is doing *inverse* composition (thus using the negatives)
    if(_tracker.get_transformation().get_transformType() == Transformations::TRANSFORM_TRANSLATION) {
      Array<f32> update(1,2,scratch);
      update[0][0] = -horizontalShift_pix;
      update[0][1] = -verticalShift_pix;
      
#if USE_MATLAB_TRACKER
      MatlabVisionProcessor::UpdateTracker(update);
#else
      _tracker.UpdateTransformation(update, 1.f, scratch,
                                    Transformations::TRANSFORM_TRANSLATION);
#endif
    }
    else {
      // Inverse update we are composing is:
      //
      //                  [s 0 0]^(-1)     [0 0 h_shift]^(-1)
      //   updateMatrix = [0 s 0]       *  [0 0 v_shift]
      //                  [0 0 1]          [0 0    1   ]
      //
      //      [1/s  0  -h_shift/s]   [ update_0  update_1  update_2 ]
      //   =  [ 0  1/2 -v_shift/s] = [ update_3  update_4  update_5 ]
      //      [ 0   0      1     ]   [    0         0         1     ]
      //
      // Note: UpdateTransformation adds 1.0 to the diagonal scale terms
      Array<f32> update(1,6,scratch);
      update.Set(0.f);
      update[0][0] = 1.f/scaleChange - 1.f;               // first row, first col
      update[0][2] = -horizontalShift_pix/scaleChange;    // first row, last col
      update[0][4] = 1.f/scaleChange - 1.f;               // second row, second col
      update[0][5] = -verticalShift_pix/scaleChange;      // second row, last col
      
#if USE_MATLAB_TRACKER
      MatlabVisionProcessor::UpdateTracker(update);
#else
      _tracker.UpdateTransformation(update, 1.f, scratch,
                                    Transformations::TRANSFORM_AFFINE);
#endif
    } // if(tracker transformation type == TRANSLATION...)
    
#endif // if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
    
    // TODO: Re-enable tracker prediction viz on basestation
    //MatlabVisualization::SendTrackerPrediction_After(GetTrackerQuad(scratch));
    GetVizQuad(GetTrackerQuad(scratch), vizQuad);
    VizManager::getInstance()->DrawCameraQuad(9999, vizQuad, NamedColors::GREEN);
    
    return result;
  } // TrackerPredictionUpdate()
  
  void VisionSystem::FillDockErrMsg(const Embedded::Quadrilateral<f32>& currentQuad,
                                    MessageDockingErrorSignal& dockErrMsg,
                                    MemoryStack scratch)
  {
    dockErrMsg.isApproximate = false;
    
#if USE_APPROXIMATE_DOCKING_ERROR_SIGNAL
    dockErrMsg.isApproximate = true;
    
    const bool useTopBar = false; // TODO: pass in? make a docker parameter?
    const f32 focalLength_x = _headCamInfo->focalLength_x;
    const f32 imageResolutionWidth_pix = _detectionParameters.detectionWidth;
    
    Quadrilateral<f32> sortedQuad = currentQuad.ComputeClockwiseCorners<f32>();
    const Point<f32>& lineLeft  = (useTopBar ? sortedQuad[0] : sortedQuad[3]); // topLeft  or bottomLeft
    const Point<f32>& lineRight = (useTopBar ? sortedQuad[1] : sortedQuad[2]); // topRight or bottomRight
    
    AnkiAssert(lineRight.x > lineLeft.x);
    
    //L = sqrt(sum( (upperRight-upperLeft).^2) );
    const f32 lineDx = lineRight.x - lineLeft.x;
    const f32 lineDy = lineRight.y - lineLeft.y;
    const f32 lineLength = sqrtf(lineDx*lineDx + lineDy*lineDy);
    
    // Get the angle from vertical of the top or bottom bar of the marker
    //we're tracking
    
    //angleError = -asin( (upperRight(2)-upperLeft(2)) / L);
    //const f32 angleError = -asinf( (upperRight.y-upperLeft.y) / lineLength);
    const f32 angleError = -asinf( (lineRight.y-lineLeft.y) / lineLength) * 4;  // Multiply by scalar which makes angleError a little more accurate.  TODO: Something smarter than this.
    
    //currentDistance = BlockMarker3D.ReferenceWidth * this.calibration.fc(1) / L;
    const f32 distanceError = trackingMarkerWidth_mm * focalLength_x / lineLength;
    
    //ANS: now returning error in terms of camera. mainExecution converts to robot coords
    // //distError = currentDistance - CozmoDocker.LIFT_DISTANCE;
    // const f32 distanceError = currentDistance - cozmoLiftDistanceInMM;
    
    // TODO: should I be comparing to ncols/2 or calibration center?
    
    //midPointErr = -( (upperRight(1)+upperLeft(1))/2 - this.trackingResolution(1)/2 );
    f32 midpointError = ( (lineRight.x+lineLeft.x)/2 - imageResolutionWidth_pix/2 );
    
    //midPointErr = midPointErr * currentDistance / this.calibration.fc(1);
    midpointError *= distanceError / focalLength_x;
    
    // Go ahead and put the errors in the robot centric coordinates (other
    // than taking head angle into account)
    dockErrMsg.x_distErr = distanceError;
    dockErrMsg.y_horErr  = -midpointError;
    dockErrMsg.angleErr  = angleError;
    dockErrMsg.z_height  = -1.f; // unknown for approximate error signal
    
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
    
#if USE_MATLAB_TRACKER
    MatlabVisionProcessor::ComputeProjectiveDockingSignal(currentQuad,
                                                          dockErrMsg.x_distErr,
                                                          dockErrMsg.y_horErr,
                                                          dockErrMsg.z_height,
                                                          dockErrMsg.angleErr);
#else
    // Despite the names, fill the elements of the message with camera-centric coordinates
    dockErrMsg.x_distErr = _tracker.GetTranslation().x;
    dockErrMsg.y_horErr  = _tracker.GetTranslation().y;
    dockErrMsg.z_height  = _tracker.GetTranslation().z;
    
    dockErrMsg.angleErr  = _tracker.get_angleY();
    
#endif // if USE_MATLAB_TRACKER
    
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE || DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
    
#if USE_MATLAB_TRACKER
    MatlabVisionProcessor::ComputeProjectiveDockingSignal(currentQuad,
                                                          dockErrMsg.x_distErr,
                                                          dockErrMsg.y_horErr,
                                                          dockErrMsg.z_height,
                                                          dockErrMsg.angleErr);
#else
    
    // Compute the current pose of the block relative to the camera:
    Array<P3P_PRECISION> R = Array<P3P_PRECISION>(3,3, scratch);
    Point3<P3P_PRECISION> T;
    Quadrilateral<P3P_PRECISION> currentQuad_atPrecision(Point<P3P_PRECISION>(currentQuad[0].x, currentQuad[0].y),
                                                         Point<P3P_PRECISION>(currentQuad[1].x, currentQuad[1].y),
                                                         Point<P3P_PRECISION>(currentQuad[2].x, currentQuad[2].y),
                                                         Point<P3P_PRECISION>(currentQuad[3].x, currentQuad[3].y));
    
#warning broken
    /*P3P::computePose(currentQuad_atPrecision,
     _canonicalMarker3d[0], _canonicalMarker3d[1],
     _canonicalMarker3d[2], _canonicalMarker3d[3],
     _headCamInfo->focalLength_x, _headCamInfo->focalLength_y,
     _headCamInfo->center_x, _headCamInfo->center_y,
     R, T, scratch);*/
    
    // Extract what we need for the docking error signal from the block's pose:
    dockErrMsg.x_distErr = T.x;
    dockErrMsg.y_horErr  = T.y;
    dockErrMsg.z_height  = T.z;
    dockErrMsg.angleErr  = asinf(R[2][0]);
    
#endif // if USE_MATLAB_TRACKER
    
#endif // if USE_APPROXIMATE_DOCKING_ERROR_SIGNAL
  } // FillDockErrMsg()
  
#if 0
#pragma mark --- Public VisionSystem API Implementations ---
#endif
  
  u32 VisionSystem::DownsampleHelper(const Array<u8>& in,
                                     Array<u8>& out,
                                     MemoryStack scratch)
  {
    const s32 inWidth  = in.get_size(1);
    //const s32 inHeight = in.get_size(0);
    
    const s32 outWidth  = out.get_size(1);
    //const s32 outHeight = out.get_size(0);
    
    const u32 downsampleFactor = inWidth / outWidth;
    
    const u32 downsamplePower = Log2u32(downsampleFactor);
    
    if(downsamplePower > 0) {
      //PRINT("Downsampling [%d x %d] frame by %d.\n", inWidth, inHeight, (1 << downsamplePower));
      
      ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(in,
                                                         downsamplePower,
                                                         out,
                                                         scratch);
    } else {
      // No need to downsample, just copy the buffer
      out.Set(in);
    }
    
    return downsampleFactor;
  }
  
  /*
   const HAL::CameraInfo* VisionSystem::GetCameraCalibration() {
   // TODO: is just returning the pointer to HAL's camera info struct kosher?
   return _headCamInfo;
   }
   */
  
  f32 VisionSystem::GetTrackingMarkerWidth() {
    return _markerToTrack.width_mm;
  }
  
  f32 VisionSystem::GetVerticalFOV() {
    return _headCamFOV_ver;
  }
  
  f32 VisionSystem::GetHorizontalFOV() {
    return _headCamFOV_hor;
  }
  
  const FaceDetectionParameters& VisionSystem::GetFaceDetectionParams() {
    return _faceDetectionParameters;
  }
  
  std::string VisionSystem::GetCurrentModeName() const {
    return VisionSystem::GetModeName(static_cast<Mode>(_mode));
  }
  
  std::string VisionSystem::GetModeName(Mode mode) const
  {
    
    static const std::map<Mode, std::string> LUT = {
      {IDLE,                  "IDLE"}
      ,{LOOKING_FOR_MARKERS,  "LOOKING_FOR_MARKERS"}
      ,{TRACKING,             "TRACKING"}
      ,{DETECTING_FACES,      "DETECTING_FACES"}
      ,{TAKING_SNAPSHOT,      "TAKING_SNAPSHOT"}
      ,{LOOKING_FOR_SALIENCY, "LOOKING_FOR_SALIENCY"}
    };
    
    if(mode == 0) {
      return LUT.at(IDLE);
    } else {
      std::string retStr("");
      
      for(auto possibleMode : LUT) {
        if(possibleMode.first != IDLE &&
           mode & possibleMode.first)
        {
          if(!retStr.empty()) {
            retStr += "+";
          }
          retStr += possibleMode.second;
        }
      }
      
      return retStr;
    }
  } // GetModeName()
  
  VisionSystem::CameraInfo::CameraInfo(const Vision::CameraCalibration& camCalib)
  : focalLength_x(camCalib.GetFocalLength_x())
  , focalLength_y(camCalib.GetFocalLength_y())
  , center_x(camCalib.GetCenter_x())
  , center_y(camCalib.GetCenter_y())
  , skew(camCalib.GetSkew())
  , nrows(camCalib.GetNrows())
  , ncols(camCalib.GetNcols())
  {
    
    // TODO: Set distortion coefficients too
    
  }
  
  Result VisionSystem::Init(const Vision::CameraCalibration& camCalib)
  {
    Result result = RESULT_OK;
    
    bool gotNewCalibration = true;
    if(_isInitialized) {
      gotNewCalibration = (camCalib.GetFocalLength_x() != _headCamInfo->focalLength_x ||
                           camCalib.GetFocalLength_y() != _headCamInfo->focalLength_y ||
                           camCalib.GetCenter_x()      != _headCamInfo->center_x      ||
                           camCalib.GetCenter_y()      != _headCamInfo->center_y      ||
                           camCalib.GetSkew()          != _headCamInfo->skew          ||
                           camCalib.GetNrows()         != _headCamInfo->nrows         ||
                           camCalib.GetNcols()         != _headCamInfo->ncols);
    }
    
    if(gotNewCalibration) {
      bool calibSizeValid = false;
      switch(camCalib.GetNcols())
      {
        case 640:
          calibSizeValid = camCalib.GetNrows() == 480;
          _captureResolution = Vision::CAMERA_RES_VGA;
          break;
        case 400:
          calibSizeValid = camCalib.GetNrows() == 296;
          _captureResolution = Vision::CAMERA_RES_CVGA;
          break;
        case 320:
          calibSizeValid = camCalib.GetNrows() == 240;
          _captureResolution = Vision::CAMERA_RES_QVGA;
          break;
      }
      AnkiConditionalErrorAndReturnValue(calibSizeValid, RESULT_FAIL_INVALID_SIZE,
                                         "VisionSystem.InvalidCalibrationResolution",
                                         "Unexpected calibration resolution (%dx%d)\n",
                                         camCalib.GetNcols(), camCalib.GetNrows());
      
      // WARNING: the order of these initializations matter!
      
      //
      // Initialize the VisionSystem's state (i.e. its "private member variables")
      //
      
      _mode                      = LOOKING_FOR_MARKERS;
      _markerToTrack.Clear();
      _numTrackFailures          = 0;
      
      _wasCalledOnce             = false;
      _havePreviousRobotState    = false;
      
      //_headCamInfo = HAL::GetHeadCamInfo();
      if(_headCamInfo == nullptr) {
        delete _headCamInfo;
      }
      _headCamInfo = new CameraInfo(camCalib);
      if(_headCamInfo == nullptr) {
        PRINT_STREAM_INFO("VisionSystem.Init", "Initialize() - HeadCam Info pointer is NULL!");
        return RESULT_FAIL;
      }
      
      // Compute FOV from focal length (currently used for tracker prediciton)
      _headCamFOV_ver = 2.f * atanf(static_cast<f32>(_headCamInfo->nrows) /
                                    (2.f * _headCamInfo->focalLength_y));
      _headCamFOV_hor = 2.f * atanf(static_cast<f32>(_headCamInfo->ncols) /
                                    (2.f * _headCamInfo->focalLength_x));
      
      _exposureTime = 0.2f; // TODO: pick a reasonable start value
      _frameNumber = 0;
      
      // Just make all the vision parameters' resolutions match capture resolution:
      _detectionParameters.Initialize(_captureResolution);
      _trackerParameters.Initialize(_captureResolution);
      _faceDetectionParameters.Initialize(_captureResolution);
      
      Simulator::Initialize();
      
#ifdef RUN_SIMPLE_TRACKING_TEST
      Anki::Cozmo::VisionSystem::SetMarkerToTrack(Vision::MARKER_FIRE, DEFAULT_BLOCK_MARKER_WIDTH_MM);
#endif
      
      result = _memory.Initialize();
      if(result != RESULT_OK) { return result; }
      
      // TODO: Re-enable debugstream/MatlabViz on Basestation visionSystem
      /*
       result = DebugStream::Initialize();
       if(result != RESULT_OK) { return result; }
       
       result = MatlabVisualization::Initialize();
       if(result != RESULT_OK) { return result; }
       */
      
#if USE_MATLAB_TRACKER || USE_MATLAB_DETECTOR
      result = MatlabVisionProcessor::Initialize();
      if(result != RESULT_OK) { return result; }
#endif
      
      _RcamWrtRobot = Array<f32>(3,3,_memory._onchipScratch);
      
      _markerToTrack.Clear();
      _newMarkerToTrack.Clear();
      _newMarkerToTrackWasProvided = false;
      
      _isWaitingOnSnapShot = false;
      _isSnapshotReady = NULL;
      _snapshotROI = Embedded::Rectangle<s32>(-1, -1, -1, -1);
      _snapshot = NULL;
      
      _isInitialized = true;
    }
    
    return result;
  } // Init()
  
  
  Result VisionSystem::SetMarkerToTrack(const Vision::MarkerType& markerTypeToTrack,
                                        const f32 markerWidth_mm,
                                        const bool checkAngleX)
  {
    const Embedded::Point2f imageCenter(-1.f, -1.f);
    const f32     searchRadius = -1.f;
    return SetMarkerToTrack(markerTypeToTrack, markerWidth_mm,
                            imageCenter, searchRadius, checkAngleX);
  }
  
  Result VisionSystem::SetMarkerToTrack(const Vision::MarkerType& markerTypeToTrack,
                                        const f32 markerWidth_mm,
                                        const Embedded::Point2f& atImageCenter,
                                        const f32 imageSearchRadius,
                                        const bool checkAngleX)
  {
    _newMarkerToTrack.type              = markerTypeToTrack;
    _newMarkerToTrack.width_mm          = markerWidth_mm;
    _newMarkerToTrack.imageCenter       = atImageCenter;
    _newMarkerToTrack.imageSearchRadius = imageSearchRadius;
    _newMarkerToTrack.checkAngleX       = checkAngleX;
    
    // Next call to Update(), we will call UpdateMarkerToTrack() and
    // actually replace the current _markerToTrack with the one set here.
    _newMarkerToTrackWasProvided = true;
    
    return RESULT_OK;
  }
  
  
  
  const Embedded::FixedLengthList<Embedded::VisionMarker>& VisionSystem::GetObservedMarkerList()
  {
    return _memory._markers;
  } // GetObservedMarkerList()
  
  /*
   Result VisionSystem::GetVisionMarkerPoseNearestTo(const Embedded::Point3<f32>&  atPosition,
   const Vision::MarkerType&     withType,
   const f32                     maxDistance_mm,
   Embedded::Array<f32>&         rotationWrtRobot,
   Embedded::Point3<f32>&        translationWrtRobot,
   bool&                         markerFound)
   {
   using namespace Embedded;
   
   Result lastResult = RESULT_OK;
   markerFound = false;
   
   if(_memory._markers.get_size() > 0)
   {
   FixedLengthList<VisionMarker*> markersWithType(_memory._markers.get_size(),
   _memory._onchipScratch);
   
   AnkiConditionalErrorAndReturnValue(markersWithType.IsValid(),
   RESULT_FAIL_MEMORY,
   "GetVisionMarkerPoseNearestTo",
   "Failed to allocate markersWithType FixedLengthList.");
   
   // Find all markers with specified type
   s32 numFound = 0;
   VisionMarker  * restrict pMarker = _memory._markers.Pointer(0);
   VisionMarker* * restrict pMarkerWithType = markersWithType.Pointer(0);
   
   for(s32 i=0; i<_memory._markers.get_size(); ++i)
   {
   if(pMarker[i].markerType == withType) {
   pMarkerWithType[numFound++] = pMarker + i;
   }
   }
   markersWithType.set_size(numFound);
   
   // If any were found, find the one that is closest to the specified
   // 3D point and within the specified max distance
   if(numFound > 0) {
   // Create a little MemoryStack for allocating temporary
   // rotation matrix
   const s32 SCRATCH_BUFFER_SIZE = 128;
   char scratchBuffer[SCRATCH_BUFFER_SIZE];
   MemoryStack scratch(scratchBuffer, SCRATCH_BUFFER_SIZE);
   
   // Create temporary pose storage (wrt camera)
   Embedded::Point3<f32> translationWrtCamera;
   Array<f32> rotationWrtCamera(3,3,scratch);
   AnkiConditionalErrorAndReturnValue(rotationWrtCamera.IsValid(),
   RESULT_FAIL_MEMORY,
   "GetVisionMarkerPoseNearestTo",
   "Failed to allocate rotationWrtCamera Array.");
   
   VisionMarker* const* restrict pMarkerWithType = markersWithType.Pointer(0);
   
   f32 closestDistance = maxDistance_mm;
   
   for(s32 i=0; i<numFound; ++i) {
   // Compute this marker's pose WRT camera
   if((lastResult = GetVisionMarkerPose(*(pMarkerWithType[i]), true,
   rotationWrtCamera, translationWrtCamera)) != RESULT_OK) {
   return lastResult;
   }
   
   // Convert it to pose WRT robot
   if((lastResult = GetWithRespectToRobot(rotationWrtCamera, translationWrtCamera,
   rotationWrtRobot, translationWrtRobot)) != RESULT_OK) {
   return lastResult;
   }
   
   // See how far it is from the specified position
   const f32 currentDistance = (translationWrtRobot - atPosition).Length();
   if(currentDistance < closestDistance) {
   closestDistance = currentDistance;
   markerFound = true;
   }
   } // for each marker with type
   } // if numFound > 0
   } // if(VisionMemory::_markers.get_size() > 0)
   
   return RESULT_OK;
   } // GetVisionMarkerPoseNearestTo()
   
   template<typename PRECISION>
   static Result GetCamPoseWrtRobot(Array<PRECISION>& RcamWrtRobot,
   Embedded::Point3<PRECISION>& TcamWrtRobot)
   {
   AnkiConditionalErrorAndReturnValue(RcamWrtRobot.get_size(0)==3 &&
   RcamWrtRobot.get_size(1)==3,
   RESULT_FAIL_INVALID_SIZE,
   "VisionSystem::GetCamPoseWrtRobot",
   "Rotation matrix must already be 3x3.");
   
   const f32 headAngle = HeadController::GetAngleRad();
   const f32 cosH = cosf(headAngle);
   const f32 sinH = sinf(headAngle);
   
   RcamWrtRobot[0][0] = 0;  RcamWrtRobot[0][1] = sinH;  RcamWrtRobot[0][2] = cosH;
   RcamWrtRobot[1][0] = -1; RcamWrtRobot[1][1] = 0;     RcamWrtRobot[1][2] = 0;
   RcamWrtRobot[2][0] = 0;  RcamWrtRobot[2][1] = -cosH; RcamWrtRobot[2][2] = sinH;
   
   TcamWrtRobot.x = HEAD_CAM_POSITION[0]*cosH - HEAD_CAM_POSITION[2]*sinH + NECK_JOINT_POSITION[0];
   TcamWrtRobot.y = 0;
   TcamWrtRobot.z = HEAD_CAM_POSITION[2]*cosH + HEAD_CAM_POSITION[0]*sinH + NECK_JOINT_POSITION[2];
   
   return RESULT_OK;
   }
   
   Result GetWithRespectToRobot(const Embedded::Point3<f32>& pointWrtCamera,
   Embedded::Point3<f32>&       pointWrtRobot)
   {
   Point3<f32> TcamWrtRobot;
   
   Result lastResult;
   if((lastResult = GetCamPoseWrtRobot(_RcamWrtRobot, TcamWrtRobot)) != RESULT_OK) {
   return lastResult;
   }
   
   pointWrtRobot = _RcamWrtRobot*pointWrtCamera + TcamWrtRobot;
   
   return RESULT_OK;
   }
   
   Result GetWithRespectToRobot(const Embedded::Array<f32>&  rotationWrtCamera,
   const Embedded::Point3<f32>& translationWrtCamera,
   Embedded::Array<f32>&        rotationWrtRobot,
   Embedded::Point3<f32>&       translationWrtRobot)
   {
   Point3<f32> TcamWrtRobot;
   
   Result lastResult;
   if((lastResult = GetCamPoseWrtRobot(_RcamWrtRobot, TcamWrtRobot)) != RESULT_OK) {
   return lastResult;
   }
   
   if((lastResult = Matrix::Multiply(_RcamWrtRobot, rotationWrtCamera, rotationWrtRobot)) != RESULT_OK) {
   return lastResult;
   }
   
   translationWrtRobot = _RcamWrtRobot*translationWrtCamera + TcamWrtRobot;
   
   return RESULT_OK;
   }
   */
  Result VisionSystem::GetVisionMarkerPose(const Embedded::VisionMarker& marker,
                                           const bool ignoreOrientation,
                                           Embedded::Array<f32>&  rotation,
                                           Embedded::Point3<f32>& translation)
  {
    Embedded::Quadrilateral<f32> sortedQuad;
    if(ignoreOrientation) {
      sortedQuad = marker.corners.ComputeClockwiseCorners<f32>();
    } else {
      sortedQuad = marker.corners;
    }
    
    return P3P::computePose(sortedQuad,
                            _canonicalMarker3d[0], _canonicalMarker3d[1],
                            _canonicalMarker3d[2], _canonicalMarker3d[3],
                            _headCamInfo->focalLength_x, _headCamInfo->focalLength_y,
                            _headCamInfo->center_x, _headCamInfo->center_y,
                            rotation, translation);
  } // GetVisionMarkerPose()
  
  
  Result VisionSystem::TakeSnapshot(const Embedded::Rectangle<s32> roi, const s32 subsample,
                                    Embedded::Array<u8>& snapshot, bool& readyFlag)
  {
    if(!_isWaitingOnSnapShot)
    {
      _snapshotROI = roi;
      
      _snapshotSubsample = subsample;
      AnkiConditionalErrorAndReturnValue(_snapshotSubsample >= 1,
                                         RESULT_FAIL_INVALID_PARAMETER,
                                         "VisionSystem::TakeSnapshot()",
                                         "Subsample must be >= 1. %d was specified!\n", _snapshotSubsample);
      
      _snapshot = &snapshot;
      
      AnkiConditionalErrorAndReturnValue(_snapshot != NULL, RESULT_FAIL_INVALID_OBJECT,
                                         "VisionSystem::TakeSnapshot()", "NULL snapshot pointer!\n");
      
      AnkiConditionalErrorAndReturnValue(_snapshot->IsValid(),
                                         RESULT_FAIL_INVALID_OBJECT,
                                         "VisionSystem::TakeSnapshot()", "Invalid snapshot array!\n");
      
      const s32 nrowsSnap = _snapshot->get_size(0);
      const s32 ncolsSnap = _snapshot->get_size(1);
      
      AnkiConditionalErrorAndReturnValue(nrowsSnap*_snapshotSubsample == _snapshotROI.get_height() &&
                                         ncolsSnap*_snapshotSubsample == _snapshotROI.get_width(),
                                         RESULT_FAIL_INVALID_SIZE,
                                         "VisionSystem::TakeSnapshot()",
                                         "Snapshot ROI size (%dx%d) subsampled by %d doesn't match snapshot array size (%dx%d)!\n",
                                         _snapshotROI.get_height(), _snapshotROI.get_width(), _snapshotSubsample, nrowsSnap, ncolsSnap);
      
      _isSnapshotReady = &readyFlag;
      
      AnkiConditionalErrorAndReturnValue(_isSnapshotReady != NULL,
                                         RESULT_FAIL_INVALID_OBJECT,
                                         "VisionSystem::TakeSnapshot()",
                                         "NULL isSnapshotReady pointer!\n");
      
      _isWaitingOnSnapShot = true;
      
    } // if !_isWaitingOnSnapShot
    
    return RESULT_OK;
  } // TakeSnapshot()
  
  
  Result VisionSystem::TakeSnapshotHelper(const Embedded::Array<u8>& grayscaleImage)
  {
    if(_isWaitingOnSnapShot) {
      
      const s32 nrowsFull = grayscaleImage.get_size(0);
      const s32 ncolsFull = grayscaleImage.get_size(1);
      
      if(_snapshotROI.top    < 0 || _snapshotROI.top    >= nrowsFull-1 ||
         _snapshotROI.bottom < 0 || _snapshotROI.bottom >= nrowsFull-1 ||
         _snapshotROI.left   < 0 || _snapshotROI.left   >= ncolsFull-1 ||
         _snapshotROI.right  < 0 || _snapshotROI.right  >= ncolsFull-1)
      {
        PRINT_STREAM_INFO("VisionSystem.TakeSnapshotHelper", "VisionSystem::TakeSnapshotHelper(): Snapshot ROI out of bounds!");
        return RESULT_FAIL_INVALID_SIZE;
      }
      
      const s32 nrowsSnap = _snapshot->get_size(0);
      const s32 ncolsSnap = _snapshot->get_size(1);
      
      for(s32 iFull=_snapshotROI.top, iSnap=0;
          iFull<_snapshotROI.bottom && iSnap<nrowsSnap;
          iFull+= _snapshotSubsample, ++iSnap)
      {
        const u8 * restrict pImageRow = grayscaleImage.Pointer(iFull,0);
        u8 * restrict pSnapRow = _snapshot->Pointer(iSnap, 0);
        
        for(s32 jFull=_snapshotROI.left, jSnap=0;
            jFull<_snapshotROI.right && jSnap<ncolsSnap;
            jFull+= _snapshotSubsample, ++jSnap)
        {
          pSnapRow[jSnap] = pImageRow[jFull];
        }
      }
      
      _isWaitingOnSnapShot = false;
      *_isSnapshotReady = true;
      
    } // if _isWaitingOnSnapShot
    
    return RESULT_OK;
    
  } // TakeSnapshotHelper()
  
  
  
#if defined(SEND_IMAGE_ONLY)
#  error SEND_IMAGE_ONLY doesn't really make sense for Basestation vision system.
#elif defined(RUN_GROUND_TRUTHING_CAPTURE)
#  error RUN_GROUND_TRUTHING_CAPTURE not implemented in Basestation vision system.
#endif
  
  Result GetImageHelper(const Vision::Image& srcImage,
                      Array<u8>& destArray)
  {
    const s32 captureHeight = destArray.get_size(0);
    const s32 captureWidth  = destArray.get_size(1);
    
    if(srcImage.GetNumRows() != captureHeight || srcImage.GetNumCols() != captureWidth) {
      PRINT_NAMED_ERROR("VisionSystem.GetImageHelper.MismatchedImageSizes",
                        "Source Vision::Image and destination Embedded::Array should "
                        "be the same size (source is %dx%d and destinatinon is %dx%d\n",
                        srcImage.GetNumRows(), srcImage.GetNumCols(),
                        captureHeight, captureWidth);
      return RESULT_FAIL_INVALID_SIZE;
    }
    
    memcpy(reinterpret_cast<u8*>(destArray.get_buffer()),
           srcImage.GetDataPointer(),
           captureHeight*captureWidth*sizeof(u8));
    
    return RESULT_OK;
    
  } // GetImageHelper()

  
  // This is the regular Update() call
  Result VisionSystem::Update(const MessageRobotState robotState,
                              const Vision::Image&    inputImage)
  {
    Result lastResult = RESULT_OK;
    
    AnkiConditionalErrorAndReturnValue(IsInitialized(), RESULT_FAIL,
                                       "VisionSystem.Update", "VisionSystem not initialized.\n");
    
    _frameNumber++;
    
    // no-op on real hardware
    if(!Simulator::IsFrameReady()) {
      return RESULT_OK;
    }
    
    /* Not necessary on basestation
     // Make sure that we send the robot state message associated with the
     // image we are about to process.
     Messages::SendRobotStateMsg(&robotState);
     */
    
    UpdateRobotState(robotState);
    
    // prevent us from trying to update a tracker we just initialized in the same
    // frame
    bool trackerJustInitialzed = false;
    
    // If SetMarkerToTrack() was called by main() during previous Update(),
    // actually swap in the new marker now.
    lastResult = UpdateMarkerToTrack();
    AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                       "VisionSystem::Update()", "UpdateMarkerToTrack failed.\n");
    
    // Use the timestamp of passed-in robot state as our frame capture's
    // timestamp.  This is not totally correct, since the image will be
    // grabbed some (trivial?) number of cycles later, once we get to the
    // CameraGetFrame() calls below.  But this enforces, for now, that we
    // always send a RobotState message off to basestation with a matching
    // timestamp to every VisionMarker message.
    //const TimeStamp_t imageTimeStamp = HAL::GetTimeStamp();
    const TimeStamp_t imageTimeStamp = inputImage.GetTimestamp(); // robotState.timestamp;
    
    if(_mode & TAKING_SNAPSHOT) {
      // Nothing to do, unless a snapshot was requested
      
      if(_isWaitingOnSnapShot) {
        const s32 captureHeight = Vision::CameraResInfo[_captureResolution].height;
        const s32 captureWidth  = Vision::CameraResInfo[_captureResolution].width;
        
        
        Array<u8> grayscaleImage(captureHeight, captureWidth,
                                 _memory._offchipScratch, Flags::Buffer(false,false,false));
        
        GetImageHelper(inputImage, grayscaleImage);
        
        if((lastResult = TakeSnapshotHelper(grayscaleImage)) != RESULT_OK) {
          PRINT_STREAM_INFO("VisionSystem.Update", "TakeSnapshotHelper() failed.\n");
          return lastResult;
        }
      }
      
    } // if(_mode & TAKING_SNAPSHOT)
    
    
    if(_mode & LOOKING_FOR_MARKERS) {
      Simulator::SetDetectionReadyTime(); // no-op on real hardware
      
      _memory.ResetBuffers();
      
      //MemoryStack _offchipScratchlocal(VisionMemory::_offchipScratch);
      
      const s32 captureHeight = Vision::CameraResInfo[_captureResolution].height;
      const s32 captureWidth  = Vision::CameraResInfo[_captureResolution].width;
      
      Array<u8> grayscaleImage(captureHeight, captureWidth,
                               _memory._offchipScratch, Flags::Buffer(false,false,false));
      
      GetImageHelper(inputImage, grayscaleImage);
      
      if((lastResult = TakeSnapshotHelper(grayscaleImage)) != RESULT_OK) {
        PRINT_STREAM_INFO("VisionSystem.Update", "TakeSnapshotHelper() failed.\n");
        return lastResult;
      }
      
      BeginBenchmark("VisionSystem_CameraImagingPipeline");
      
      if(_vignettingCorrection == VignettingCorrection_Software) {
        BeginBenchmark("VisionSystem_CameraImagingPipeline_Vignetting");
        
        MemoryStack _onchipScratchlocal = _memory._onchipScratch;
        FixedLengthList<f32> polynomialParameters(5, _onchipScratchlocal, Flags::Buffer(false, false, true));
        
        for(s32 i=0; i<5; i++)
          polynomialParameters[i] = _vignettingCorrectionParameters[i];
        
        CorrectVignetting(grayscaleImage, polynomialParameters);
        
        EndBenchmark("VisionSystem_CameraImagingPipeline_Vignetting");
      } // if(_vignettingCorrection == VignettingCorrection_Software)
      
      if(_autoExposure_enabled && (_frameNumber % _autoExposure_adjustEveryNFrames) == 0) {
        BeginBenchmark("VisionSystem_CameraImagingPipeline_AutoExposure");
        
        ComputeBestCameraParameters(
                                    grayscaleImage,
                                    Embedded::Rectangle<s32>(0, grayscaleImage.get_size(1)-1, 0, grayscaleImage.get_size(0)-1),
                                    _autoExposure_integerCountsIncrement,
                                    _autoExposure_highValue,
                                    _autoExposure_percentileToMakeHigh,
                                    _autoExposure_minExposureTime,
                                    _autoExposure_maxExposureTime,
                                    _autoExposure_tooHighPercentMultiplier,
                                    _exposureTime,
                                    _memory._ccmScratch);
        
        EndBenchmark("VisionSystem_CameraImagingPipeline_AutoExposure");
      }
      
      // TODO: Provide a way to specify camera parameters from basestation
      //HAL::CameraSetParameters(_exposureTime, _vignettingCorrection == VignettingCorrection_CameraHardware);
      
      EndBenchmark("VisionSystem_CameraImagingPipeline");
      
      // TODO: Re-enable sending of images from basestation vision
      //DownsampleAndSendImage(grayscaleImage);
      
      if((lastResult = LookForMarkers(grayscaleImage,
                                      _detectionParameters,
                                      _memory._markers,
                                      _memory._ccmScratch,
                                      _memory._onchipScratch,
                                      _memory._offchipScratch)) != RESULT_OK)
      {
        return lastResult;
      }
      
      const s32 numMarkers = _memory._markers.get_size();
      
      bool isTrackingMarkerFound = false;
      for(s32 i_marker = 0; i_marker < numMarkers; ++i_marker)
      {
        const VisionMarker& crntMarker = _memory._markers[i_marker];
        
        // TODO: Let the robot do the ignoring of VisionMarkers in its mailbox
        //if (!Localization::IsOnRamp() && !IMUFilter::IsPickedUp()) {
        // Create a vision marker message and process it (which just queues it
        // in the mailbox to be picked up and sent out by main execution)
        {
          MessageVisionMarker msg;
          msg.timestamp  = imageTimeStamp;
          msg.markerType = crntMarker.markerType;
          
          msg.x_imgLowerLeft  = crntMarker.corners[Embedded::Quadrilateral<f32>::BottomLeft].x;
          msg.y_imgLowerLeft  = crntMarker.corners[Embedded::Quadrilateral<f32>::BottomLeft].y;
          
          msg.x_imgUpperLeft  = crntMarker.corners[Embedded::Quadrilateral<f32>::TopLeft].x;
          msg.y_imgUpperLeft  = crntMarker.corners[Embedded::Quadrilateral<f32>::TopLeft].y;
          
          msg.x_imgUpperRight = crntMarker.corners[Embedded::Quadrilateral<f32>::TopRight].x;
          msg.y_imgUpperRight = crntMarker.corners[Embedded::Quadrilateral<f32>::TopRight].y;
          
          msg.x_imgLowerRight = crntMarker.corners[Embedded::Quadrilateral<f32>::BottomRight].x;
          msg.y_imgLowerRight = crntMarker.corners[Embedded::Quadrilateral<f32>::BottomRight].y;
          
          //HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::VisionMarker),&msg);
          _visionMarkerMailbox.putMessage(msg);
          //}
        }
        
        // Was the desired marker found? If so, start tracking it -- if not already in tracking mode!
        if(!(_mode & TRACKING)          &&
           _markerToTrack.IsSpecified() &&
           !isTrackingMarkerFound       &&
           _markerToTrack.Matches(crntMarker))
        {
          // We will start tracking the _first_ marker of the right type that
          // we see.
          // TODO: Something smarter to track the one closest to the image center or to the expected location provided by the basestation?
          isTrackingMarkerFound = true;
          
          // I'd rather only initialize _trackingQuad if InitTemplate() succeeds, but
          // InitTemplate downsamples it for the time being, since we're still doing template
          // initialization at tracking resolution instead of the eventual goal of doing it at
          // full detection resolution.
          _trackingQuad = crntMarker.corners;
          
          // Normalize the image
          // NOTE: This will change grayscaleImage!
          if(_trackerParameters.normalizationFilterWidthFraction < 0.f) {
            // Faster: normalize using mean of quad
            lastResult = BrightnessNormalizeImage(grayscaleImage, _trackingQuad);
          } else {
            // Slower: normalize using local averages
            // NOTE: This is currently off-chip for memory reasons, so it's slow!
            lastResult = BrightnessNormalizeImage(grayscaleImage, _trackingQuad,
                                                  _trackerParameters.normalizationFilterWidthFraction,
                                                  _memory._offchipScratch);
          }
          
          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                             "VisionSystem::Update::BrightnessNormalizeImage",
                                             "BrightnessNormalizeImage failed.\n");
          
          if((lastResult = InitTemplate(grayscaleImage,
                                        _trackingQuad,
                                        _trackerParameters,
                                        _tracker,
                                        _memory._ccmScratch,
                                        _memory._onchipScratch, //< NOTE: onchip is a reference
                                        _memory._offchipScratch)) != RESULT_OK)
          {
            return lastResult;
          }
          
          trackerJustInitialzed = true;
          
          // store the current mode so we can put it back when done tracking
          _modeBeforeTracking = _mode;
          
          // Template initialization succeeded, switch to tracking mode:
          // TODO: Log or issue message?
          // NOTE: this disables any other modes so we are *only* tracking
          _mode = IDLE;
          EnableModeHelper(TRACKING);
          
        } // if(isTrackingMarkerSpecified && !isTrackingMarkerFound && markerType == markerToTrack)
      } // for(each marker)
    } // if(_mode & LOOKING_FOR_MARKERS)
    
    
    if(_mode & TRACKING) {
      Simulator::SetTrackingReadyTime(); // no-op on real hardware
      
      //
      // Capture image for tracking
      //
      bool converged = false;
      if(!trackerJustInitialzed)
      {
        MemoryStack _offchipScratchlocal(_memory._offchipScratch);
        MemoryStack _onchipScratchlocal(_memory._onchipScratch);
        
        const s32 captureHeight = Vision::CameraResInfo[_captureResolution].height;
        const s32 captureWidth  = Vision::CameraResInfo[_captureResolution].width;
        
        Array<u8> grayscaleImage(captureHeight, captureWidth,
                                 _onchipScratchlocal, Flags::Buffer(false,false,false));
        
        GetImageHelper(inputImage, grayscaleImage);
        //memcpy(reinterpret_cast<u8*>(grayscaleImage.get_buffer()), inputImage, captureWidth*captureHeight*sizeof(u8));
        //HAL::CameraGetFrame(),
        //  _captureResolution, false);
        
        if((lastResult = TakeSnapshotHelper(grayscaleImage)) != RESULT_OK) {
          PRINT_STREAM_INFO("VisionSystem.Update", "TakeSnapshotHelper() failed.\n");
          return lastResult;
        }
        
        BeginBenchmark("VisionSystem_CameraImagingPipeline");
        
        if(_vignettingCorrection == VignettingCorrection_Software) {
          BeginBenchmark("VisionSystem_CameraImagingPipeline_Vignetting");
          
          MemoryStack _onchipScratchlocal = _memory._onchipScratch;
          FixedLengthList<f32> polynomialParameters(5, _onchipScratchlocal, Flags::Buffer(false, false, true));
          
          for(s32 i=0; i<5; i++)
            polynomialParameters[i] = _vignettingCorrectionParameters[i];
          
          CorrectVignetting(grayscaleImage, polynomialParameters);
          
          EndBenchmark("VisionSystem_CameraImagingPipeline_Vignetting");
        } // if(_vignettingCorrection == VignettingCorrection_Software)
        
        // TODO: allow tracking to work with exposure changes
        /*if(_autoExposure_enabled && (_frameNumber % _autoExposure_adjustEveryNFrames) == 0) {
         BeginBenchmark("VisionSystem_CameraImagingPipeline_AutoExposure");
         
         ComputeBestCameraParameters(
         grayscaleImage,
         Rectangle<s32>(0, grayscaleImage.get_size(1)-1, 0, grayscaleImage.get_size(0)-1),
         _autoExposure_integerCountsIncrement,
         _autoExposure_highValue,
         _autoExposure_percentileToMakeHigh,
         _autoExposure_minExposureTime, _autoExposure_maxExposureTime,
         _autoExposure_tooHighPercentMultiplier,
         _exposureTime,
         VisionMemory::_ccmScratch);
         
         EndBenchmark("VisionSystem_CameraImagingPipeline_AutoExposure");
         }*/
        
        EndBenchmark("VisionSystem_CameraImagingPipeline");
        
        // TODO: Re-enable setting camera parameters from basestation vision
        //HAL::CameraSetParameters(_exposureTime, _vignettingCorrection == VignettingCorrection_CameraHardware);
        
        // TODO: Re-enable sending of images from basestation vision
        //DownsampleAndSendImage(grayscaleImage);
        
        // Normalize the image
        // NOTE: This will change grayscaleImage!
        if(_trackerParameters.normalizationFilterWidthFraction < 0.f) {
          // Faster: normalize using mean of quad
          lastResult = BrightnessNormalizeImage(grayscaleImage, _trackingQuad);
        } else {
          // Slower: normalize using local averages
          // NOTE: This is currently off-chip for memory reasons, so it's slow!
          lastResult = BrightnessNormalizeImage(grayscaleImage, _trackingQuad,
                                                _trackerParameters.normalizationFilterWidthFraction,
                                                _memory._offchipScratch);
        }
        
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "VisionSystem::Update::BrightnessNormalizeImage",
                                           "BrightnessNormalizeImage failed.\n");
        
        //
        // Tracker Prediction
        //
        // Adjust the tracker transformation by approximately how much we
        // think we've moved since the last tracking call.
        //
        
        if((lastResult =TrackerPredictionUpdate(grayscaleImage, _onchipScratchlocal)) != RESULT_OK) {
          PRINT_STREAM_INFO("VisionSystem.Update", " TrackTemplate() failed.\n");
          return lastResult;
        }
        
        //
        // Update the tracker transformation using this image
        //
        
        // Set by TrackTemplate() call
        if((lastResult = TrackTemplate(grayscaleImage,
                                       _trackingQuad,
                                       _trackerParameters,
                                       _tracker,
                                       converged,
                                       _memory._ccmScratch,
                                       _onchipScratchlocal,
                                       _offchipScratchlocal)) != RESULT_OK) {
          PRINT_STREAM_INFO("VisionSystem.Update", "TrackTemplate() failed.\n");
          return lastResult;
        }
      } else {
        converged = true;
      } // if(!trackerJustInitialzed)
      
      //
      // Create docking error signal from tracker
      //
      
      MessageDockingErrorSignal dockErrMsg;
      dockErrMsg.timestamp = imageTimeStamp;
      dockErrMsg.didTrackingSucceed = static_cast<u8>(converged);
      
      if(converged)
      {
        Embedded::Quadrilateral<f32> currentQuad = GetTrackerQuad(_memory._onchipScratch);
        FillDockErrMsg(currentQuad, dockErrMsg, _memory._onchipScratch);
        
        // Send tracker quad if image streaming
        if (_imageSendMode == ISM_STREAM) {
          f32 scale = 1.f;
          for (u8 s = (u8)Vision::CAMERA_RES_CVGA; s<(u8)_nextSendImageResolution; ++s) {
            scale *= 0.5f;
          }
          
          MessageTrackerQuad m;
          m.topLeft_x     = static_cast<u16>(currentQuad[Embedded::Quadrilateral<f32>::TopLeft].x * scale);
          m.topLeft_y     = static_cast<u16>(currentQuad[Embedded::Quadrilateral<f32>::TopLeft].y * scale);
          m.topRight_x    = static_cast<u16>(currentQuad[Embedded::Quadrilateral<f32>::TopRight].x * scale);
          m.topRight_y    = static_cast<u16>(currentQuad[Embedded::Quadrilateral<f32>::TopRight].y * scale);
          m.bottomRight_x = static_cast<u16>(currentQuad[Embedded::Quadrilateral<f32>::BottomRight].x * scale);
          m.bottomRight_y = static_cast<u16>(currentQuad[Embedded::Quadrilateral<f32>::BottomRight].y * scale);
          m.bottomLeft_x  = static_cast<u16>(currentQuad[Embedded::Quadrilateral<f32>::BottomLeft].x * scale);
          m.bottomLeft_y  = static_cast<u16>(currentQuad[Embedded::Quadrilateral<f32>::BottomLeft].y * scale);
          
          //HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::TrackerQuad), &m);
          _trackerMailbox.putMessage(m);
        }
        
        // Reset the failure counter
        _numTrackFailures = 0;
        
        _dockingMailbox.putMessage(dockErrMsg);
      }
      else {
        _numTrackFailures += 1;
        
        if(_numTrackFailures == MAX_TRACKING_FAILURES)
        {          
          PRINT_NAMED_INFO("VisionSystem.Update", "Reached max number of tracking "
                           "failures (%d). Switching back to looking for markers.\n",
                           MAX_TRACKING_FAILURES);
          
          // This resets docking, puttings us back in VISION_MODE_LOOKING_FOR_MARKERS mode
          SetMarkerToTrack(_markerToTrack.type,
                           _markerToTrack.width_mm,
                           _markerToTrack.imageCenter,
                           _markerToTrack.imageSearchRadius,
                           _markerToTrack.checkAngleX);
        }
      }
      
      //Messages::ProcessDockingErrorSignalMessage(dockErrMsg);
      
      
    } // if(_mode & TRACKING)
    
    
    if(_mode & DETECTING_FACES) {
      Simulator::SetFaceDetectionReadyTime();
      
      static cv::CascadeClassifier _faceCascade;
      if(_faceCascade.empty()) {
        const std::string cascadeFilename = PREPEND_SCOPED_PATH(Config, "haarcascade_frontalface_alt2.xml");
        const bool loadResult = _faceCascade.load(cascadeFilename);
        AnkiConditionalError(loadResult == true, "VisionSystem.Update.LoadFaceCascade",
                             "Failed to load face cascade from %s\n", cascadeFilename.c_str());
      }
      
      std::vector<cv::Rect> faces;
      
      cv::Mat equalizedImage;
      cv::equalizeHist(inputImage.get_CvMat_(), equalizedImage);
      
      _faceCascade.detectMultiScale(equalizedImage, faces,
                                    1.1, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(30, 30));
      
      for(auto & currentFace : faces)
      {
        MessageFaceDetection msg;
        msg.width       = static_cast<u16>(currentFace.width);
        msg.height      = static_cast<u16>(currentFace.height);
        msg.x_upperLeft = static_cast<u16>(currentFace.x);
        msg.y_upperLeft = static_cast<u16>(currentFace.y);
        msg.visualize   = static_cast<u8>(true);
        
        _faceDetectMailbox.putMessage(msg);
      } // for each face detection
      
    } // if(_mode & DETECTING_FACES)
    
    // DEBUG!!!!
    //_mode |= LOOKING_FOR_SALIENCY;
    
    if(_mode & LOOKING_FOR_SALIENCY)
    {
      const bool headSame =  NEAR(_robotState.headAngle, _prevRobotState.headAngle, DEG_TO_RAD(1));
      const bool poseSame = (NEAR(_robotState.pose_x,    _prevRobotState.pose_x,    1.f) &&
                             NEAR(_robotState.pose_y,    _prevRobotState.pose_y,    1.f) &&
                             NEAR(_robotState.pose_angle,_prevRobotState.pose_angle, DEG_TO_RAD(1)));
      
      //PRINT_STREAM_INFO("pose_angle diff = %.1f\n", RAD_TO_DEG(std::abs(_robotState.pose_angle - _prevRobotState.pose_angle)));
      
      if(headSame && poseSame && !_prevImage.IsEmpty()) {
        /*
        Vision::Image diffImage(*inputImage);
        diffImage -= _prevImage;
        diffImage.Abs();
        */
#       if ANKI_COZMO_USE_MATLAB_VISION
        //_matlab.PutOpencvMat(diffImage.get_CvMat_(), "diffImage");
        //_matlab.EvalString("imagesc(diffImage), axis image, drawnow");
        
        _matlab.PutOpencvMat(inputImage->get_CvMat_(), "inputImage");
        _matlab.PutOpencvMat(_prevImage.get_CvMat_(), "prevImage");
        
        
        _matlab.EvalString("[nrows,ncols,~] = size(inputImage); "
                           "[xgrid,ygrid] = meshgrid(1:ncols,1:nrows); "
                           "logImg1 = log(max(1,double(prevImage))); "
                           "logImg2 = log(max(1,double(inputImage))); "
                           "diff = imabsdiff(logImg1, logImg2); "
                           "diffThresh = diff > log(1.5); "
                           "if any(diffThresh(:)), "
                           "  diff(~diffThresh) = 0; "
                           "  sumDiff = sum(diff(:)); "
                           "  x = sum(xgrid(:).*diff(:))/sumDiff; "
                           "  y = sum(ygrid(:).*diff(:))/sumDiff; "
                           "  centroid = [x y]; "
/*                           "stats = regionprops(diffThresh, 'Area', 'Centroid', 'PixelIdxList'); "
                           "areas = [stats.Area]; "
                           "keep = areas > .01*nrows*ncols & areas < .5*nrows*ncols; "
                           "diffThresh(vertcat(stats(~keep).PixelIdxList)) = false; "
                           "keep = find(keep); "
                           "if ~isempty(keep), "
                           "  [~,toTrack] = max(areas(keep)); "
                           "  centroid = stats(keep(toTrack)).Centroid; "
  */                         "else, "
                           "  clear centroid; "
                           "end");
        
        if(_matlab.DoesVariableExist("centroid")) {
          
          _matlab.EvalString("hold off, imagesc(diff), axis image, colormap(gray), "
                             "title(%d), "
                             "hold on, plot(centroid(1), centroid(2), 'go', 'MarkerSize', 10, 'LineWidth', 2); drawnow",
                             inputImage->GetTimestamp());
          
          mxArray* mxCentroid = _matlab.GetArray("centroid");
          
          CORETECH_ASSERT(mxGetNumberOfElements(mxCentroid) == 2);
          const f32 xCen = mxGetPr(mxCentroid)[0];
          const f32 yCen = mxGetPr(mxCentroid)[1];
          
          MessagePanAndTiltHead msg;
          
          // Convert image positions to desired angles
          const f32 yError_pix = static_cast<f32>(inputImage->GetNumRows())*0.5f - yCen;
          msg.relativeHeadTiltAngle_rad = atan_fast(yError_pix / _headCamInfo->focalLength_y);
          
          const f32 xError_pix = static_cast<f32>(inputImage->GetNumCols())*0.5f - xCen;
          msg.relativePanAngle_rad = atan_fast(xError_pix / _headCamInfo->focalLength_x);
          
          _panTiltMailbox.putMessage(msg);
        }
        
        _matlab.EvalString("imagesc(imabsdiff(inputImage, prevImage)), axis image, colormap(gray), drawnow");
        _matlab.EvalString("title(%d)", inputImage->GetTimestamp());
#       endif

        
      } // if(headSame && poseSame)
      
      // Store a copy of the current image for next time
      // TODO: switch to just swapping pointers between current and previous image
      inputImage.CopyDataTo(_prevImage);
      _prevImage.SetTimestamp(inputImage.GetTimestamp());
      
    } // if(_mode & LOOKING_FOR_SALIENCY)
    
    return lastResult;
  } // Update() [Real]
  
  
  void VisionSystem::SetParams(const bool autoExposureOn,
                     const f32 exposureTime,
                     const s32 integerCountsIncrement,
                     const f32 minExposureTime,
                     const f32 maxExposureTime,
                     const u8 highValue,
                     const f32 percentileToMakeHigh)
  {
    _autoExposure_enabled = autoExposureOn;
    _exposureTime = exposureTime;
    _autoExposure_integerCountsIncrement = integerCountsIncrement;
    _autoExposure_minExposureTime = minExposureTime;
    _autoExposure_maxExposureTime = maxExposureTime;
    _autoExposure_highValue = highValue;
    _autoExposure_percentileToMakeHigh = percentileToMakeHigh;
    
    PRINT_NAMED_INFO("VisionSystem.SetParams", "Changed VisionSystem params: autoExposureOn %d exposureTime %f integerCountsInc %d, minExpTime %f, maxExpTime %f, highVal %d, percToMakeHigh %f\n",
               _autoExposure_enabled,
               _exposureTime,
               _autoExposure_integerCountsIncrement,
               _autoExposure_minExposureTime,
               _autoExposure_maxExposureTime,
               _autoExposure_highValue,
               _autoExposure_percentileToMakeHigh);
  }
  
  void VisionSystem::SetFaceDetectParams(const f32 scaleFactor,
                                         const s32 minNeighbors,
                                         const s32 minObjectHeight,
                                         const s32 minObjectWidth,
                                         const s32 maxObjectHeight,
                                         const s32 maxObjectWidth)
  {
    PRINT_STREAM_INFO("VisionSystem::SetFaceDetectParams", "Updated VisionSystem FaceDetect params");
    _faceDetectionParameters.scaleFactor = scaleFactor;
    _faceDetectionParameters.minNeighbors = minNeighbors;
    _faceDetectionParameters.minHeight = minObjectHeight;
    _faceDetectionParameters.minWidth = minObjectWidth;
    _faceDetectionParameters.maxHeight = maxObjectHeight;
    _faceDetectionParameters.maxWidth = maxObjectWidth;
  }
    
} // namespace Cozmo
} // namespace Anki
