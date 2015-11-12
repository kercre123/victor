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
#include "clad/types/robotStatusAndActions.h"
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

namespace VizInterface {
class MessageViz;
struct TrackerQuad;
enum class MessageVizTag : uint8_t;
} // end namespace VizInterface

// Forward declaration
class Robot;
class VisionSystem;
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
    void SetCameraCalibration(const Vision::CameraCalibration& camCalib);
    
    // Provide next image for processing, with corresponding robot state.
    // In synchronous mode, the image is processed immediately. In asynchronous
    // mode, it will be processed as soon as the current image is completed.
    // Also, any debug images left by vision processing for display will be
    // displayed.
    void SetNextImage(const Vision::ImageRGB& image,
                      const RobotState& robotState);

    void Pause(); // toggle paused state
    void Pause(bool isPaused); // set pause state
    
    // Enable/disable different types of processing
    void EnableMarkerDetection(bool tf);
    void EnableFaceDetection(bool tf);
    void EnableMotionDetection(bool tf);
    
    // Vision system will switch to tracking when this marker is seen
    void SetMarkerToTrack(const Vision::Marker::Code&  markerToTrack,
                          const f32                    markerWidth_mm,
                          const Point2f&               imageCenter,
                          const f32                    radius,
                          const bool                   checkAngleX,
                          const f32                    postOffsetX_mm = 0,
                          const f32                    postOffsetY_mm = 0,
                          const f32                    postOffsetAngle_rad = 0);
    
    // Abort any marker tracking we were doing
    void StopMarkerTracking();
    
    Result UpdateFaces(Robot& robot);
    Result UpdateVisionMarkers(Robot& robot);
    Result UpdateTrackingQuad(Robot& robot);
    Result UpdateDockingErrorSignal(Robot& robot);
    Result UpdateMotionCentroid(Robot& robot);
    
    const Vision::Camera& GetCamera(void) const;
    Vision::Camera& GetCamera(void);
    
    const Vision::CameraCalibration& GetCameraCalibration() const;
      
    // If the current image is newer than the specified timestamp, copy it into
    // the given img and return true.
    bool GetCurrentImage(Vision::ImageRGB& img, TimeStamp_t newerThanTimestamp);
    
    bool GetLastProcessedImage(Vision::ImageRGB& img, TimeStamp_t newerThanTimestamp);
    
    TimeStamp_t GetLastProcessedImageTimeStamp();
    
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
    
    RobotState _currentRobotState;
    RobotState _nextRobotState;
    
    std::thread _processingThread;
    
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

