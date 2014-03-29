
#ifndef ANKI_VISIONPARAMETERS_H
#define ANKI_VISIONPARAMETERS_H

#include "anki/common/types.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/visionSystem.h"

namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
      
     
      //
      // Fiducial Detection Parameters
      //
      struct DetectFiducialMarkersParameters
      {
        bool isInitialized;
        HAL::CameraMode detectionResolution;
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
        s32 maxExtractedQuads;
        s32 quads_minQuadArea;
        s32 quads_quadSymmetryThreshold;
        s32 quads_minDistanceFromImageEdge;
        f32 decode_minContrastRatio;
        s32 maxConnectedComponentSegments;
        
        // Methods
        DetectFiducialMarkersParameters();
        void Initialize();
        
      }; // struct DetectFiducialMarkersParameters
      
      
      // Tracker Parameters, depending on the tracking algorithm
      struct TrackerParameters
      {
        bool isInitialized;
        HAL::CameraMode trackingResolution;
        s32 trackingImageHeight;
        s32 trackingImageWidth;
        
#if DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
        
        f32 scaleTemplateRegionPercent;
        //u8  edgeDetection_grayvalueThreshold;
        s32 edgeDetection_threshold_yIncrement;
        s32 edgeDetection_threshold_xIncrement;
        f32 edgeDetection_threshold_blackPercentile;
        f32 edgeDetection_threshold_whitePercentile;
        f32 edgeDetection_threshold_scaleRegionPercent;
        s32 edgeDetection_minComponentWidth;
        s32 edgeDetection_maxDetectionsPerType;
        s32 edgeDetection_everyNLines;
        s32 matching_maxTranslationDistance;
        s32 matching_maxProjectiveDistance;
        s32 verification_maxTranslationDistance;
        f32 percentMatchedPixelsThreshold;
        
#else
        // LK Trackers:
        f32 scaleTemplateRegionPercent;
        s32 numPyramidLevels;
        s32 maxIterations;
        f32 convergenceTolerance;
        u8 verify_maxPixelDifference;
        bool useWeights;
        s32 maxSamplesAtBaseLevel;
        
#endif // if DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
        
        TrackerParameters();
        void Initialize();
        
      }; // struct TrackerParameters
      
      
      //
      // Simulator
      //
      struct SimulatorParameters {

#ifdef SIMULATOR
        static const u32 LOOK_FOR_BLOCK_PERIOD_US = 200000; // 5Hz
        static const u32 TRACKING_ALGORITHM_SPEED_HZ = 10;
        static const u32 TRACK_BLOCK_PERIOD_US = 1e6 / TRACKING_ALGORITHM_SPEED_HZ;
        
        u32 frameRdyTimeUS_;
#endif
        
        ReturnCode Initialize();
        
      }; // struct SimulatorParameters
      
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_VISIONPARAMETERS_H

