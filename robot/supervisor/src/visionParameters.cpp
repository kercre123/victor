/**
 * File: visionParameters.cpp
 *
 * Author: Andrew Stein
 * Date:   3/28/2014
 *
 * Description: High-level vision system parameter definitions, including the
 *              type of tracker to use.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "visionParameters.h"

#include "anki/common/robot/utilities.h"

#include "anki/cozmo/robot/visionSystem.h"

#if 0
#pragma mark --- DetectFiducialMarkersParameters ---
#endif

namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
      
      using namespace Embedded;
      
      
#if 0
#pragma mark --- DetectorParameters ---
#endif

      
      DetectFiducialMarkersParameters::DetectFiducialMarkersParameters()
      : isInitialized(false)
      {
      
      }
        
      void DetectFiducialMarkersParameters::Initialize()
      {
        detectionResolution = HAL::CAMERA_MODE_QVGA;
        detectionWidth  = CameraModeInfo[detectionResolution].width;
        detectionHeight = CameraModeInfo[detectionResolution].height;
        
        scaleImage_thresholdMultiplier = 65536; // 1.0*(2^16)=65536
        scaleImage_numPyramidLevels = 3;
        
        component1d_minComponentWidth = 0;
        component1d_maxSkipDistance = 0;
        
        minSideLength = 0.03f*static_cast<f32>(MAX(detectionWidth,detectionHeight));
        maxSideLength = 0.97f*static_cast<f32>(MIN(detectionWidth,detectionHeight));
        
        component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
        component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
        component_sparseMultiplyThreshold = 1000 << 5;
        component_solidMultiplyThreshold = 2 << 5;
        
        component_minHollowRatio = 1.0f;
        
        maxExtractedQuads = 1000/2;
        quads_minQuadArea = 100/4;
        quads_quadSymmetryThreshold = 512; // ANS: corresponds to 2.0, loosened from 384 (1.5), for large mat markers at extreme perspective distortion
        quads_minDistanceFromImageEdge = 2;
        
        decode_minContrastRatio = 1.25;
        
        maxConnectedComponentSegments = 39000; // 322*240/2 = 38640
        
        isInitialized = true;
      } // DetectFiducialMarkersParameters::Initialize()

    
    
    
#if 0
#pragma mark --- TrackerParameters ---
#endif
      
      TrackerParameters::TrackerParameters()
      : isInitialized(false)
      {
        
      }
      
      void TrackerParameters::Initialize()
      {
#if DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
        // Binary tracker works at QVGA (unlike LK)
        trackingResolution = HAL::CAMERA_MODE_QVGA;
        
        trackingImageWidth  = CameraModeInfo[trackingResolution].width;
        trackingImageHeight = CameraModeInfo[trackingResolution].height;
        scaleTemplateRegionPercent = 1.1f;
        //edgeDetection_grayvalueThreshold    = 128;
        edgeDetection_threshold_yIncrement = 4;
        edgeDetection_threshold_xIncrement = 4;
        edgeDetection_threshold_blackPercentile = 0.20f;
        edgeDetection_threshold_whitePercentile = 0.80f;
        edgeDetection_threshold_scaleRegionPercent = 0.8f;
        edgeDetection_minComponentWidth     = 2;
        edgeDetection_maxDetectionsPerType  = 2500;
        edgeDetection_everyNLines           = 1;
        matching_maxTranslationDistance     = 7;
        matching_maxProjectiveDistance      = 7;
        verification_maxTranslationDistance = 2;
        percentMatchedPixelsThreshold       = 0.02f; // TODO: pick a reasonable value
        
#else
        // LK tracker parameter initialization
        
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE
        trackingResolution   = HAL::CAMERA_MODE_QVGA; // 320x240
        numPyramidLevels     = 4;
#else
        //trackingResolution   = HAL::CAMERA_MODE_QQQVGA; // 80x60
        trackingResolution   = HAL::CAMERA_MODE_QQVGA; // 160x120        
        numPyramidLevels     = 3;
#endif // DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE
        
        trackingImageWidth   = CameraModeInfo[trackingResolution].width;
        trackingImageHeight  = CameraModeInfo[trackingResolution].height;
        scaleTemplateRegionPercent = 1.1f;
        
        maxIterations             = 25;
        convergenceTolerance      = 1.f;
        verify_maxPixelDifference = 30;
        useWeights                = true;
        maxSamplesAtBaseLevel     = 500; // NOTE: used by all Matlab trackers & "SAMPLED_PROJECTIVE"
        
        
#endif // if DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
        isInitialized = true;
      }
      
      

    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki


