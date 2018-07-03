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

#include "coretech/vision/engine/cameraCalibration.h"
#include "coretech/vision/engine/droppedFrameStats.h"
#include "coretech/vision/engine/imageCache.h"
#include "coretech/vision/engine/visionMarker.h"
#include "coretech/vision/engine/faceTracker.h"
#include "engine/components/nvStorageComponent.h"
#include "util/entityComponent/entity.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robotStateHistory.h"
#include "engine/rollingShutterCorrector.h"
#include "engine/vision/visionModeSchedule.h"
#include "engine/vision/visionPoseData.h"

#include "clad/types/cameraParams.h"
#include "clad/types/imageTypes.h"
#include "clad/types/loadedKnownFace.h"
#include "clad/types/robotStatusAndActions.h"
#include "clad/types/salientPointTypes.h"
#include "clad/types/visionModes.h"

#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <thread>
#include <mutex>
#include <list>
#include <vector>
#include <future>

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
  
namespace ExternalInterface {
struct RobotCompletedFactoryDotTest;
}
  
struct DockingErrorSignal;

  class VisionComponent : public IDependencyManagedComponent<RobotComponentID>, public Util::noncopyable
  {
  public:
  
    VisionComponent();
    virtual ~VisionComponent();

    //////
    // IDependencyManagedComponent functions
    //////
    virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps) override;
    virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
      dependencies.insert(RobotComponentID::CozmoContextWrapper);
    };
    virtual void AdditionalInitAccessibleComponents(RobotCompIDSet& components) const override {
      components.insert(RobotComponentID::CozmoContextWrapper);
    };

    virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
      dependencies.insert(RobotComponentID::Movement);
      // Applies the scheduling consequences of the last frame's subscriptions before ticking VisionComponent
      dependencies.insert(RobotComponentID::VisionScheduleMediator);
    };
    virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
    //////
    // end IDependencyManagedComponent functions
    //////

    // SetNextImage does nothing until enabled
    void Enable(bool enable) { _enabled = enable; }
    
    // Set whether vision system runs synchronously or on its own thread
    void SetIsSynchronous(bool isSync);

    // Calibration must be provided before Update() will run
    void SetCameraCalibration(std::shared_ptr<Vision::CameraCalibration> camCalib);
    
    bool IsDisplayingProcessedImagesOnly() const;
        
    // Provide next image for processing, with corresponding robot state.
    // In synchronous mode, the image is processed immediately. In asynchronous
    // mode, it will be processed as soon as the current image is completed.
    // Also, any debug images left by vision processing for display will be
    // displayed.
    // NOTE: The given image is swapped into place, so it will no longer
    //       be valid in the caller after using this method.
    Result SetNextImage(Vision::ImageRGB& image);

    void Pause(bool isPaused);

    // If the vision thread isn't busy, grab the lock and release all internally held images and return
    // true. Otherwise, return false
    bool TryReleaseInternalImages();
    
    // Enable/disable different types of processing
    Result EnableMode(VisionMode mode, bool enable);
    
    // Push new schedule which takes affect on next image and can be popped using
    // PopCurrentModeSchedule() below.
    Result PushNextModeSchedule(AllVisionModesSchedule&& schedule);
    
    // Return to the schedule prior to the last push.
    // Note that you cannot popup the last remaining (original) schedule.
    Result PopCurrentModeSchedule();
    
    // Check whether a specific vision mode is enabled
    bool   IsModeEnabled(VisionMode mode) const;
    
    // Set whether or not markers queued while robot is "moving" (meaning it is
    // turning too fast or head is moving too fast) will be considered
    void   EnableVisionWhileMovingFast(bool enable);

    // Set the camera's capture format
    // Non blocking but will cause VisionComponent/System to
    // wait until no one is using the shared camera memory before
    // requesting the format change and will continue to
    // wait until we once again recieve frames from the camera
    bool   SetCameraCaptureFormat(ImageEncoding format);
    
    // Returns true while camera format is in the process of changing
    bool   IsWaitingForCaptureFormatChange() const;

    // Set whether or not we draw each processed image to the robot's screen
    // (Not using the word "face" because of confusion with "faces" in vision)
    void   EnableDrawImagesToScreen(bool enable) { _drawImagesToScreen = enable; }
    void   AddDrawScreenModifier(const std::function<void(Vision::ImageRGB&)>& modFcn) { _screenImageModFuncs.push_back(modFcn); }
    
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
    Result UpdateComputedCalibration(const VisionProcessingResult& result);
    Result UpdateImageQuality(const VisionProcessingResult& procResult);
    Result UpdateVisualObstacles(const VisionProcessingResult& procResult);
    Result UpdateSalientPoints(const VisionProcessingResult& result);
    Result UpdateWhiteBalance(const VisionProcessingResult& procResult);
    Result UpdatePhotoManager(const VisionProcessingResult& procResult);
    Result UpdateDetectedIllumination(const VisionProcessingResult& procResult);

    const Vision::Camera& GetCamera(void) const;
    Vision::Camera& GetCamera(void);
    
    const std::shared_ptr<Vision::CameraCalibration> GetCameraCalibration() const;
    bool IsCameraCalibrationSet() const { return _camera->IsCalibrated(); }
    Result ClearCalibrationImages();
      
    TimeStamp_t GetLastProcessedImageTimeStamp() const;
    
    TimeStamp_t GetProcessingPeriod_ms() const;
    TimeStamp_t GetFramePeriod_ms() const;
    
    template<class PixelType>
    Result CompressAndSendImage(const Vision::ImageBase<PixelType>& img, s32 quality, const std::string& identifier);
    
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
    void AddLiftOccluder(const TimeStamp_t t_request);
    
    // Camera calibration
    void StoreNextImageForCameraCalibration(const Rectangle<s32>& targetROI);
    void StoreNextImageForCameraCalibration(); // Target ROI = entire image
    
    bool WillStoreNextImageForCameraCalibration() const { return _storeNextImageForCalibration;  }
    size_t  GetNumStoredCameraCalibrationImages() const;
    
    // Get jpeg compressed data of calibration images
    // dotsFoundMask has bits set to 1 wherever the corresponding calibration image was found to contain a valid target
    std::list<std::vector<u8> > GetCalibrationImageJpegData(u8* dotsFoundMask = nullptr) const;
    
    // Get the specified calibration pose to the robot. 'whichPose' must be [0,numCalibrationimages].
    Result GetCalibrationPoseToRobot(size_t whichPose, Pose3d& p) const;
    
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
    
    // Get a list of names and their IDs
    void RequestEnrolledNames();
    
    // Will assign a new name to a given face ID. The old name is provided as a
    // safety measure: the rename will only happen if the given old name matches
    // the one currently assigned to faceID. Otherwise, failure is returned.
    // On success, a RobotLoadedKnownFace message with the new ID/name pairing is broadcast.
    Result RenameFace(Vision::FaceID_t faceID, const std::string& oldName, const std::string& newName);
    
    // Returns true if the provided name has been enrolled
    bool IsNameTaken(const std::string& name);
    
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
    
    // See VisionSystem::SetSaveParameters for details on the arguments
    // NOTE: if path is empty, it will default to <cachePath>/camera/images (where cachePath comes from DataPlatform)
    void SetSaveImageParameters(const ImageSendMode saveMode,
                                const std::string& path,
                                const std::string& basename,
                                const int8_t onRobotQuality,
                                const Vision::ImageCache::Size& saveSize = Vision::ImageCache::Size::Full,
                                const bool removeRadialDistortion = false,
                                const f32 thumbnailScaleFraction = 0.f);

    // This is for faking images being processed for unit tests
    void FakeImageProcessed(TimeStamp_t t);
    
    // Templated message handler used internally by AnkiEventUtil
    template<typename T>
    void HandleMessage(const T& msg);
    
    void SetAndDisableAutoExposure(u16 exposure_ms, f32 gain);
    void EnableAutoExposure(bool enable);

    void SetAndDisableWhiteBalance(f32 gainR, f32 gainG, f32 gainB);
    void EnableWhiteBalance(bool enable);
    
    s32 GetMinCameraExposureTime_ms() const;
    s32 GetMaxCameraExposureTime_ms() const;
    f32 GetMinCameraGain() const;
    f32 GetMaxCameraGain() const;
    
    CameraParams GetCurrentCameraParams() const;
    
    // Captures image data from HAL, if available, and puts it in image_out
    // Returns true if image was captured, false if not
    bool CaptureImage(Vision::ImageRGB& image_out);

    // Releases captured image backing data to CameraService
    bool ReleaseImage(Vision::ImageRGB& image);

    f32 GetBodyTurnSpeedThresh_degPerSec() const;
    
    void SetPhysicalRobot(const bool isPhysical);

    bool LookupGroundPlaneHomography(f32 atHeadAngle, Matrix_3x3f& H) const;

    bool HasStartedCapturingImages() const { return _hasStartedCapturingImages; }
    
    // Non-rotated points representing the lift cross bar
    std::vector<Point3f> _liftCrossBarSource;

  protected:
    
    // helper method --- unpacks bitflags representation into a set of vision modes
    std::set<VisionMode> GetVisionModesFromFlags(u32 bitflags) const;

    bool _isInitialized = false;
    bool _hasStartedCapturingImages = false;
    
    Robot* _robot = nullptr;
    const CozmoContext* _context = nullptr;
    
    VisionSystem* _visionSystem = nullptr;
    VizManager*   _vizManager = nullptr;
    std::map<std::string, s32> _vizDisplayIndexMap;
    std::list<std::pair<TimeStamp_t, Vision::SalientPoint>> _salientPointsToDraw;
    
    // Robot stores the calibration, camera just gets a reference to it
    // This is so we can share the same calibration data across multiple
    // cameras (e.g. those stored inside the pose history)
    std::unique_ptr<Vision::Camera> _camera;
    bool                      _enabled = false;
    
    bool   _isSynchronous = false;
    bool   _running = false;
    bool   _paused  = false;
    bool   _drawImagesToScreen = false;

    std::list<std::function<void(Vision::ImageRGB&)>> _screenImageModFuncs;

    std::mutex _lock;
    
    // Current image is the one the vision system (thread) is actively working on.
    // Next image is the one queued up for the visin system to start processing when it is done with current.
    // Buffered image will become "next" once we've got a corresponding RobotState available in history.
    Vision::ImageRGB _currentImg;
    Vision::ImageRGB _nextImg;
    Vision::ImageRGB _bufferedImg;
    
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

    ImageEncoding _desiredImageFormat = ImageEncoding::NoneImageEncoding;
    ImageEncoding _currentImageFormat = ImageEncoding::RawRGB;
    
    // State machine to make sure nothing is using the shared memory from the camera system
    // before we request a different camera capture format as well as to wait
    // until we get a frame from the camera after changing formats before unpausing
    // vision component
    enum class CaptureFormatState
    {
      None,
      WaitingForProcessingToStop,
      WaitingForFrame
    };
    CaptureFormatState _captureFormatState = CaptureFormatState::None;

    TimeStamp_t _lastImageCaptureTime_ms = 0;

    // Future used for async YUV to RGB conversion
    std::future<Vision::ImageRGB> _cvtYUV2RGBFuture;
    
    std::vector<u8> _albumData, _enrollData; // for loading / saving face data
    
    std::thread _processingThread;
    
    std::vector<Signal::SmartHandle> _signalHandles;
    
    std::map<f32,Matrix_3x3f> _groundPlaneHomographyLUT; // keyed on head angle in radians


    void ReadVisionConfig(const Json::Value& config);
    void PopulateGroundPlaneHomographyLUT(f32 angleResolution_rad = DEG_TO_RAD(0.25f));
    
    void Processor();
    void UpdateVisionSystem(const VisionPoseData& poseData, const Vision::ImageRGB& image);
    
    void Lock();
    void Unlock();
    
    // Used for asynchronous run mode
    void Start(); // SetCameraCalibration() must have been called already
    void Stop();
    
    // Helper for loading face album data from file / robot
    void BroadcastLoadedNamesAndIDs(const std::list<Vision::LoadedKnownFace>& loadedFaces) const;
    
    void VisualizeObservedMarkerIn3D(const Vision::ObservedMarker& marker) const;
    
    // Sets the exposure and gain on the robot
    void SetExposureSettings(const s32 exposure_ms, const f32 gain);

    // Sets the WhiteBalance gains of the camera
    void SetWhiteBalanceSettings(f32 gainR, f32 gainG, f32 gainB);

    // Updates the state of requesting for a camera capture format change
    void UpdateCaptureFormatChange(s32 gotNumRows=0);
    
    // Factory centroid finder: returns the centroids of the 4 factory test dots,
    // computes camera pose w.r.t. the target and broadcasts a RobotCompletedFactoryDotTest
    // message. This runs on the main thread and should only be used for factory tests.
    // Is run automatically when _doFactoryDotTest=true and sets it back to false when done.
    bool _doFactoryDotTest = false;
    
    bool _enableAutoExposure = true;
    bool _enableWhiteBalance = true;
    
    // Threading for OpenCV
    int _openCvNumThreads = 1;
    
  }; // class VisionComponent
  
  inline void VisionComponent::Pause(bool isPaused) {
    if(_paused == isPaused)
    {
      return;
    }

    PRINT_CH_INFO("VisionComponent", "VisionComponent.Pause.Set", "Setting Paused from %d to %d",
                  _paused, isPaused);
    _paused = isPaused;
  }
  
  inline const Vision::Camera& VisionComponent::GetCamera(void) const {
    return *_camera;
  }
  
  inline Vision::Camera& VisionComponent::GetCamera(void) {
    return *_camera;
  }
  
  inline const std::shared_ptr<Vision::CameraCalibration> VisionComponent::GetCameraCalibration() const {
    return _camera->GetCalibration();
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
    StoreNextImageForCameraCalibration(Rectangle<s32>(-1,-1,0,0));
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
