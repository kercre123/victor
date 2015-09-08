/**
 * File: visionParameters.h [Basestation]
 *
 * Author: Andrew Stein
 * Date:   3/28/2014
 *
 * Description: High-level vision system parameter definitions, including the
 *              type of tracker to use.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_BASESTATION_VISIONPARAMETERS_H
#define ANKI_COZMO_BASESTATION_VISIONPARAMETERS_H

#include "anki/common/types.h"

#include "anki/vision/robot/binaryTracker.h"
#include "anki/vision/robot/lucasKanade.h"

#include "anki/vision/CameraSettings.h"

//#include "anki/cozmo/robot/hal.h"

namespace Anki {
  namespace Cozmo {

      //
      // Preprocessor directives
      //
      
//#define USE_HEADER_TEMPLATE //< Currently only supported for binary tracker and battery marker
      
      // Set to 1 to use the top (or bottom) bar of the tracked marker to approximate
      // the pose of the block relative to the camera for docking.
      // NOTE: This *must* be set to 1 if using an affine tracker.
#define USE_APPROXIMATE_DOCKING_ERROR_SIGNAL 0
      
#define USE_MATLAB_TRACKER  0
#define USE_MATLAB_DETECTOR 0

#ifdef STREAM_DEBUG_IMAGES
      #define SEND_DEBUG_STREAM 1
      //#define RUN_SIMPLE_TRACKING_TEST 1
      #define RUN_GROUND_TRUTHING_CAPTURE 1
      //#define SEND_IMAGE_ONLY 1      
      //#define SEND_BINARY_IMAGE_ONLY 1
      
#else
      #define SEND_DEBUG_STREAM 0
#endif
      
#define P3P_PRECISION f32
     
      //
      // Fiducial Detection Parameters
      //
      struct DetectFiducialMarkersParameters
      {
        static const s32 MAX_MARKERS = 64;
        
        bool isInitialized;
        Vision::CameraResolution detectionResolution;
        s32 detectionWidth;
        s32 detectionHeight;
        s32 scaleImage_thresholdMultiplier;
        s32 scaleImage_numPyramidLevels;
        s32 component1d_minComponentWidth;
        s32 component1d_maxSkipDistance;
        f32 minSideLength;
        f32 maxSideLength;
        s32 component_minimumNumPixels;
        s32 component_maximumNumPixels;
        s32 component_sparseMultiplyThreshold;
        s32 component_solidMultiplyThreshold;
        f32 component_minHollowRatio;
        s32 minLaplacianPeakRatio;
        s32 maxExtractedQuads;
        s32 quads_minQuadArea;
        s32 quads_quadSymmetryThreshold;
        s32 quads_minDistanceFromImageEdge;
        f32 decode_minContrastRatio;
        u16 maxConnectedComponentSegments;
        s32 quadRefinementIterations; // set to zero to disable
        s32 numRefinementSamples;
        f32 quadRefinementMaxCornerChange;
        f32 quadRefinementMinCornerChange;
        bool keepUnverifiedMarkers;
        
        // Methods
        DetectFiducialMarkersParameters();
        void Initialize(Vision::CameraResolution resolution);
        
      }; // struct DetectFiducialMarkersParameters
      
      
      // Tracker Parameters, depending on the tracking algorithm
      struct TrackerParameters
      {
        bool isInitialized;
        Vision::CameraResolution trackingResolution;
        s32 trackingImageHeight;
        s32 trackingImageWidth;
        f32 normalizationFilterWidthFraction; // as fraction of tracking quad diagonal (0 to disable)
        
        u8 verify_maxPixelDifference;
        

        // LK Trackers:
        f32 scaleTemplateRegionPercent;
        s32 numPyramidLevels;
        s32 maxIterations;

        bool useWeights;
        
        // Planar6DoF tracker has separate convergence tolerance for
        // angle and distance components parameters of the tracker
        f32 convergenceTolerance_angle;    // radians
        f32 convergenceTolerance_distance; // mm
        s32 numInteriorSamples;            // number of samples on the code within the fiducial
        s32 numSamplingRegions;            // NxN grid of sampling regions within fiducial
        s32 numFiducialEdgeSamples;        // Number of samples on fiducial square's edges
        
        f32 successTolerance_angle;
        f32 successTolerance_distance;
        f32 successTolerance_matchingPixelsFraction;

        // Parameters for sanity checks after tracker update
        static const f32 MIN_TRACKER_DISTANCE;
        static const f32 MAX_TRACKER_DISTANCE;
        static const f32 MAX_BLOCK_DOCKING_ANGLE;
        static const f32 MAX_DOCKING_FOV_ANGLE;
        
        TrackerParameters();
        void Initialize(Vision::CameraResolution resolution);
        
      }; // struct TrackerParameters
      
      
      //
      // Simulator
      //
      struct SimulatorParameters {
#ifdef SIMULATOR
        static const u32 FIDUCIAL_DETECTION_SPEED_HZ = 30;
        static const u32 FACE_DETECTION_SPEED_HZ     = 30;
        
        static const u32 TRACKING_ALGORITHM_SPEED_HZ = 15;
        
        static const u32 TRACK_BLOCK_PERIOD_US        = 1e6 / TRACKING_ALGORITHM_SPEED_HZ;
        static const u32 FIDUCIAL_DETECTION_PERIOD_US = 1e6 / FIDUCIAL_DETECTION_SPEED_HZ;
        static const u32 FACE_DETECTION_PERIOD_US     = 1e6 / FACE_DETECTION_SPEED_HZ;
#endif
      }; // struct SimulatorParameters
      
      
      //
      // Face Detection
      //
      struct FaceDetectionParameters {
        bool isInitialized;
        
        Vision::CameraResolution detectionResolution;
        s32 faceDetectionHeight;
        s32 faceDetectionWidth;
        
        double scaleFactor;
        int minNeighbors;
        s32 minHeight;
        s32 minWidth;
        s32 maxHeight;
        s32 maxWidth;
        s32 MAX_CANDIDATES;
        
        // Maximum number of detections in a single image that will be put
        // into the mailbox for pickup by main execution. Note that the
        // multi-mailbox is a circular buffer, so if there are more than 16
        // detections in a frame, the next one will overwrite the first one.
        static const u32 MAX_FACE_DETECTIONS = 16;
        
        FaceDetectionParameters();
        void Initialize(Vision::CameraResolution resolution);
        
      }; // struct FaceDetectionParameters
      
      
      //
      // Tracker Typedef
      //
      typedef Embedded::TemplateTracker::LucasKanadeTracker_SampledPlanar6dof Tracker;
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_VISIONPARAMETERS_H

