/**
 * File: visionProcessingThread.h
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

//#include "clad/robotInterface/messageRobotToEngine.h"
//#include "clad/robotInterface/messageEngineToRobot.h"
//#include "clad/vizInterface/messageViz.h"

#include "anki/vision/basestation/cameraCalibration.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/visionMarker.h"
#include "anki/vision/basestation/faceTracker.h"
#include "clad/types/robotStatusAndActions.h"
#include <thread>
#include <mutex>

namespace Anki {
  
// Forward declaration
namespace Util {
namespace Data {
  class DataPlatform;
}
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
class VisionSystem;
struct DockingErrorSignal;

class VisionProcessingThread
  {
  public:
    
    VisionProcessingThread(Util::Data::DataPlatform* dataPlatform);
    ~VisionProcessingThread();
    
    //
    // Asynchronous operation
    //
    void Start(); // SetCameraCalibration() must have been called already
    void Start(const Vision::CameraCalibration& camCalib);
    void Stop();
    
    void SetNextImage(const Vision::Image& image,
                      const RobotState& robotState);
    
    //
    // Synchronous operation
    //
    void SetCameraCalibration(const Vision::CameraCalibration& camCalib);
    
    void Update(const Vision::Image& image,
                const RobotState& robotState);

    
    void SetMarkerToTrack(const Vision::Marker::Code&  markerToTrack,
                          const f32                    markerWidth_mm,
                          const Point2f&               imageCenter,
                          const f32                    radius,
                          const bool                   checkAngleX);
    
    // Enable/disable different types of processing
    void EnableMarkerDetection(bool tf);
    void EnableFaceDetection(bool tf);
    
    // Abort any marker tracking we were doing
    void StopMarkerTracking();
    
    // These return true if a mailbox messages was available, and they copy
    // that message into the passed-in message struct.
    //bool CheckMailbox(ImageChunk&          msg);
    bool CheckMailbox(DockingErrorSignal&  msg);
    //bool CheckMailbox(FaceDetection&       msg);
    bool CheckMailbox(Vision::ObservedMarker&     msg);
    bool CheckMailbox(VizInterface::TrackerQuad&         msg);
    bool CheckMailbox(RobotInterface::PanAndTilt&      msg);
    bool CheckMailbox(Vision::TrackedFace&        msg);
    
    // If the current image is newer than the specified timestamp, copy it into
    // the given img and return true.
    bool GetCurrentImage(Vision::Image& img, TimeStamp_t newerThanTimestamp);
    
    bool GetLastProcessedImage(Vision::Image& img, TimeStamp_t newerThanTimestamp);
    
    TimeStamp_t GetLastProcessedImageTimeStamp();
    
  protected:
    
    VisionSystem* _visionSystem;
    
    Vision::CameraCalibration _camCalib;
    bool   _isCamCalibSet;
    
    bool   _running;
    std::mutex _lock;
    
    Vision::Image _currentImg;
    Vision::Image _nextImg;
    Vision::Image _lastImg; // the last image we processed
    
    RobotState _currentRobotState;
    RobotState _nextRobotState;
    
    std::thread _processingThread;
    
    void Processor();
    
    void Lock();
    void Unlock();
    
    
  }; // class VisionProcessingThread


} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_VISION_PROC_THREAD_H

