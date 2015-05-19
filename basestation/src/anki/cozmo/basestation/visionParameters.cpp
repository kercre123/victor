/**
 * File: visionParameters.cpp [Basestation]
 *
 * Author: Andrew Stein
 * Date:   3/28/2014
 *
 * Description: High-level vision system parameter definitions, including the
 *              type of tracker to use.
 *
 * Copyright: Anki, Inc. 2014
 **/


#include "anki/common/robot/utilities.h"

#include "anki/vision/robot/fiducialDetection.h" // just for FIDUCIAL_SQUARE_WIDTH_FRACTION

#include "anki/cozmo/basestation/visionParameters.h"

namespace Anki {
  namespace Cozmo {
      
      using namespace Embedded;
      
      
#if 0
#pragma mark --- DetectFiducialMarkersParameters ---
#endif

      
      DetectFiducialMarkersParameters::DetectFiducialMarkersParameters()
      : isInitialized(false)
      {
      
      }
        
      void DetectFiducialMarkersParameters::Initialize(Vision::CameraResolution resolution)
      {
        detectionResolution = resolution;
        detectionWidth  = Vision::CameraResInfo[detectionResolution].width;
        detectionHeight = Vision::CameraResInfo[detectionResolution].height;

#ifdef SIMULATOR
        scaleImage_thresholdMultiplier = 32768; // 0.5*(2^16)=32768
#else
        scaleImage_thresholdMultiplier = 65536; // 1.0*(2^16)=65536
        //scaleImage_thresholdMultiplier = 49152; // 0.75*(2^16)=49152
#endif
        scaleImage_numPyramidLevels = 3;
        
        component1d_minComponentWidth = 0;
        component1d_maxSkipDistance = 0;
        
        minSideLength = 0.03f*static_cast<f32>(MAX(detectionWidth,detectionHeight));
        maxSideLength = 0.97f*static_cast<f32>(MIN(detectionWidth,detectionHeight));
        
        component_minimumNumPixels = Round<s32>(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength));
        component_maximumNumPixels = Round<s32>(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength));
        component_sparseMultiplyThreshold = 1000 << 5;
        component_solidMultiplyThreshold = 2 << 5;
        
        component_minHollowRatio = 1.0f;
        
        // Ratio of 4th to 5th biggest Laplacian peak must be greater than this
        // for a quad to be extracted from a connected component
        minLaplacianPeakRatio = 5;
        
        maxExtractedQuads = 1000/2;
        quads_minQuadArea = 100/4;
        quads_quadSymmetryThreshold = 512; // ANS: corresponds to 2.0, loosened from 384 (1.5), for large mat markers at extreme perspective distortion
        quads_minDistanceFromImageEdge = 2;
        
        decode_minContrastRatio = 1.25;
        
        maxConnectedComponentSegments = 39000; // 322*240/2 = 38640
        
        // TODO: Benchmark quad refinement so we can enable this by default
        quadRefinementIterations = 25;
        
        // TODO: Could this be fewer samples?
        numRefinementSamples = 100;
        
        // If quad refinment moves any corner by more than this (in pixels), the
        // original quad/homography are restored.
        quadRefinementMaxCornerChange = 5.f;
        
        // If quad refinement moves all corners by less than this (in pixels),
        // the refinment is considered converged and stops immediately
        quadRefinementMinCornerChange = 0.005f;
        
        // Return unknown/unverified markers (e.g. for display)
        keepUnverifiedMarkers = false;
        
        isInitialized = true;
      } // DetectFiducialMarkersParameters::Initialize()

    
    
    
#if 0
#pragma mark --- TrackerParameters ---
#endif
      
      
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
      const f32 TrackerParameters::MIN_TRACKER_DISTANCE = 10.f;
      const f32 TrackerParameters::MAX_TRACKER_DISTANCE = 200.f;
      const f32 TrackerParameters::MAX_BLOCK_DOCKING_ANGLE = DEG_TO_RAD(45);
      const f32 TrackerParameters::MAX_DOCKING_FOV_ANGLE = DEG_TO_RAD(60);
#endif
      
      TrackerParameters::TrackerParameters()
      : isInitialized(false)
      {
        
      }
      
      void TrackerParameters::Initialize(Vision::CameraResolution resolution)
      {
        // This is size of the box filter used to locally normalize the image
        // as a fraction of the size of the current tracking quad.
        // Set to zero to disable normalization.
        // Set to a negative value to simply use (much faster) mean normalization,
        // which just makes the mean of the pixels within the tracking quad equal 128.
        normalizationFilterWidthFraction = -1.f; //0.5f;
        
#if DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
        // Binary tracker works at QVGA (unlike LK)
        trackingResolution = Vision::CAMERA_RES_QVGA;
        
        trackingImageWidth  = Vision::CameraResInfo[trackingResolution].width;
        trackingImageHeight = Vision::CameraResInfo[trackingResolution].height;
        scaleTemplateRegionPercent = 1.1f;
        
        
        edgeDetectionParams_template = TemplateTracker::BinaryTracker::EdgeDetectionParameters(
          TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE,
          4,    // s32 threshold_yIncrement; //< How many pixels to use in the y direction (4 is a good value?)
          4,    // s32 threshold_xIncrement; //< How many pixels to use in the x direction (4 is a good value?)
          0.1f, // f32 threshold_blackPercentile; //< What percentile of histogram energy is black? (.1 is a good value)
          0.9f, // f32 threshold_whitePercentile; //< What percentile of histogram energy is white? (.9 is a good value)
          0.8f, // f32 threshold_scaleRegionPercent; //< How much to scale template bounding box (.8 is a good value)
          2,    // s32 minComponentWidth; //< The smallest horizontal size of a component (1 to 4 is good)
          500,  // s32 maxDetectionsPerType; //< As many as you have memory and time for (500 is good)
          1,    // s32 combHalfWidth; //< How far apart to compute the derivative difference (1 is good)
          10,   // s32 combResponseThreshold; //< The minimum absolute-value response to start an edge component (20 is good)
          1     // s32 everyNLines; //< As many as you have time for
        );
        
        edgeDetectionParams_update = edgeDetectionParams_template;
        edgeDetectionParams_update.maxDetectionsPerType = 2500;
          
        matching_maxTranslationDistance     = 7;
        matching_maxProjectiveDistance      = 7;
        verify_maxTranslationDistance = 2;
        verify_maxPixelDifference = 50;
        verify_coordinateIncrement = 3;
        percentMatchedPixelsThreshold       = 0.02f; // TODO: pick a reasonable value
        
#else
        // LK tracker parameter initialization
        
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
        trackingResolution   = resolution;
        numPyramidLevels     = 3; // TODO: Compute from resolution to get down to a given size?
#else
        //trackingResolution   = Vision::CAMERA_RES_QQQVGA; // 80x60
        trackingResolution   = Vision::CAMERA_RES_QQVGA; // 160x120
        numPyramidLevels     = 3;
#endif // DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE
        
        trackingImageWidth   = Vision::CameraResInfo[trackingResolution].width;
        trackingImageHeight  = Vision::CameraResInfo[trackingResolution].height;
        
        maxIterations             = 50;
        verify_maxPixelDifference = 30;
        useWeights                = true;
       
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
        convergenceTolerance_angle    = DEG_TO_RAD(0.05);
        convergenceTolerance_distance = 0.05f; // mm
        
        numSamplingRegions            = 5;
        
        // Split total samples between fiducial and interior
        numInteriorSamples            = 500;
        numFiducialEdgeSamples        = 500;
        
        if(numFiducialEdgeSamples > 0) {
          scaleTemplateRegionPercent    = 1.f - FIDUCIAL_SQUARE_WIDTH_FRACTION;
        } else {
          scaleTemplateRegionPercent = 1.1f;
        }
        
        successTolerance_angle        = DEG_TO_RAD(30);
        successTolerance_distance     = 20.f;
        successTolerance_matchingPixelsFraction = 0.75f;
#else
        scaleTemplateRegionPercent    = 1.1f;
        convergenceTolerance          = 1.f;
        maxSamplesAtBaseLevel         = 500;
#endif
        
        
#endif // if DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
        isInitialized = true;
      }
      
      
#if 0
#pragma mark --- Face Detection Parameters ----
#endif
      
      FaceDetectionParameters::FaceDetectionParameters()
      : isInitialized(false)
      {
        
      } // FaceDetectionParameters()
      
      void FaceDetectionParameters::Initialize(Vision::CameraResolution resolution)
      {
        detectionResolution = resolution;
        
        faceDetectionHeight = Vision::CameraResInfo[detectionResolution].height;
        faceDetectionWidth  = Vision::CameraResInfo[detectionResolution].width;

        scaleFactor    = 1.1;
        minNeighbors   = 2;
        minHeight      = 30;
        minWidth       = 30;
        maxHeight      = faceDetectionHeight;
        maxWidth       = faceDetectionWidth;
        MAX_CANDIDATES = 5000;
                
        isInitialized = true;
        
      } // FaceDetectionParameters::Initialize()
      

  } // namespace Cozmo
} // namespace Anki


