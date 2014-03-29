#ifndef ANKI_COZMO_ROBOT_VISIONSYSTEM_H
#define ANKI_COZMO_ROBOT_VISIONSYSTEM_H

#include "anki/common/types.h"

#include "anki/vision/robot/binaryTracker.h"
#include "anki/vision/robot/fiducialMarkers.h"
#include "anki/vision/robot/lucasKanade.h"

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/messages.h"

// Available trackers for docking:
#define DOCKING_LUCAS_KANADE_SLOW               1 //< LucasKanadeTracker_Slow (doesn't seem to work?)
#define DOCKING_LUCAS_KANADE_AFFINE             2 //< LucasKanadeTracker_Affine (With Translation + Affine option)
#define DOCKING_LUCAS_KANADE_PROJECTIVE         3 //< LucasKanadeTracker_Projective (With Projective + Affine option)
#define DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE 4 //<
#define DOCKING_BINARY_TRACKER                  5 //< BinaryTracker
#define DOCKING_LUCAS_KANADE_PLANAR6DOF         6 //< Currently only implemented in Matlab (USE_MATLAB_TRACKER = 1)

// Set the docker here:
#define DOCKING_ALGORITHM DOCKING_LUCAS_KANADE_PLANAR6DOF

// Set to 1 to use the top (or bottom) bar of the tracked marker to approximate 
// the pose of the block relative to the camera for docking.
// NOTE: This will be forced to 1 if an affine tracker is selected.
#define USE_APPROXIMATE_DOCKING_ERROR_SIGNAL 0

#define USE_MATLAB_TRACKER  1
#define USE_MATLAB_DETECTOR 0

namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
      
      //
      // Data structures:
      //
      
      // Define the tracker type depending on DOCKING_ALGORITHM:
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW
      typedef Embedded::TemplateTracker::LucasKanadeTracker_Slow Tracker;
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
      typedef Embedded::TemplateTracker::LucasKanadeTracker_Affine Tracker;
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
      typedef Embedded::TemplateTracker::LucasKanadeTracker_Projective Tracker;
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE
      typedef Embedded::TemplateTracker::LucasKanadeTracker_SampledProjective Tracker;
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
      typedef Embedded::TemplateTracker::BinaryTracker Tracker;
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PLANAR6DOF
#if !USE_MATLAB_TRACKER
#error Planar 6DoF tracker is currently only available using Matlab.
#endif
      typedef Embedded::TemplateTracker::LucasKanadeTracker_Projective Tracker; // just to keep compilation happening
#else
#error Unknown DOCKING_ALGORITHM
#endif
      
      
      // Define the tracker parameters, depending on the tracking algorithm
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
      
      typedef struct {
        u8  headerByte; // used to specify a frame's resolution in a packet if transmitting
        u16 width, height;
        u8 downsamplePower[HAL::CAMERA_MODE_COUNT];
      } CameraModeInfo_t;
      
      // NOTE: To get the downsampling power to go from resoution "FROM" to
      //       resolution "TO", use:
      //       u8 power = CameraModeInfo[FROM].downsamplePower[TO];
      const CameraModeInfo_t CameraModeInfo[HAL::CAMERA_MODE_COUNT] =
      {
        // VGA
        { 0xBA, 640, 480, {0, 1, 2, 3, 4} },
        // QVGA
        { 0xBC, 320, 240, {0, 0, 1, 2, 3} },
        // QQVGA
        { 0xB8, 160, 120, {0, 0, 0, 1, 2} },
        // QQQVGA
        { 0xBD,  80,  60, {0, 0, 0, 0, 1} },
        // QQQQVGA
        { 0xB7,  40,  30, {0, 0, 0, 0, 0} }
      };
      
      
      //
      // Methods:
      //
      
      ReturnCode Init(void);
      bool IsInitialized();
      
      void Destroy();
      
      // Accessors
      const HAL::CameraInfo* GetCameraCalibration();
      f32 GetTrackingMarkerWidth();
      
      // NOTE: It is important the passed-in robot state message be passed by
      //   value and NOT by reference, since the vision system can be interrupted
      //   by main execution (which updates the state).
      ReturnCode Update(const Messages::RobotState robotState);
      
      void StopTracking();

      // Select a block type to look for to dock with.  Use NUM_MARKER_TYPES to disable.
      // Next time the vision system sees a block of this type while looking
      // for blocks, it will initialize a template tracker and switch to
      // docking mode.
      // TODO: Something smarter about seeing the block in the expected place as well?
      ReturnCode SetMarkerToTrack(const Vision::MarkerType& markerToTrack,
                                  const f32 markerWidth_mm);
      
      u32 DownsampleHelper(const Embedded::Array<u8>& imageIn,
                           Embedded::Array<u8>&       imageOut,
                           Embedded::MemoryStack      scratch);
      
    } // namespace VisionSystem
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_VISIONSYSTEM_H
