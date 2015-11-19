/**
 * File: visionComponent.h
 *
 * Author: Andrew Stein
 * Date:   11/20/2014
 *
 * Description: Container for the thread containing the basestation vision
 *              system, which provides methods for managing and communicating
 *              with it.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_BASESTATION_VISION_PROC_THREAD_H
#define ANKI_COZMO_BASESTATION_VISION_PROC_THREAD_H

#include "anki/vision/basestation/cameraCalibration.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/visionMarker.h"
#include "anki/vision/basestation/faceTracker.h"
#include "anki/cozmo/basestation/robotPoseHistory.h"
#include "anki/cozmo/basestation/visionSystem.h"
#include "clad/types/robotStatusAndActions.h"
#include "clad/types/visionModes.h"
#include "util/helpers/noncopyable.h"

#include <thread>
#include <mutex>

namespace Anki {
  
// Forward declaration
namespace Util {
namespace Data {
  class DataPlatform;
}
}
  
namespace Vision {
  class TrackedFace;
}
  
namespace Cozmo {

namespace RobotInterface {
  struct PanAndTilt;
  class EngineToRobot;
  class RobotToEngine;
  enum class EngineToRobotTag : uint8_t;
  enum class RobotToEngineTag : uint8_t;
} // end namespace RobotInterface

// Forward declaration
class Robot;
  
struct DockingErrorSignal;

  class VisionComponent : public Util::noncopyable
  {
  public:
    
    enum class RunMode : u8 {
      Synchronous,
      Asynchronous
    };
    
    VisionComponent(RobotID_t robotID, RunMode mode, Util::Data::DataPlatform* dataPlatform);
    virtual ~VisionComponent();
    
    void SetRunMode(RunMode mode);

    // Calibration must be provided before Update() can be called
    void SetCameraCalibration(const Robot& robot, const Vision::CameraCalibration& camCalib);
    
    // Provide next image for processing, with corresponding robot state.
    // In synchronous mode, the image is processed immediately. In asynchronous
    // mode, it will be processed as soon as the current image is completed.
    // Also, any debug images left by vision processing for display will be
    // displayed.
    Result SetNextImage(const Vision::ImageRGB& image,
                        const Robot& robot);

    void Pause(); // toggle paused state
    void Pause(bool isPaused); // set pause state
    
    // Enable/disable different types of processing
    Result EnableMode(VisionMode mode, bool enable);
    
    // Check whether a specific vision mode is enabled
    bool IsModeEnabled(VisionMode mode) const;
    
    // Get a bit flag for all enabled vision modes
    u32 GetEnabledModes() const;
    
    // Set modes from a bit mask
    Result SetModes(u32 modes);
    
    // Vision system will switch to tracking when this marker is seen
    void SetMarkerToTrack(const Vision::Marker::Code&  markerToTrack,
                          const f32                    markerWidth_mm,
                          const Point2f&               imageCenter,
                          const f32                    radius,
                          const bool                   checkAngleX,
                          const f32                    postOffsetX_mm = 0,
                          const f32                    postOffsetY_mm = 0,
                          const f32                    postOffsetAngle_rad = 0);
    
    Result UpdateFaces(Robot& robot);
    Result UpdateVisionMarkers(Robot& robot);
    Result UpdateTrackingQuad(Robot& robot);
    Result UpdateDockingErrorSignal(Robot& robot);
    Result UpdateMotionCentroid(Robot& robot);
    Result UpdateOverheadMap(const Vision::ImageRGB& image,
                             const VisionSystem::PoseData& poseData);
    
    const Vision::Camera& GetCamera(void) const;
    Vision::Camera& GetCamera(void);
    
    const Vision::CameraCalibration& GetCameraCalibration() const;
      
    // If the current image is newer than the specified timestamp, copy it into
    // the given img and return true.
    bool GetCurrentImage(Vision::ImageRGB& img, TimeStamp_t newerThanTimestamp);
    
    bool GetLastProcessedImage(Vision::ImageRGB& img, TimeStamp_t newerThanTimestamp);
    
    TimeStamp_t GetLastProcessedImageTimeStamp();
    
    TimeStamp_t GetProcessingPeriod();
    
  protected:
    
    VisionSystem* _visionSystem = nullptr;
    
    // Robot stores the calibration, camera just gets a reference to it
    // This is so we can share the same calibration data across multiple
    // cameras (e.g. those stored inside the pose history)
    Vision::Camera            _camera;
    Vision::CameraCalibration _camCalib;
    bool    _isCamCalibSet = false;
    
    RunMode _runMode = RunMode::Asynchronous;
    
    bool   _running = false;
    bool   _paused  = false;
    std::mutex _lock;
    
    Vision::ImageRGB _currentImg;
    Vision::ImageRGB _nextImg;
    Vision::ImageRGB _lastImg; // the last image we processed
    
    TimeStamp_t _processingPeriod = 0;

    VisionSystem::PoseData   _currentPoseData;
    VisionSystem::PoseData   _nextPoseData;
    
    std::thread _processingThread;
    
    struct GroundPlaneROI {
      // In mm
      f32 dist = 50.f;
      f32 length = 100.f;
      f32 widthFar = 80.f;
      f32 widthClose = 30.f;
    } _groundPlaneROI;
    
    Vision::Image _groundPlaneMask;
    
    std::map<f32,Matrix_3x3f> _groundPlaneHomographyLUT; // keyed on head angle in radians
    void PopulateGroundPlaneHomographyLUT(const Robot& robot, f32 angleResolution_rad = DEG_TO_RAD(0.25f));
    bool LookupGroundPlaneHomography(f32 atHeadAngle, Matrix_3x3f& H) const;
    
    void Processor();
    
    void Lock();
    void Unlock();
    
    // Used for asynchronous run mode
    void Start(); // SetCameraCalibration() must have been called already
    void Stop();
    
  }; // class VisionComponent
  
  inline void VisionComponent::Pause() {
    _paused = !_paused;
  }
  
  inline void VisionComponent::Pause(bool isPaused) {
    _paused = isPaused;
  }
  
  inline const Vision::Camera& VisionComponent::GetCamera(void) const {
    return _camera;
  }
  
  inline Vision::Camera& VisionComponent::GetCamera(void) {
    return _camera;
  }
  
  inline const Vision::CameraCalibration& VisionComponent::GetCameraCalibration() const {
    return _camCalib;
  }

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_VISION_PROC_THREAD_H

