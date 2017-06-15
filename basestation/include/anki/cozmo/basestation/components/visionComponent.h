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
#include "anki/vision/basestation/droppedFrameStats.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/visionMarker.h"
#include "anki/vision/basestation/faceTracker.h"
#include "anki/cozmo/basestation/components/nvStorageComponent.h"
#include "anki/cozmo/basestation/encodedImage.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robotStateHistory.h"
#include "anki/cozmo/basestation/rollingShutterCorrector.h"
#include "anki/cozmo/basestation/visionModeSchedule.h"
#include "anki/cozmo/basestation/visionPoseData.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/cameraParams.h"
#include "clad/types/loadedKnownFace.h"
#include "clad/types/robotStatusAndActions.h"
#include "clad/types/visionModes.h"

#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <thread>
#include <mutex>
#include <list>
#include <vector>

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

// Forward declaration
class Robot;
class CozmoContext;
struct VisionProcessingResult;
class VisionSystem;
class VizManager;
  
struct DockingErrorSignal;

  class VisionComponent : public Util::noncopyable
  {
  public:
  
    VisionComponent(Robot& robot, const CozmoContext* context);
    virtual ~VisionComponent();
    
    Result Init(const Json::Value& config);

    // SetNextImage does nothing until enabled
    void Enable(bool enable) { _enabled = enable; }
    
    // Set whether vision system runs synchronously or on its own thread
    void SetIsSynchronous(bool isSync);

    // Calibration must be provided before Update() will run
    void SetCameraCalibration(Vision::CameraCalibration& camCalib);
    
    // Provide next image for processing, with corresponding robot state.
    // In synchronous mode, the image is processed immediately. In asynchronous
    // mode, it will be processed as soon as the current image is completed.
    // Also, any debug images left by vision processing for display will be
    // displayed.
    // NOTE: The given encodedImage is swapped into place, so it will no longer
    //       be valid in the caller after using this method.
    Result SetNextImage(EncodedImage& encodedImage);

    void Pause(bool isPaused);
    
    // Enable/disable different types of processing
    Result EnableMode(VisionMode mode, bool enable);
    
    // Push new schedule which takes affect on next image and can be popped using
    // PopCurrentModeSchedule() below.
    Result PushNextModeSchedule(AllVisionModesSchedule&& schedule);
    
    // Return to the schedule prior to the last push.
    // Note that you cannot popup the last remaining (original) schedule.
    Result PopCurrentModeSchedule();
    
    // Check whether a specific vision mode is enabled
    bool IsModeEnabled(VisionMode mode) const;
    
    // Set whether or not markers queued while robot is "moving" (meaning it is
    // turning too fast or head is moving too fast) will be considered
    void   EnableVisionWhileMovingFast(bool enable);
    
    // Looks through all results available from the VisionSystem and processes them.
    // This updates the Robot's BlockWorld and FaceWorld using those results.
    Result UpdateAllResults();
    
    // Individual processing update helpers. These are called individually by
    // UpdateAllResults() above, but are exposed as public fo Unit Test usage.
    Result UpdateFaces(const VisionProcessingResult& result);
    Result UpdatePets(const VisionProcessingResult& procResult);
    Result UpdateVisionMarkers(const VisionProcessingResult& result);
    Result UpdateMotionCentroid(const VisionProcessingResult& result);
    Result UpdateLaserPoints(const VisionProcessingResult& result);
    Result UpdateOverheadEdges(const VisionProcessingResult& result);
    Result UpdateToolCode(const VisionProcessingResult& result);
    Result UpdateComputedCalibration(const VisionProcessingResult& result);
    Result UpdateImageQuality(const VisionProcessingResult& procResult);
    
    Result UpdateOverheadMap(const Vision::ImageRGB& image,
                             const VisionPoseData& poseData);

    const Vision::Camera& GetCamera(void) const;
    Vision::Camera& GetCamera(void);
    
    const Vision::CameraCalibration& GetCameraCalibration() const;
    bool IsCameraCalibrationSet() const { return _isCamCalibSet; }
    Result ClearCalibrationImages();
    
    // If enabled, the camera calibration will be updated based on the
    // position of the centroids of the dots that are part of the tool codes.
    // Fails if vision system is already in the middle of reading tool code.
    Result EnableToolCodeCalibration(bool enable);
      
    TimeStamp_t GetLastProcessedImageTimeStamp() const;
    
    TimeStamp_t GetProcessingPeriod_ms() const;
    TimeStamp_t GetFramePeriod_ms() const;
    
    template<class PixelType>
    Result CompressAndSendImage(const Vision::ImageBase<PixelType>& img, s32 quality);
    
    // Detected markers will only be queued for BlockWorld processing if the robot
    // was turning by less than these amounts when they were observed.
    // Use values < 0 to set to defaults
    void SetMarkerDetectionTurnSpeedThresholds(f32 bodyTurnSpeedThresh_degPerSec,
                                               f32 headTurnSpeedThresh_degPerSec);

    // Get the current thresholds in case you want to be able to restore what they
    // were before you changed them
    void GetMarkerDetectionTurnSpeedThresholds(f32& bodyTurnSpeedThresh_degPerSec,
                                               f32& headTurnSpeedThresh_degPerSec) const;
    
    bool WasHeadRotatingTooFast(TimeStamp_t t,
                                const f32 headTurnSpeedLimit_radPerSec = DEG_TO_RAD(10),
                                const int numImuDataToLookBack = 0) const;
    bool WasBodyRotatingTooFast(TimeStamp_t t,
                                const f32 bodyTurnSpeedLimit_radPerSec = DEG_TO_RAD(10),
                                const int numImuDataToLookBack = 0) const;
    
    // Returns true if head or body were moving too fast at the timestamp
    // If numImuDataToLookBack is greater than zero we will look that far back in imu data history instead
    // of just looking at the previous and next imu data
    bool WasRotatingTooFast(TimeStamp_t t,
                            const f32 bodyTurnSpeedLimit_radPerSec = DEG_TO_RAD(10),
                            const f32 headTurnSpeedLimit_radPerSec = DEG_TO_RAD(10),
                            const int numImuDataToLookBack = 0) const;

    // Add an occluder to the camera for the cross-bar of the lift in its position
    // at the requested time
    void AddLiftOccluder(TimeStamp_t t_request);
    
    // Camera calibration
    void StoreNextImageForCameraCalibration(const Rectangle<s32>& targetROI);
    void StoreNextImageForCameraCalibration(); // Target ROI = entire image
    
    bool WillStoreNextImageForCameraCalibration() const { return _storeNextImageForCalibration;  }
    size_t  GetNumStoredCameraCalibrationImages() const;
    
    // Get jpeg compressed data of calibration images
    // dotsFoundMask has bits set to 1 wherever the corresponding calibration image was found to contain a valid target
    std::list<std::vector<u8> > GetCalibrationImageJpegData(u8* dotsFoundMask = nullptr);
    
    // Get the specified calibration pose to the robot. 'whichPose' must be [0,numCalibrationimages].
    Result GetCalibrationPoseToRobot(size_t whichPose, Pose3d& p);
    
    // Tool code images
    Result ClearToolCodeImages();    
    size_t  GetNumStoredToolCodeImages() const;
    std::list<std::vector<u8> > GetToolCodeImageJpegData();

    // For factory test behavior use only: tell vision component to find the
    // four dots for the test target and compute camera pose. Result is
    // broadcast via an EngineToGame::RobotCompletedFactoryDotTest message.
    void UseNextImageForFactoryDotTest() { _doFactoryDotTest = true; }
    
    // Called automatically when doFactoryDotTest=true. Exposed publicly for unit test
    Result FindFactoryTestDotCentroids(const Vision::Image& image,
                                       ExternalInterface::RobotCompletedFactoryDotTest& msg);
    
    // Used by FindFactoryTestDotCentroids iff camera is already calibrated.
    // Otherwise call manually by populating obsQuad with the dot centroids.
    Result ComputeCameraPoseVsIdeal(const Quad2f& obsQuad, Pose3d& pose) const;
    
    const ImuDataHistory& GetImuDataHistory() const { return _imuHistory; }
    ImuDataHistory& GetImuDataHistory() { return _imuHistory; }

    // Links a name with a face ID and sets up the robot's ability to say that name.
    // Then merges the newly-named ID into mergeWithID, if set to a valid existing ID.
    void AssignNameToFace(Vision::FaceID_t faceID, const std::string& name,
                          Vision::FaceID_t mergeWithID = Vision::UnknownFaceID);
    
		// Enable face enrollment mode and optionally specify the ID for which 
    // enrollment is allowed (use UnknownFaceID to indicate "any" ID).
    // Enrollment will automatically disable after numEnrollments. (Use 
    // a value < 0 to enable ongoing enrollments.)
		void SetFaceEnrollmentMode(Vision::FaceEnrollmentPose pose,
															 Vision::FaceID_t forFaceID = Vision::UnknownFaceID,
															 s32 numEnrollments = -1);

    // Erase faces
    Result EraseFace(Vision::FaceID_t faceID);
    void   EraseAllFaces();
    
    // Will assign a new name to a given face ID. The old name is provided as a
    // safety measure: the rename will only happen if the given old name matches
    // the one currently assigned to faceID. Otherwise, failure is returned.
    // On success, a RobotLoadedKnownFace message with the new ID/name pairing is broadcast.
    Result RenameFace(Vision::FaceID_t faceID, const std::string& oldName, const std::string& newName);
    
    // Load/Save face album data to/from robot's NVStorage
    Result SaveFaceAlbumToRobot();
    Result SaveFaceAlbumToRobot(std::function<void(NVStorage::NVResult)> albumCallback,
                                std::function<void(NVStorage::NVResult)> enrollCallback);
    Result LoadFaceAlbumFromRobot(); // Broadcasts any loaded names and IDs
    
    // Load/Save face album data to/from file.
    // NOTE: Load replaces whatever is in the robot's NVStorage!
    Result SaveFaceAlbumToFile(const std::string& path);
    Result LoadFaceAlbumFromFile(const std::string& path); // Broadcasts any loaded names and IDs
    Result LoadFaceAlbumFromFile(const std::string& path, std::list<Vision::LoadedKnownFace>& loadedFaces); // Populates list, does not broadcast
    
    // This is for faking images being processed for unit tests
    void FakeImageProcessed(TimeStamp_t t, const std::vector<const ImageImuData>& imuData={});
    
    // Handles receiving the default camera parameters from robot which are requested when we
    // read the camera calibration
    void HandleDefaultCameraParams(const DefaultCameraParams& params);
    
    // Templated message handler used internally by AnkiEventUtil
    template<typename T>
    void HandleMessage(const T& msg);
    
    void SetAndDisableAutoExposure(u16 exposure_ms, f32 gain);
    void EnableAutoExposure(bool enable);
    void EnableColorImages(bool enable);
    
    s32 GetMinCameraExposureTime_ms() const;
    s32 GetMaxCameraExposureTime_ms() const;
    f32 GetMinCameraGain() const;
    f32 GetMaxCameraGain() const;
    
    bool AreColorImagesEnabled() const { return _enableColorImages; }
    s32  GetCurrentCameraExposureTime_ms() const;
    f32  GetCurrentCameraGain() const;
    
#   ifdef COZMO_V2
    // COZMO 2.0 ONLY
    // Captures image to be queued for processing and sent to game and viz
    void CaptureAndSendImage();
#   endif

    f32 GetBodyTurnSpeedThresh_degPerSec() const;
    
  protected:
    
    bool _isInitialized = false;
    
    Robot& _robot;
    const CozmoContext* _context = nullptr;
    
    VisionSystem* _visionSystem = nullptr;
    VizManager*   _vizManager = nullptr;
  
    // Robot stores the calibration, camera just gets a reference to it
    // This is so we can share the same calibration data across multiple
    // cameras (e.g. those stored inside the pose history)
    Vision::Camera            _camera;
    Vision::CameraCalibration _camCalib;
    bool                      _isCamCalibSet = false;
    bool                      _enabled = false;
    
    bool   _isSynchronous = false;
    bool   _running = false;
    bool   _paused  = false;
    std::mutex _lock;
    
    EncodedImage _currentImg;
    EncodedImage _nextImg;
    
    Vision::DroppedFrameStats _dropStats;
    
    ImuDataHistory _imuHistory;

    bool _storeNextImageForCalibration = false;
    Rectangle<s32> _calibTargetROI;
    
    constexpr static f32 kDefaultBodySpeedThresh = DEG_TO_RAD(60);
    constexpr static f32 kDefaultHeadSpeedThresh = DEG_TO_RAD(10);
    f32 _markerDetectionBodyTurnSpeedThreshold_radPerSec = kDefaultBodySpeedThresh;
    f32 _markerDetectionHeadTurnSpeedThreshold_radPerSec = kDefaultHeadSpeedThresh;
    
    TimeStamp_t _lastReceivedImageTimeStamp_ms = 0;
    TimeStamp_t _lastProcessedImageTimeStamp_ms = 0;
    TimeStamp_t _processingPeriod_ms = 0;  // How fast we are processing frames
    TimeStamp_t _framePeriod_ms = 0;       // How fast we are receiving frames
    
    ImageQuality _lastImageQuality = ImageQuality::Good;
    ImageQuality _lastBroadcastImageQuality = ImageQuality::Unchecked;
    TimeStamp_t  _currentQualityBeginTime_ms = 0;
    TimeStamp_t  _waitForNextAlert_ms = 0;
    
    VisionPoseData   _currentPoseData;
    VisionPoseData   _nextPoseData;
    bool             _visionWhileMovingFastEnabled = false;
    
    std::vector<u8> _albumData, _enrollData; // for loading / saving face data
    
    std::thread _processingThread;
    
    std::vector<Signal::SmartHandle> _signalHandles;
    
    std::map<f32,Matrix_3x3f> _groundPlaneHomographyLUT; // keyed on head angle in radians
    void PopulateGroundPlaneHomographyLUT(f32 angleResolution_rad = DEG_TO_RAD(0.25f));
    bool LookupGroundPlaneHomography(f32 atHeadAngle, Matrix_3x3f& H) const;
    
    void Processor();
    void UpdateVisionSystem(const VisionPoseData& poseData, const EncodedImage& encodedImgs);
    
    void Lock();
    void Unlock();
    
    // Used for asynchronous run mode
    void Start(); // SetCameraCalibration() must have been called already
    void Stop();
    
    // Helper for loading face album data from file / robot
    void BroadcastLoadedNamesAndIDs(const std::list<Vision::LoadedKnownFace>& loadedFaces) const;
    
    void VisualizeObservedMarkerIn3D(const Vision::ObservedMarker& marker) const;
    
    // Sets the exposure and gain on the robot
    void SetCameraSettings(const s32 exposure_ms, const f32 gain);
    
    // Factory centroid finder: returns the centroids of the 4 factory test dots,
    // computes camera pose w.r.t. the target and broadcasts a RobotCompletedFactoryDotTest
    // message. This runs on the main thread and should only be used for factory tests.
    // Is run automatically when _doFactoryDotTest=true and sets it back to false when done.
    bool _doFactoryDotTest = false;
    
    bool _enableAutoExposure = true;
    bool _enableColorImages = false;
    
  }; // class VisionComponent
  
  inline void VisionComponent::Pause(bool isPaused) {
    PRINT_CH_INFO("VisionComponent", "VisionComponent.Pause.Set", "Setting Paused from %d to %d",
                  _paused, isPaused);
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
  
  inline void VisionComponent::EnableVisionWhileMovingFast(bool enable) {
    _visionWhileMovingFastEnabled = enable;
  }
  
  inline void VisionComponent::SetMarkerDetectionTurnSpeedThresholds(f32 bodyTurnSpeedThresh_degPerSec,
                                                                     f32 headTurnSpeedThresh_degPerSec)
  {
    if(bodyTurnSpeedThresh_degPerSec < 0) {
      _markerDetectionBodyTurnSpeedThreshold_radPerSec = kDefaultBodySpeedThresh;
    } else {
      _markerDetectionBodyTurnSpeedThreshold_radPerSec = DEG_TO_RAD(bodyTurnSpeedThresh_degPerSec);
    }
    
    if(headTurnSpeedThresh_degPerSec < 0) {
      _markerDetectionHeadTurnSpeedThreshold_radPerSec = kDefaultHeadSpeedThresh;
    } else {
      _markerDetectionHeadTurnSpeedThreshold_radPerSec = DEG_TO_RAD(headTurnSpeedThresh_degPerSec);
    }
  }

  inline void VisionComponent::GetMarkerDetectionTurnSpeedThresholds(f32& bodyTurnSpeedThresh_degPerSec,
                                                                     f32& headTurnSpeedThresh_degPerSec) const
  {
    bodyTurnSpeedThresh_degPerSec = RAD_TO_DEG(_markerDetectionBodyTurnSpeedThreshold_radPerSec);
    headTurnSpeedThresh_degPerSec = RAD_TO_DEG(_markerDetectionHeadTurnSpeedThreshold_radPerSec);
  }
  
  inline void VisionComponent::StoreNextImageForCameraCalibration() {
    StoreNextImageForCameraCalibration(Rectangle<s32>(0,0,_camCalib.GetNcols(), _camCalib.GetNrows()));
  }
  
  inline void VisionComponent::StoreNextImageForCameraCalibration(const Rectangle<s32>& targetROI) {
    _storeNextImageForCalibration = true;
    _calibTargetROI = targetROI;
  }
  
  inline TimeStamp_t VisionComponent::GetFramePeriod_ms() const
  {
    return _framePeriod_ms;
  }
  
  inline TimeStamp_t VisionComponent::GetProcessingPeriod_ms() const
  {
    return _processingPeriod_ms;
  }

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_VISION_PROC_THREAD_H
