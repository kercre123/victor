/**
 * File: visionComponent.cpp
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


#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/aiComponent/aiComponent.h"
#include "camera/cameraService.h"
#include "engine/ankiEventUtil.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/animationComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/components/photographyManager.h"
#include "engine/components/powerStateManager.h"
#include "engine/components/visionComponent.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/namedColors/namedColors.h"
#include "engine/navMap/mapComponent.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/faceWorld.h"
#include "engine/petWorld.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotStateHistory.h"
#include "engine/vision/imageSaver.h"
#include "engine/vision/visionModesHelpers.h"
#include "engine/vision/visionSystem.h"
#include "engine/viz/vizManager.h"

#include "coretech/vision/engine/compressedImage.h"

#include "coretech/common/engine/opencvThreading.h"
#include "coretech/common/engine/math/polygon_impl.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/logging/DAS.h"
#include "util/string/stringUtils.h"
#include "util/threading/threadPriority.h"

#include "anki/cozmo/shared/factory/faultCodes.h"

#include "proto/external_interface/shared.pb.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

#include "aiComponent/beiConditions/conditions/conditionEyeContact.h"

#include "opencv2/highgui/highgui.hpp"

#define LOG_CHANNEL "VisionComponent"

namespace Anki {
namespace Vector {

  VisionComponent* s_VisionComponent = nullptr;

#if ANKI_CPU_PROFILER_ENABLED
  CONSOLE_VAR_ENUM(u8, kVisionComponent_Logging,   ANKI_CPU_CONSOLEVARGROUP, 0, Util::CpuProfiler::CpuProfilerLogging());
#endif

  // Whether or not to send 'debug' images
  CONSOLE_VAR(bool, kSendDebugImages,  "Vision.General", true);

  CONSOLE_VAR(bool, kSendUndistortedImages, "Vision.General", false);

  // Whether or not to do rolling shutter correction for physical robots
  CONSOLE_VAR(bool, kRollingShutterCorrectionEnabled, "Vision.PreProcessing", true);
  CONSOLE_VAR(f32,  kMinCameraGain,                   "Vision.PreProcessing", 0.1f);

  // Set to a value greater than 0 to randomly drop that fraction of frames, for testing
  CONSOLE_VAR_RANGED(f32, kSimulateDroppedFrameFraction, "Vision.General", 0.f, 0.f, 1.f); // DO NOT COMMIT > 0!

  CONSOLE_VAR(bool, kVisualizeObservedMarkersIn3D,  "Vision.General", false);
  // If > 0, displays detected marker names in Viz Camera Display (still at fixed scale) and
  CONSOLE_VAR(bool,  kDisplayMarkerNames,           "Vision.General", false);

  // TODO: Is there any way to move this into VisionSystem? (Unlikely, since it needs FaceWorld)
  CONSOLE_VAR(bool, kDisplayEyeContactInMirrorMode, "Vision.General", false);

  // Hack to continue drawing salient points for a bit after they are detected
  // since neural nets run slowly. If zero, just draw until the next salient point
  // detection comes in.
  CONSOLE_VAR(u32, kKeepDrawingSalientPointsFor_ms, "Vision.General", 0);

  // Prints warning if haven't captured valid frame in this amount of time.
  // This is dependent on how fast we can process an image
  CONSOLE_VAR(u32, kMaxExpectedTimeBetweenCapturedFrames_ms, "Vision.General", 500);

  void DebugEraseAllEnrolledFaces(ConsoleFunctionContextRef context)
  {
    LOG_INFO("VisionComponent.ConsoleFunc","DebugEraseAllEnrolledFaces function called");
    s_VisionComponent->EraseAllFaces();
  }
  CONSOLE_FUNC(DebugEraseAllEnrolledFaces, "Vision.General");

  void DebugCaptureOneImage(ConsoleFunctionContextRef context)
  {
    LOG_INFO("VisionComponent.ConsoleFunc","Capture one image");
    s_VisionComponent->CaptureOneFrame();
  }
  CONSOLE_FUNC(DebugCaptureOneImage, "Vision.General");

  void DebugToggleCameraEnabled(ConsoleFunctionContextRef context)
  {
    static bool enable = false;
    LOG_INFO("VisionComponent.ConsoleFunc","Camera %s", (enable ? "enabled" : "disabled"));
    s_VisionComponent->EnableImageCapture(enable);
    enable = !enable;
  }
  CONSOLE_FUNC(DebugToggleCameraEnabled, "Vision.General");

  namespace JsonKey
  {
    const char * const ImageQualityGroup = "ImageQuality";
    const char * const PerformanceLogging = "PerformanceLogging";
    const char * const DropStatsWindowLength = "DropStatsWindowLength_sec";
    const char * const ImageQualityAlertDuration = "TimeBeforeErrorMessage_ms";
    const char * const ImageQualityAlertSpacing = "RepeatedErrorMessageInterval_ms";
    const char * const InitialExposureTime = "InitialExposureTime_ms";
    const char * const OpenCvThreadMode = "NumOpenCvThreads";
    const char * const FaceAlbum = "FaceAlbum";
  }

  namespace
  {
    // These aren't actually constant, b/c they are loaded from configuration files,
    // but they are used like constants in the code.

    // How long to see bad image quality continuously before alerting
    TimeStamp_t kImageQualityAlertDuration_ms = 3000;

    // Time between sending repeated EngineErrorCodeMessages after first alert
    TimeStamp_t kImageQualityAlertSpacing_ms = 5000;

    u16 kInitialExposureTime_ms = 16;

    const char* const kDefaultFaceAlbumName = "default";
  }

  VisionComponent::VisionComponent()
  : IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::Vision)
  {
  } // VisionSystem()


  void VisionComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
  {
    s_VisionComponent = this;
    _robot = robot;
    _context = _robot->GetContext();
    _vizManager = _context->GetVizManager();
    _camera = std::make_unique<Vision::Camera>(_robot->GetID());
    _visionSystem = new VisionSystem(_context);

    // Set up event handlers
    if(nullptr != _context && nullptr != _context->GetExternalInterface())
    {
      using namespace ExternalInterface;
      auto helper = MakeAnkiEventUtil(*_context->GetExternalInterface(), *this, _signalHandles);

      // In alphabetical order:
      helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableColorImages>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableVisionMode>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::EraseAllEnrolledFaces>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::EraseEnrolledFaceByID>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestEnrolledNames>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::LoadFaceAlbumFromFile>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SaveFaceAlbumToFile>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::UpdateEnrolledFaceByID>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::VisionRunMode>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::VisionWhileMoving>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SetCameraSettings>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SaveImages>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SetCameraCaptureFormat>();

      // Separate list for engine messages to listen to:
      helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotConnectionResponse>();
    }

    // "Special" viz identifier for the main camera feed
    _vizDisplayIndexMap["camera"] = 0;

    auto& context = dependentComps.GetComponent<ContextWrapper>().context;

    if (nullptr != context->GetDataPlatform())
    {
      const Json::Value& config = context->GetDataLoader()->GetRobotVisionConfig();
      ReadVisionConfig(config);

      if(!config.isMember("InitialModeSchedules"))
      {
        LOG_ERROR("VisionComponent.InitDependent.MissingInitialModeSchedulesConfigField", "");
      }

      const Json::Value& modeSchedulesConfig = config["InitialModeSchedules"];
      const Result result = AllVisionModesSchedule::SetDefaultSchedulesFromJSON(modeSchedulesConfig);
      if(RESULT_OK != result) {
        LOG_ERROR("VisionComponent.InitDependent.FailedToInitializeDefaultModeSchedules", "");
      }
    }

    CameraService::getInstance()->RegisterOnCameraRestartCallback([this](){
        if(IsWaitingForCaptureFormatChange())
        {
          _captureFormatState = CaptureFormatState::None;
        }
      });

    SetLiftCrossBar();

    SetupVisionModeConsoleVars();
  }

  void VisionComponent::ReadVisionConfig(const Json::Value& config)
  {
    _isInitialized = false;

    // Helper macro for grabbing a parameter from Json config and printing an
    // error message and returning failure if it doesn't exist
#   define GET_JSON_PARAMETER(__json__, __fieldName__, __variable__) \
    do { \
      if(!JsonTools::GetValueOptional(__json__, __fieldName__, __variable__)) { \
        LOG_ERROR("Vision.Init.MissingJsonParameter", "%s", __fieldName__); \
        return; \
    }} while(0)

    GET_JSON_PARAMETER(config, JsonKey::OpenCvThreadMode, _openCvNumThreads);

    const Json::Value& imageQualityConfig = config[JsonKey::ImageQualityGroup];
    GET_JSON_PARAMETER(imageQualityConfig, JsonKey::ImageQualityAlertDuration, kImageQualityAlertDuration_ms);
    GET_JSON_PARAMETER(imageQualityConfig, JsonKey::ImageQualityAlertSpacing,  kImageQualityAlertSpacing_ms);
    GET_JSON_PARAMETER(imageQualityConfig, JsonKey::InitialExposureTime,       kInitialExposureTime_ms);

    f32 kDropStatsWindowLength_sec = -1.f;
    const Json::Value& performanceConfig = config[JsonKey::PerformanceLogging];
    GET_JSON_PARAMETER(performanceConfig, JsonKey::DropStatsWindowLength, kDropStatsWindowLength_sec);

    Result result = _visionSystem->Init(config);
    if(RESULT_OK != result) {
      LOG_ERROR("VisionComponent.Init.VisionSystemInitFailed", "");
      return;
    }


    // Load face album and broadcast all known faces
    {
      _faceAlbumName = kDefaultFaceAlbumName;
      JsonTools::GetValueOptional(config, JsonKey::FaceAlbum, _faceAlbumName);
      result = LoadFaceAlbum(); // NOTE: Also broadcasts any loaded faces

      if(RESULT_OK != result) {
        LOG_WARNING("VisionComponent.Init.LoadFaceAlbumFromFileFailed",
                    "AlbumFile: %s", _faceAlbumName.c_str());
      }
    }

    _isInitialized = true;
  } //ReadVisionConfig()

  void VisionComponent::SetCameraCalibration(std::shared_ptr<Vision::CameraCalibration> camCalib)
  {
    const bool calibChanged = _camera->SetCalibration(camCalib);
    if(calibChanged)
    {
      // Stop the visionSystem before updating calibration
      // New calibration causes MarkerDetector's embedded memory to be reset
      // which could happen while it is running so we need to stop it first
      if(!_isSynchronous)
      {
        Stop();
      }

      Lock();
      _visionSystem->UpdateCameraCalibration(camCalib);
      Unlock();

      if(!_isSynchronous)
      {
        Start();
      }

      // Got a new calibration: rebuild the LUT for ground plane homographies
      PopulateGroundPlaneHomographyLUT();
    }

    if(kRollingShutterCorrectionEnabled)
    {
      _visionSystem->ShouldDoRollingShutterCorrection(_robot->IsPhysical());
    }
    else
    {
      _visionSystem->ShouldDoRollingShutterCorrection(false);
      EnableVisionWhileRotatingFast(false);
    }
  } // SetCameraCalibration()


  void VisionComponent::SetIsSynchronous(bool isSynchronous) {
    if(isSynchronous && !_isSynchronous) {
      LOG_INFO("VisionComponent.SetSynchronousMode.SwitchToSync", "");
      if(_running) {
        Stop();
      }
      _isSynchronous = true;
    }
    else if(!isSynchronous && _isSynchronous) {
      LOG_INFO("VisionComponent.SetSynchronousMode.SwitchToAsync", "");
      _isSynchronous = false;
      Start();
    }
    _visionSystem->SetFaceRecognitionIsSynchronous(_isSynchronous);
  }

  void VisionComponent::Start()
  {
    if(!IsCameraCalibrationSet()) {
      LOG_ERROR("VisionComponent.Start",
                "Camera calibration must be set to start VisionComponent.");
      return;
    }

    if(_running) {
      LOG_INFO("VisionComponent.Start.Restarting",
                       "Thread already started, calling Stop() and then restarting (paused:%d).",
                       _paused);
      Stop();
    } else {
      LOG_INFO("VisionComponent.Start",
                       "Starting vision processing thread (paused:%d)",
                       _paused);
    }

    _running = true;
    // Note that we're giving the Processor a pointer to "this", so we
    // have to ensure this VisionSystem object outlives the thread.
    _processingThread = std::thread(&VisionComponent::Processor, this);
  }

  void VisionComponent::Stop()
  {
    {
      std::unique_lock<std::mutex> lock{_imageReadyMutex};
      _running = false;
    }
    _imageReadyCondition.notify_all();

    // Wait for processing thread to die before destructing since we gave it
    // a reference to *this
    if(_processingThread.joinable()) {
      _processingThread.join();
    }
  }


  VisionComponent::~VisionComponent()
  {
    Vision::Image::CloseAllDisplayWindows();
    Vision::ImageRGB::CloseAllDisplayWindows();

    Stop();

    Util::SafeDelete(_visionSystem);

    CameraService::removeInstance();

    #if REMOTE_CONSOLE_ENABLED
    for(auto& iter : _visionModeConsoleVars)
    {
      Util::SafeDelete(iter.first);
    }
    #endif
  } // ~VisionSystem()

  RobotTimeStamp_t VisionComponent::GetLastProcessedImageTimeStamp() const
  {
    return _lastProcessedImageTimeStamp_ms;
  }

  void VisionComponent::Lock()
  {
    _lock.lock();
  }

  void VisionComponent::Unlock()
  {
    _lock.unlock();
  }

  Result VisionComponent::EnableMode(VisionMode mode, bool enable)
  {
    if(enable)
    {
      VisionModeRequest request{.mode = mode,
                                .frequency = EVisionUpdateFrequency::High};

      _robot->GetVisionScheduleMediator().AddAndUpdateVisionModeSubscriptions(this, {request});
    }
    else
    {
      _robot->GetVisionScheduleMediator().RemoveVisionModeSubscriptions(this, {mode});
    }
    return RESULT_OK;
  }

  static Result GetImageHistState(const Robot&           robot,
                                  const RobotTimeStamp_t imageTimeStamp,
                                  HistRobotState&        imageHistState,
                                  RobotTimeStamp_t&      imageHistTimeStamp)
  {
    // Handle the (rare, Webots-test-only?) possibility that the image timstamp is _newer_
    // than the latest thing in history. In that case, we'll just use the last pose information
    // we have, since we can't really interpolate.
    const RobotTimeStamp_t requestedTimeStamp = std::min(imageTimeStamp, robot.GetStateHistory()->GetNewestTimeStamp());

    Result lastResult = robot.GetStateHistory()->ComputeStateAt(requestedTimeStamp, imageHistTimeStamp, imageHistState, true);

    return lastResult;
  }

  void VisionComponent::UpdateDependent(const RobotCompMap& dependentComps)
  {
    if(!_isInitialized) {
      LOG_WARNING("VisionComponent.Update.NotInitialized", "");
      return;
    }

    if (!_enabled) {
      LOG_PERIODIC_INFO(200, "VisionComponent.Update.NotEnabled", "If persistent, camera calibration is probably missing");
      return;
    }

    if(!IsCameraCalibrationSet())
    {
      LOG_WARNING("VisionComponent.Update.NoCameraCalibration",
                  "Camera calibration should be set before calling UpdateDependent().");
      return;
    }

    UpdateVisionModeConsoleVars();

    // Check and update any results from VisionSystem
    UpdateAllResults();

    UpdateCaptureFormatChange();

    // If we don't yet have an image to process, we need to capture one
    if(!_visionSystemInput.locked)
    {
      Vision::ImageBuffer& buffer = _visionSystemInput.imageBuffer;
      const bool gotImage = CaptureImage(buffer);
      _hasStartedCapturingImages = true;

      UpdateImageReceivedChecks(gotImage);

      if(!gotImage)
      {
        LOG_DEBUG("VisionComponent.Update.WaitingForBufferedImage", "Tick:%zu",
                  BaseStationTimer::getInstance()->GetTickCount());
        return;
      }

      // After this point we know we have captured an image

      UpdateCaptureFormatChange(buffer.GetNumRows());

      if(CaptureFormatState::WaitingForFrame == _captureFormatState)
      {
        return;
      }

      // Track how fast we are receiving frames
      if(_lastReceivedImageTimeStamp_ms > 0)
      {
        // Time should not move backwards!
        const bool timeWentBackwards = buffer.GetTimestamp() < _lastReceivedImageTimeStamp_ms;
        if (timeWentBackwards)
        {
          LOG_WARNING("VisionComponent.SetNextImage.UnexpectedTimeStamp",
                      "Current:%u Last:%u",
                      buffer.GetTimestamp(), (TimeStamp_t)_lastReceivedImageTimeStamp_ms);

          // This should be recoverable (it could happen if we receive a bunch of garbage image data)
          // so reset the lastReceived and lastProcessed timestamps so we can set them fresh next time
          // we get an image
          _lastReceivedImageTimeStamp_ms = 0;
          _lastProcessedImageTimeStamp_ms = 0;
          ReleaseImage(buffer);
          return;
        }
        _framePeriod_ms = (TimeStamp_t)(buffer.GetTimestamp() - _lastReceivedImageTimeStamp_ms);
      }
      _lastReceivedImageTimeStamp_ms = buffer.GetTimestamp();

      // Try to get the corresponding historical state
      const bool imageOlderThanOldestState = (buffer.GetTimestamp() < _robot->GetStateHistory()->GetOldestTimeStamp());
      if(imageOlderThanOldestState)
      {
        // Special case: we're trying to process an image with a timestamp older than the oldest thing in
        // state history. This can happen at startup, or possibly when we delocalize and clear state
        // history. Just drop this image.
        LOG_INFO("VisionComponent.Update.DroppingImageOlderThanStateHistory",
                 "ImageTime=%d OldestState=%d",
                 buffer.GetTimestamp(), (TimeStamp_t)_robot->GetStateHistory()->GetOldestTimeStamp());

        ReleaseImage(buffer);
        return;
      }

      // Do we have anything in state history at least as new as this image yet?
      const bool haveHistStateAtLeastAsNewAsImage = (_robot->GetStateHistory()->GetNewestTimeStamp() >= buffer.GetTimestamp());
      if(!haveHistStateAtLeastAsNewAsImage)
      {
        LOG_INFO("VisionComponent.Update.WaitingForState",
                 "CapturedImageTime:%u NewestStateInHistory:%u",
                 buffer.GetTimestamp(), (TimeStamp_t)_robot->GetStateHistory()->GetNewestTimeStamp());
  
        ReleaseImage(buffer);
        return;
      }

      const Result res = SetNextImage(buffer);
      if(res != RESULT_OK)
      {
        ReleaseImage(buffer);
      }
    }
  }

  Result VisionComponent::SetNextImage(Vision::ImageBuffer& buffer)
  {
    // Can't set a new image while we are still processing one
    DEV_ASSERT(!_visionSystemInput.locked, "VisionComponent.SetNextImage.AlreadyProcessingImage");

    // Fill in the pose data for the given image, by querying robot history
    HistRobotState imageHistState;
    RobotTimeStamp_t imageHistTimeStamp;

    Result lastResult = GetImageHistState(*_robot, buffer.GetTimestamp(), imageHistState, imageHistTimeStamp);

    if(lastResult == RESULT_FAIL_ORIGIN_MISMATCH)
    {
      // Don't print a warning for this case: we expect not to get pose history
      // data successfully
      LOG_INFO("VisionComponent.SetNextImage.OriginMismatch",
               "Could not get pose data for t=%u due to origin mismatch. Returning OK",
               buffer.GetTimestamp());
      ReleaseImage(buffer);
      return RESULT_OK;
    }
    else if(lastResult != RESULT_OK)
    {
      LOG_WARNING("VisionComponent.SetNextImage.StateHistoryFail",
                  "Unable to get computed pose at image timestamp of %u."
                  "(rawStates: have %zu from %u:%u) (visionStates: have %zu from %u:%u)",
                  buffer.GetTimestamp(),
                  _robot->GetStateHistory()->GetNumRawStates(),
                  (TimeStamp_t)_robot->GetStateHistory()->GetOldestTimeStamp(),
                  (TimeStamp_t)_robot->GetStateHistory()->GetNewestTimeStamp(),
                  _robot->GetStateHistory()->GetNumVisionStates(),
                  (TimeStamp_t)_robot->GetStateHistory()->GetOldestVisionOnlyTimeStamp(),
                  (TimeStamp_t)_robot->GetStateHistory()->GetNewestVisionOnlyTimeStamp());
      ReleaseImage(buffer);
      return lastResult;
    }

    const Pose3d& cameraPose = _robot->GetHistoricalCameraPose(imageHistState, imageHistTimeStamp);
    Matrix_3x3f groundPlaneHomography;
    const bool groundPlaneVisible = LookupGroundPlaneHomography(imageHistState.GetHeadAngle_rad(),
                                                                groundPlaneHomography);

    _visionSystemInput.poseData.Set(imageHistTimeStamp,
                                    imageHistState,
                                    cameraPose,
                                    groundPlaneVisible,
                                    groundPlaneHomography,
                                    _robot->GetImuComponent().GetImuHistory());


    // Experimental:
    //UpdateOverheadMap(image, _visionSystemInput.poseData);

    UpdateForCalibration();

    if(_paused)
    {
      _vizManager->SetText(TextLabelType::VISION_MODE, NamedColors::CYAN,
                           "Vision: <PAUSED>");
      // Won't be processing the image because we are paused
      // so we need to release it
      ReleaseImage(buffer);
      return RESULT_OK;
    }

    // We just wanted to capture one image so disable image capture now that
    // the one image has been captured and is going to be processed
    if(_captureOneImage)
    {
      _captureOneImage = false;
      EnableImageCapture(false);
    }

    _visionSystemInput.modesToProcess.Clear();
    _visionSystemInput.futureModesToProcess.Clear();

    static u32 scheduleCount = 0;
    const AllVisionModesSchedule& schedule = _robot->GetVisionScheduleMediator().GetSchedule();
    for(VisionMode mode = VisionMode(0); mode < VisionMode::Count; mode++)
    {
      _visionSystemInput.modesToProcess.Enable(mode, schedule.IsTimeToProcess(mode, scheduleCount));
      _visionSystemInput.futureModesToProcess.Enable(mode, schedule.GetScheduleForMode(mode).WillEverRun());
    }
    const bool kResetSingleShotModes = true;
    _robot->GetVisionScheduleMediator().AddSingleShotModesToSet(_visionSystemInput.modesToProcess, kResetSingleShotModes);
    scheduleCount++;

    // We are all set to process this image so lock input so VisionSystem can use it;
    // then notify the processing thread that the image is ready to be processed
    {
      std::unique_lock<std::mutex> lock{_imageReadyMutex};
      _visionSystemInput.locked = true;
    }
    _imageReadyCondition.notify_all();
  
    if(_isSynchronous)
    {
      // Process image now
      UpdateVisionSystem(_visionSystemInput);
      ReleaseImage(buffer);

      // Unlock input since it has been processed
      _visionSystemInput.locked = false;
    }

    return RESULT_OK;

  } // SetNextImage()


  void VisionComponent::PopulateGroundPlaneHomographyLUT(f32 angleResolution_rad)
  {
    const Pose3d& robotPose = _robot->GetPose();

    DEV_ASSERT(_camera->IsCalibrated(), "VisionComponent.PopulateGroundPlaneHomographyLUT.CameraNotCalibrated");

    const auto calibration = _camera->GetCalibration();
    const Matrix_3x3f K = calibration->GetCalibrationMatrix();

    GroundPlaneROI groundPlaneROI;

    // Loop over all possible head angles at the specified resolution and store
    // the ground plane homography for each.
    const f32 headAngleSlop_rad = DEG_TO_RAD(2);
    for(f32 headAngle_rad = MIN_HEAD_ANGLE-headAngleSlop_rad; // Start a little below min angle to account for slop
        headAngle_rad <= MAX_HEAD_ANGLE+headAngleSlop_rad;    // End a little above max angle to account for slop
        headAngle_rad += angleResolution_rad)
    {
      // Get the robot origin w.r.t. the camera position with the camera at
      // the current head angle
      const Pose3d& cameraPose = _robot->GetCameraPose(headAngle_rad); // need to store the camera pose so it can be parent
      Pose3d robotPoseWrtCamera;
      const bool result = robotPose.GetWithRespectTo(cameraPose, robotPoseWrtCamera);
      // this really shouldn't fail! camera has to be in the robot's pose tree
      DEV_ASSERT(result == true, "VisionComponent.PopulateGroundPlaneHomographyLUT.GetWrtFailed");
#     pragma unused(result) // Avoid errors in release/shipping when assert compiles out

      const RotationMatrix3d& R = robotPoseWrtCamera.GetRotationMatrix();
      const Vec3f&            T = robotPoseWrtCamera.GetTranslation();

      // Construct the homography mapping points on the ground plane into the
      // image plane
      const Matrix_3x3f H = K*Matrix_3x3f{R.GetColumn(0),R.GetColumn(1),T};

      Quad2f imgQuad;
      groundPlaneROI.GetImageQuad(H, calibration->GetNcols(), calibration->GetNrows(), imgQuad);

      const bool isRoiVisible = (_camera->IsWithinFieldOfView(imgQuad[Quad::CornerName::TopLeft]) ||
                                 _camera->IsWithinFieldOfView(imgQuad[Quad::CornerName::BottomLeft]));
      _groundPlaneHomographyLUT[headAngle_rad] = {H, isRoiVisible};
    }

  } // PopulateGroundPlaneHomographyLUT()

  bool VisionComponent::LookupGroundPlaneHomography(f32 atHeadAngle, Matrix_3x3f& H) const
  {
    if(!ANKI_VERIFY(!_groundPlaneHomographyLUT.empty(), "VisionComponent.LookupGroundPlaneHomography.EmptyLUT", ""))
    {
      return false;
    }

    if(atHeadAngle > _groundPlaneHomographyLUT.rbegin()->first) {
      // Head angle too large
      return false;
    }

    auto iter = _groundPlaneHomographyLUT.lower_bound(atHeadAngle);

    if(iter == _groundPlaneHomographyLUT.end()) {
      LOG_WARNING("VisionComponent.LookupGroundPlaneHomography.KeyNotFound",
                  "Failed to find homography using headangle of %.2frad (%.1fdeg) as lower bound",
                  atHeadAngle, RAD_TO_DEG(atHeadAngle));
      --iter;
    } else {
      auto nextIter = iter; ++nextIter;
      if(nextIter != _groundPlaneHomographyLUT.end()) {
        if(std::abs(atHeadAngle - iter->first) > std::abs(atHeadAngle - nextIter->first)) {
          iter = nextIter;
        }
      }
    }

    //      LOG_DEBUG("VisionComponent.LookupGroundPlaneHomography.HeadAngleDiff",
    //                "Requested = %.2fdeg, Returned = %.2fdeg, Diff = %.2fdeg",
    //                RAD_TO_DEG(atHeadAngle), RAD_TO_DEG(iter->first),
    //                RAD_TO_DEG(std::abs(atHeadAngle - iter->first)));

    H = iter->second.H;
    return iter->second.isGroundPlaneROIVisible;

  } // LookupGroundPlaneHomography()

  void VisionComponent::UpdateVisionSystem(const VisionSystemInput& input)
  {
    ANKI_CPU_PROFILE("VC::UpdateVisionSystem");

    Result result = _visionSystem->Update(input);
    if(RESULT_OK != result) {
      LOG_WARNING("VisionComponent.UpdateVisionSystem.UpdateFailed", "");
    }

    // VisionMode::Calibration is a one-shot mode, turn it off
    // as soon as it runs
    if(input.modesToProcess.Contains(VisionMode::Calibration))
    {
      EnableMode(VisionMode::Calibration, false);
    }
  }


  void VisionComponent::Processor()
  {
    LOG_INFO("VisionComponent.Processor",
             "Starting Robot VisionComponent::Processor thread...");

    DEV_ASSERT(_visionSystem != nullptr && _visionSystem->IsInitialized(),
               "VisionComponent.Processor.VisionSystemNotReady");

    Anki::Util::SetThreadName(pthread_self(), "VisionSystem");

    // Disable threading by OpenCV
    Result cvResult = SetNumOpencvThreads( _openCvNumThreads, "VisionComponent.Processor" );
    if( RESULT_OK != cvResult )
    {
      return;
    }

    while (_running)
    {
      ANKI_CPU_TICK("VisionComponent", 100.0, Util::CpuProfiler::CpuProfilerLoggingTime(kVisionComponent_Logging));

      // If we have something to process
      if(_visionSystemInput.locked)
      {
        // Sanity check that imageBuffer data is valid
        if(_visionSystemInput.imageBuffer.HasValidData())
        {
          ANKI_CPU_PROFILE("ProcessImage");

          // There is an image to be processed; do so now
          UpdateVisionSystem(_visionSystemInput);

          // Clear it when done.
          ReleaseImage(_visionSystemInput.imageBuffer);
        }
        else
        {
          LOG_WARNING("VisionComponent.Processor.ImageReadyButDataInvalid","");
        }

        // Done processing, allow input to be modified by VisionComponent
        _visionSystemInput.locked = false;
      }

      {
        ANKI_CPU_PROFILE("WaitForNextImage");
        // Waiting on next image
        std::unique_lock<std::mutex> lock{_imageReadyMutex};
        _imageReadyCondition.wait(lock, [this]{ return _visionSystemInput.locked || !_running; });
      }
    } // while(_running)

    ANKI_CPU_REMOVE_THIS_THREAD();

    LOG_INFO("VisionComponent.Processor.TerminatedVisionSystemThread",
             "Terminated VisionComponent::Processor thread");
  } // Processor()


  void VisionComponent::VisualizeObservedMarkerIn3D(const Vision::ObservedMarker& marker) const
  {
    // Note that this incurs extra computation to compute the 3D pose of
    // each observed marker so that we can draw in the 3D world, but this is
    // purely for debug / visualization
    u32 quadID = 0;

    // Block Markers
    BlockWorldFilter filter;

    filter.AddFilterFcn([](const ObservableObject* object)
    {
      const auto& objType = object->GetType();
      return (IsBlockType(objType, false) || IsChargerType(objType, false));
    });

    filter.AddFilterFcn([&marker,&quadID,this](const ObservableObject* object)
    {
      // When requesting the markers' 3D corners below, we want them
      // not to be relative to the object the marker is part of, so we
      // will request them at a "canonical" pose (no rotation/translation)
      const Pose3d canonicalPose;

      const auto markersWithCode = object->GetMarkersWithCode(marker.GetCode());
      for(auto& blockMarker : markersWithCode)
      {
        Pose3d markerPose;
        Result poseResult = marker.GetSeenBy().ComputeObjectPose(marker.GetImageCorners(),
                                                                 blockMarker->Get3dCorners(canonicalPose),
                                                                 markerPose);
        if(poseResult != RESULT_OK) {
          LOG_WARNING("VisionComponent.VisualizeObservedMarkerIn3D.BadPose",
                      "Could not estimate pose of marker. Not visualizing.");
        } else {
          if(markerPose.GetWithRespectTo(marker.GetSeenBy().GetPose().FindRoot(), markerPose) == true) {
            _robot->GetContext()->GetVizManager()->DrawGenericQuad(quadID++, blockMarker->Get3dCorners(markerPose), NamedColors::OBSERVED_QUAD);
          } else {
            LOG_WARNING("VisionComponent.VisualizeObservedMarkerIn3D.MarkerOriginNotCameraOrigin",
                        "Cannot visualize a marker whose pose origin is not the camera's origin that saw it.");
          }
        }
      }

      return true;
    });

    _robot->GetBlockWorld().FindLocatedMatchingObject(filter);

  } // VisualizeObservedMarkerIn3D()


  Result VisionComponent::UpdateAllResults()
  {
    ANKI_CPU_PROFILE("VC::UpdateAllResults");

    bool anyFailures = false;

    if(_visionSystem != nullptr)
    {
      VisionProcessingResult result;

      while(true == _visionSystem->CheckMailbox(result))
      {
        if((_vizManager != nullptr) &&
           _vizManager->IsConnected())
        {
          // Send processed modes to viz
          VizInterface::EnabledVisionModes evm;
          std::copy(result.modesProcessed.begin(), result.modesProcessed.end(),
                    std::back_inserter(evm.modes));
          _vizManager->SendEnabledVisionModes(std::move(evm));
        }

        SendImages(result);

        using LocalHandlerType = Result(VisionComponent::*)(const VisionProcessingResult&);
        auto tryAndReport = [this, &result, &anyFailures]( LocalHandlerType handler, const VisionModeSet& modes )
        {
          if(!modes.IsEmpty())
          {
            VisionModeSet intersection = result.modesProcessed.Intersect(modes);
            if( intersection.IsEmpty() )
            {
              return;
            }
          }

          // Call the passed in member handler to look at the result
          if (RESULT_OK != (this->*handler)(result))
          {
            std::string modeStr = modes.ToString();
            
            LOG_ERROR("VisionComponent.UpdateAllResults.LocalHandlerFailed",
                      "For mode(s):%s", modeStr.c_str());
            anyFailures = true;
          }
        };

        // NOTE: UpdateVisionMarkers will also update BlockWorld (which broadcasts
        //  object observations and should be done before sending RobotProcessedImage below!)
        tryAndReport(&VisionComponent::UpdateVisionMarkers,        {VisionMode::Markers});

        // NOTE: UpdateFaces will also update FaceWorld (which broadcasts face observations
        //  and should be done before sending RobotProcessedImage below!)
        tryAndReport(&VisionComponent::UpdateFaces,                {VisionMode::Faces});

        // NOTE: UpdatePets will also update PetWorld (which broadcasts pet face observations
        //  and should be done before sending RobotProcessedImage below!)
        tryAndReport(&VisionComponent::UpdatePets,                 {VisionMode::Pets});

        tryAndReport(&VisionComponent::UpdateMotionCentroid,       {VisionMode::Motion});
        tryAndReport(&VisionComponent::UpdateOverheadEdges,        {VisionMode::OverheadEdges});
        tryAndReport(&VisionComponent::UpdateComputedCalibration,  {VisionMode::Calibration});

        // NOTE: Same handler for two modes
        tryAndReport(&VisionComponent::UpdateCameraParams,         {VisionMode::AutoExp, VisionMode::WhiteBalance});

        tryAndReport(&VisionComponent::UpdateLaserPoints,          {VisionMode::Lasers});
        tryAndReport(&VisionComponent::UpdateSalientPoints,        {}); // Use empty set here to always call UpdateSalientPoints
        tryAndReport(&VisionComponent::UpdateVisualObstacles,      {VisionMode::Obstacles});
        tryAndReport(&VisionComponent::UpdatePhotoManager,         {VisionMode::SaveImages});
        tryAndReport(&VisionComponent::UpdateDetectedIllumination, {VisionMode::Illumination});

        // Note: we always run this because it handles switching to the mirror mode debug screen
        // It internally checks whether the MirrorMode flag is set in modesProcessed
        tryAndReport(&VisionComponent::UpdateMirrorMode,           {}); // Use empty set to always run

#       undef ToVisionModeMask

        // Store frame rate and last image processed time. Time should only move forward.
        DEV_ASSERT(result.timestamp >= _lastProcessedImageTimeStamp_ms, "VisionComponent.UpdateAllResults.BadTimeStamp");
        if(_lastProcessedImageTimeStamp_ms != 0)
        {
          _processingPeriod_ms = (TimeStamp_t)(result.timestamp - _lastProcessedImageTimeStamp_ms);
        }
        _lastProcessedImageTimeStamp_ms = result.timestamp;

        auto visionModesList = std::vector<VisionMode>();
        std::copy(result.modesProcessed.begin(), result.modesProcessed.end(),
                  std::back_inserter(visionModesList));

        // Send the processed image message last
        {
          using namespace ExternalInterface;

          u8 imageMean = 0;
          if(result.modesProcessed.Contains(VisionMode::Stats))
          {
            imageMean = result.imageMean;
          }

          _robot->Broadcast(MessageEngineToGame(RobotProcessedImage((TimeStamp_t)result.timestamp,
                                                                   std::move(visionModesList),
                                                                   imageMean)));
        }
      }
    }

    if(anyFailures) {
      return RESULT_FAIL;
    } else {
      return RESULT_OK;
    }
  } // UpdateAllResults()

  Result VisionComponent::UpdateVisionMarkers(const VisionProcessingResult& procResult)
  {
    Result lastResult = RESULT_OK;

    std::list<Vision::ObservedMarker> observedMarkers;

    if(!procResult.observedMarkers.empty())
    {
      // Get historical robot pose at this processing result's timestamp to get
      // head angle and to attach as parent of the camera pose.
      RobotTimeStamp_t t;
      HistRobotState* histStatePtr = nullptr;
      HistStateKey histStateKey;

      lastResult = _robot->GetStateHistory()->ComputeAndInsertStateAt(procResult.timestamp, t, &histStatePtr, &histStateKey, true);

      if(RESULT_FAIL_ORIGIN_MISMATCH == lastResult)
      {
        // Not finding pose information due to an origin mismatch is a normal thing
        // if we just delocalized, so just report everything's cool
        return RESULT_OK;
      }
      else if(RESULT_OK != lastResult)
      {
        // this can happen if we missed a robot status update message
        LOG_INFO("VisionComponent.UpdateVisionMarkers.HistoricalPoseNotFound",
                 "Time: %u, hist: %u to %u",
                 (TimeStamp_t)procResult.timestamp,
                 (TimeStamp_t)_robot->GetStateHistory()->GetOldestTimeStamp(),
                 (TimeStamp_t)_robot->GetStateHistory()->GetNewestTimeStamp());
        return RESULT_OK;
      }

      if(!_robot->IsPoseInWorldOrigin(histStatePtr->GetPose())) {
        LOG_INFO("VisionComponent.UpdateVisionMarkers.OldOrigin",
                 "Ignoring observed marker from origin %s (robot origin is %s)",
                 histStatePtr->GetPose().FindRoot().GetName().c_str(),
                 _robot->GetWorldOrigin().GetName().c_str());
        return RESULT_OK;
      }

      // If we get here, ComputeAndInsertPoseIntoHistory() should have succeeded
      // and this should be true
      assert(procResult.timestamp == t);

      Vision::Camera histCamera = _robot->GetHistoricalCamera(*histStatePtr, procResult.timestamp);

      // Note: we deliberately make a copy of the vision markers in observedMarkers
      // as we loop over them here, because procResult is const but we want to modify
      // each marker to hook up its camera to pose history
      for(auto visionMarker : procResult.observedMarkers)
      {
        if(visionMarker.GetTimeStamp() != procResult.timestamp)
        {
          LOG_ERROR("VisionComponent.UpdateVisionMarkers.MismatchedTimestamps",
                    "Marker t=%u vs. ProcResult t=%u",
                    visionMarker.GetTimeStamp(), (TimeStamp_t)procResult.timestamp);
          continue;
        }

        // Remove observed markers whose historical poses have become invalid.
        // This shouldn't happen! If it does, robotStateMsgs may be buffering up somewhere.
        // Increasing history time window would fix this, but it's not really a solution.
        if ((visionMarker.GetSeenBy().GetID() == GetCamera().GetID()) &&
            !_robot->GetStateHistory()->IsValidKey(histStateKey))
        {
          LOG_WARNING("VisionComponent.Update.InvalidHistStateKey", "key=%d", histStateKey);
          continue;
        }

        // Update the marker's camera to use a pose from pose history, and
        // create a new marker with the updated camera
        visionMarker.SetSeenBy(histCamera);

        const Quad2f& corners = visionMarker.GetImageCorners();
        const ColorRGBA& drawColor = (visionMarker.GetCode() == Vision::MARKER_UNKNOWN ?
                                      NamedColors::BLUE : NamedColors::RED);
        _vizManager->DrawCameraQuad(corners, drawColor, NamedColors::GREEN);

        if(kDisplayMarkerNames)
        {
          Rectangle<f32> boundingRect(corners);
          std::string markerName(visionMarker.GetCodeName());
          _vizManager->DrawCameraText(boundingRect.GetTopLeft(),
                                      markerName.substr(strlen("MARKER_"),std::string::npos),
                                      drawColor);
        }

        if(kVisualizeObservedMarkersIn3D)
        {
          VisualizeObservedMarkerIn3D(visionMarker);
        }

        observedMarkers.push_back(std::move(visionMarker));
      }
    } // if(!procResult.observedMarkers.empty())

    lastResult = _robot->GetBlockWorld().UpdateObservedMarkers(observedMarkers);
    if(RESULT_OK != lastResult)
    {
      LOG_WARNING("VisionComponent.UpdateVisionResults.BlockWorldUpdateFailed", "");
    }

    // If we have observed a marker, attempt to update the docking error signal
    if(!observedMarkers.empty())
    {
      _robot->GetDockingComponent().UpdateDockingErrorSignal(procResult.timestamp);
    }

    return lastResult;

  } // UpdateVisionMarkers()

  Result VisionComponent::UpdateFaces(const VisionProcessingResult& procResult)
  {
    Result lastResult = RESULT_OK;

    for(auto & updatedID : procResult.updatedFaceIDs)
    {
      _robot->GetFaceWorld().ChangeFaceID(updatedID);
    }

    for(auto & faceDetection : procResult.faces)
    {
      /*
       LOG_INFO("VisionComponent.Update",
                        "Saw face at (x,y,w,h)=(%.1f,%.1f,%.1f,%.1f), "
                        "at t=%d Pose: roll=%.1f, pitch=%.1f yaw=%.1f, T=(%.1f,%.1f,%.1f).",
                        faceDetection.GetRect().GetX(), faceDetection.GetRect().GetY(),
                        faceDetection.GetRect().GetWidth(), faceDetection.GetRect().GetHeight(),
                        faceDetection.GetTimestamp(),
                        faceDetection.GetHeadRoll().getDegrees(),
                        faceDetection.GetHeadPitch().getDegrees(),
                        faceDetection.GetHeadYaw().getDegrees(),
                        faceDetection.GetHeadPose().GetTranslation().x(),
                        faceDetection.GetHeadPose().GetTranslation().y(),
                        faceDetection.GetHeadPose().GetTranslation().z());
       */

      // Check this before potentially ignoring the face detection for faceWorld's purposes below
      if(faceDetection.GetNumEnrollments() > 0) {
        LOG_DEBUG("VisionComponent.UpdateFaces.ReachedEnrollmentCount",
                  "Count=%d", faceDetection.GetNumEnrollments());

        _robot->GetFaceWorld().SetFaceEnrollmentComplete(true);
      }
    }

    lastResult = _robot->GetFaceWorld().Update(procResult.faces);
    if(RESULT_OK != lastResult)
    {
      LOG_WARNING("VisionComponent.UpdateFaces.FaceWorldUpdateFailed", "");
    }

    return lastResult;
  } // UpdateFaces()

  Result VisionComponent::UpdatePets(const VisionProcessingResult& procResult)
  {
    Result lastResult = _robot->GetPetWorld().Update(procResult.pets);
    return lastResult;
  }

  Result VisionComponent::UpdateMotionCentroid(const VisionProcessingResult& procResult)
  {
    for(auto motionCentroid : procResult.observedMotions) // deliberate copy
    {
      _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(motionCentroid)));
    }

    return RESULT_OK;
  } // UpdateMotionCentroid()

  Result VisionComponent::UpdateVisualObstacles(const VisionProcessingResult& procResult)
  {
    for(auto const& edgeFrame : procResult.visualObstacles)
    {
      _robot->GetMapComponent().AddDetectedObstacles(edgeFrame);
    }

    return RESULT_OK;
  }

  Result VisionComponent::UpdateLaserPoints(const VisionProcessingResult& procResult)
  {
    for(auto laserPoint : procResult.laserPoints) // deliberate copy: we move this to the message
    {
      _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(laserPoint)));
    }

    return RESULT_OK;
  } // UpdateMotionCentroid()

  Result VisionComponent::UpdateSalientPoints(const VisionProcessingResult& procResult)
  {
    EngineTimeStamp_t currentTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

    const bool usingFixedDrawTime = (kKeepDrawingSalientPointsFor_ms > 0);
    if(usingFixedDrawTime)
    {
      auto iter = _salientPointsToDraw.begin();
      while(iter != _salientPointsToDraw.end())
      {
        if(currentTime_ms > (iter->first + kKeepDrawingSalientPointsFor_ms)) {
          iter = _salientPointsToDraw.erase(iter);
        } else {
          ++iter;
        }
      }
    }

    const auto& visionModesUsingNeuralNets = GetVisionModesUsingNeuralNets();
    if(procResult.modesProcessed.ContainsAnyOf(visionModesUsingNeuralNets)
       || procResult.modesProcessed.Contains(VisionMode::BrightColors))
    {
      if(!usingFixedDrawTime)
      {
        _salientPointsToDraw.clear();
      }

      // Notify the SalientPointsComponent that we have a bunch of new detections
      if (procResult.salientPoints.size() > 0) {
        _robot->GetAIComponent().GetComponent<SalientPointsComponent>().AddSalientPoints(procResult.salientPoints);
      }
      for(auto const& salientPoint : procResult.salientPoints)
      {
        _salientPointsToDraw.emplace_back(currentTime_ms, salientPoint);
      }
    }

    s32 colorIndex = 0;
    for(auto const& unscaledSalientPointToDraw : _salientPointsToDraw)
    {
      auto salientPointToDraw = unscaledSalientPointToDraw;
      salientPointToDraw.second.x_img *= DEFAULT_CAMERA_RESOLUTION_WIDTH;
      salientPointToDraw.second.y_img *= DEFAULT_CAMERA_RESOLUTION_HEIGHT;
      for(auto &pt : salientPointToDraw.second.shape)
      {
        pt.x *= DEFAULT_CAMERA_RESOLUTION_WIDTH;
        pt.y *= DEFAULT_CAMERA_RESOLUTION_HEIGHT;
      }

      const auto& object = salientPointToDraw.second;

      const Poly2f poly(object.shape);
      ColorRGBA color(NamedColors::RED);
      std::string caption = "";

      switch(object.salientType)
      {
        case Vision::SalientPointType::BrightColors:
        {
          color = (object.color_rgba == 0) ? NamedColors::BLACK : ColorRGBA(object.color_rgba);
          caption = object.description + "[" + std::to_string((s32)std::round(object.score))
                    + "] t:" + std::to_string(object.timestamp);
          break;
        }
        default:
        {
          color = (object.description.empty()) ? NamedColors::RED : ColorRGBA::CreateFromColorIndex(colorIndex++);
          caption = object.description + "[" + std::to_string((s32)std::round(100.f*object.score))
                      + "] t:" + std::to_string(object.timestamp);
          break;
        }
      }
      _vizManager->DrawCameraPoly(poly, color);
      _vizManager->DrawCameraText(Point2f(object.x_img, object.y_img), caption, color);
    }

    return RESULT_OK;
  }

  Result VisionComponent::UpdateOverheadEdges(const VisionProcessingResult& procResult)
  {
    for(auto & edgeFrame : procResult.overheadEdges)
    {
      _robot->GetMapComponent().ProcessVisionOverheadEdges(edgeFrame);
    }

    return RESULT_OK;
  }

  Result VisionComponent::UpdateComputedCalibration(const VisionProcessingResult& procResult)
  {
    DEV_ASSERT((procResult.cameraCalibration.empty() || procResult.cameraCalibration.size() == 1),
               "VisionComponent.UpdateComputedCalibration.UnexpectedNumCalibrations");

    for(auto & calib : procResult.cameraCalibration)
    {
      CameraCalibration msg;
      msg.center_x = calib.GetCenter_x();
      msg.center_y = calib.GetCenter_y();
      msg.focalLength_x = calib.GetFocalLength_x();
      msg.focalLength_y = calib.GetFocalLength_y();
      msg.nrows = calib.GetNrows();
      msg.ncols = calib.GetNcols();
      msg.skew = calib.GetSkew();

      DEV_ASSERT_MSG(msg.distCoeffs.size() == calib.GetDistortionCoeffs().size(),
                     "VisionComponent.UpdateComputedCalibration.WrongNumDistCoeffs",
                     "Message expects %zu, got %zu", msg.distCoeffs.size(), calib.GetDistortionCoeffs().size());

      std::copy(calib.GetDistortionCoeffs().begin(), calib.GetDistortionCoeffs().end(), msg.distCoeffs.begin());

      _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
    }

    return RESULT_OK;
  }

  Result VisionComponent::UpdateCameraParams(const VisionProcessingResult& procResult)
  {
    if(!_robot->IsPhysical() || procResult.imageQuality == Vision::ImageQuality::Unchecked)
    {
      // Nothing to do
      return RESULT_OK;
    }

    if(CaptureFormatState::None != _captureFormatState)
    {
      // Disable auto exposure and white balance while switching formats
      // Even though the vision system is paused, it is possible that we
      // are just finishing processing an image and don't want to send
      // auto exposure and white balance messages to the camera
      return RESULT_OK;
    }

    const Vision::CameraParams& params = procResult.cameraParams;

    // Note that we set all parameters together. If WB or AE isn't enabled accoding to current VisionModes,
    // their corresponding values should not actually be different in the params.
    const Result result = _visionSystem->SetNextCameraParams(params);

    if(RESULT_OK == result)
    {
      LOG_DEBUG("VisionComponent.UpdateImageQuality",
                "ExpTime:%dms ExpGain:%f GainR:%f GainG:%f GainB:%f",
                params.exposureTime_ms, params.gain,
                params.whiteBalanceGainR, params.whiteBalanceGainG, params.whiteBalanceGainB);
      
      auto cameraService = CameraService::getInstance();

      const bool isWhiteBalanceEnabled = procResult.modesProcessed.Contains(VisionMode::WhiteBalance);
      if(isWhiteBalanceEnabled)
      {
        cameraService->CameraSetWhiteBalanceParameters(params.whiteBalanceGainR,
                                                       params.whiteBalanceGainG,
                                                       params.whiteBalanceGainB);
      }

      const bool isAutoExposureEnabled = procResult.modesProcessed.Contains(VisionMode::AutoExp);
      if(isAutoExposureEnabled)
      {
        cameraService->CameraSetParameters(procResult.cameraParams.exposureTime_ms,
                                           procResult.cameraParams.gain);
      }

      _vizManager->SendCameraParams(params);

      {
        // Still needed?
        // TODO: Add WB params to message?
        using namespace ExternalInterface;
        const u16 exposure_ms_u16 = Util::numeric_cast<u16>(params.exposureTime_ms);
        _robot->Broadcast(MessageEngineToGame(CurrentCameraParams(params.gain,
                                                                  exposure_ms_u16,
                                                                  isAutoExposureEnabled)));
      }

    }

    if(procResult.imageQuality != _lastImageQuality || _currentQualityBeginTime_ms==0)
    {
      // Just switched image qualities
      _currentQualityBeginTime_ms = procResult.timestamp;
      _waitForNextAlert_ms = kImageQualityAlertDuration_ms; // for first alert, use "duration" time
      _lastBroadcastImageQuality = Vision::ImageQuality::Unchecked; // i.e. reset so we definitely broadcast again
    }
    else if(_lastBroadcastImageQuality != Vision::ImageQuality::Good) // Don't keep broadcasting once in Good state
    {
      const RobotTimeStamp_t timeWithThisQuality_ms = procResult.timestamp - _currentQualityBeginTime_ms;

      if(timeWithThisQuality_ms > _waitForNextAlert_ms)
      {
        // If we get here, we've been in a new image quality for long enough to trust it and broadcast.
        EngineErrorCode errorCode = EngineErrorCode::Count;

        switch(_lastImageQuality)
        {
          case Vision::ImageQuality::Unchecked:
            // can't get here due to IF above (but need case to prevent compiler error without default)
            assert(false);
            break;

          case Vision::ImageQuality::Good:
            errorCode = EngineErrorCode::ImageQualityGood;
            break;

          case Vision::ImageQuality::TooDark:
            errorCode = EngineErrorCode::ImageQualityTooDark;
            break;

          case Vision::ImageQuality::TooBright:
            errorCode = EngineErrorCode::ImageQualityTooBright;
            break;
        }

        LOG_INFO("robot.vision.image_quality", "%s", EnumToString(errorCode));

        LOG_DEBUG("VisionComponent.UpdateImageQuality.BroadcastingImageQualityChange",
                  "Seeing %s for more than %u > %ums, broadcasting %s",
                  EnumToString(procResult.imageQuality), (TimeStamp_t)timeWithThisQuality_ms,
                  (TimeStamp_t)_waitForNextAlert_ms, EnumToString(errorCode));

        using namespace ExternalInterface;
        _robot->Broadcast(MessageEngineToGame(EngineErrorCodeMessage(errorCode)));
        _lastBroadcastImageQuality = _lastImageQuality;

        // Reset start time just to avoid spamming if this is a "bad" quality, which
        // we will keep sending until it's better. (Good quality is only broadcast
        // one time, once we transition out of Bad quality)
        _currentQualityBeginTime_ms = procResult.timestamp;
        _waitForNextAlert_ms = kImageQualityAlertSpacing_ms; // after first alert use "spacing" time
      }
    }

    _lastImageQuality = procResult.imageQuality;

    return RESULT_OK;
  }

  Result VisionComponent::UpdatePhotoManager(const VisionProcessingResult& procResult)
  {
    if(_robot->GetPhotographyManager().IsWaitingForPhoto())
    {
      _robot->GetPhotographyManager().SetLastPhotoTimeStamp(procResult.timestamp);
    }
    return RESULT_OK;
  }

  Result VisionComponent::UpdateDetectedIllumination(const VisionProcessingResult& procResult)
  {
    ExternalInterface::RobotObservedIllumination msg( procResult.illumination );
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
    _lastIlluminationState = procResult.illumination.state;
    return RESULT_OK;
  }

  Result VisionComponent::UpdateMirrorMode(const VisionProcessingResult& procResult)
  {
    // Handle switching the debug screen on/off when mirror mode changes
    static bool wasMirrorModeEnabled = false;
    const bool isMirrorModeEnabled = procResult.modesProcessed.Contains(VisionMode::MirrorMode);
    if(wasMirrorModeEnabled != isMirrorModeEnabled)
    {
      LOG_INFO("VisionComponent.UpdateMirrorMode.TogglingMirrorMode",
               "Turning MirrorMode %s", isMirrorModeEnabled ? "ON" : "OFF");
      _robot->SendRobotMessage<RobotInterface::EnableMirrorModeScreen>(isMirrorModeEnabled);
      wasMirrorModeEnabled = isMirrorModeEnabled;
    }

    // If Mirror is no longer scheduled then don't bother trying to draw anything
    // Normally you would only need to check if MirrorMode was in the processed vision modes, however,
    // due to a timing issue between anim and engine when entering pairing from the mirror mode screen
    // we need to check the schedule instead of processed vision modes.
    // Ex: MirrorMode enabled, double press backpack button, switchboard sends down enter pairing message, engine gets it
    // first and unsubscribes from MirrorMode, anim then gets the message and draws the pairing screen, then on the next
    // tick VisionSystem comes back with a processed image from when MirrorMode was still enabled and we send that image
    // to be drawn on the face overriding the pairing screen.
    // The bug technically lies in anim process for still drawing the face image even though it is on the pairing screens
    // but it feels safer to "fix" it here instead of messing with animStreamer, faceInfoScreenManager, and connectionFlow.
    const AllVisionModesSchedule& schedule = _robot->GetVisionScheduleMediator().GetSchedule();
    if(!schedule.GetScheduleForMode(VisionMode::MirrorMode).WillEverRun())
    {
      return RESULT_OK;
    }

    // Send as face display animation
    auto & animComponent = _robot->GetAnimationComponent();
    if(isMirrorModeEnabled && animComponent.GetAnimState_NumProcAnimFaceKeyframes() < 5) // Don't get too far ahead
    {
      // NOTE: This creates a non-const image "header" around the same data as is in procResult.mirrorModeImg.
      // Due to a bug / design flaw in OpenCV, this actually allows us to draw on that image, even though
      // it's technically const. Since this is a debug mode, we're using this to avoid a copy in the case
      // that we have eye contact or a display string, since performance is the higher priority here.
      Vision::ImageRGB565 mirrorModeImg = procResult.mirrorModeImg;

      if(!_mirrorModeDisplayString.empty())
      {
        mirrorModeImg.DrawText({1,14}, _mirrorModeDisplayString, _mirrorModeStringColor, 0.6f, true);
      }

      if(kDisplayEyeContactInMirrorMode)
      {
        const u32 maxTimeSinceSeenFaceToLook_ms = ConditionEyeContact::GetMaxTimeSinceTrackedFaceUpdated_ms();
        const bool making_eye_contact = _robot->GetFaceWorld().IsMakingEyeContact(maxTimeSinceSeenFaceToLook_ms);
        if(making_eye_contact)
        {
          // Put eye contact indicator right in the middle
          const f32 x = .5f * (f32)procResult.mirrorModeImg.GetNumCols();
          const f32 y = .5f * (f32)procResult.mirrorModeImg.GetNumRows();
          const f32 width = .2f * (f32)procResult.mirrorModeImg.GetNumCols();
          const f32 height = .2f * (f32)procResult.mirrorModeImg.GetNumRows();

          mirrorModeImg.DrawFilledRect(Rectangle<f32>(x, y, width, height), NamedColors::YELLOW);
        }
      }

      // Just display the mirror mode image as is, from the processing result
      const bool kInterruptRunning = false;
      animComponent.DisplayFaceImage(mirrorModeImg,
				     AnimationComponent::DEFAULT_STREAMING_FACE_DURATION_MS,
				     kInterruptRunning);
    }
    return RESULT_OK;
  }

  void VisionComponent::AddLiftOccluder(const RobotTimeStamp_t t_request)
  {
    // TODO: More precise check for position of lift in FOV given head angle
    HistRobotState histState;
    RobotTimeStamp_t t;
    Result result = _robot->GetStateHistory()->GetRawStateAt(t_request, t, histState, false);

    if(RESULT_FAIL_ORIGIN_MISMATCH == result)
    {
      // Not a warning, this can legitimately happen
      LOG_INFO("VisionComponent.VisionComponent.AddLiftOccluder.StateHistoryOriginMismatch",
               "Could not get pose at t=%u due to origin change. Skipping.", (TimeStamp_t)t_request);
      return;
    }
    else if(RESULT_OK != result)
    {
      LOG_WARNING("VisionComponent.WasLiftInFOV.StateHistoryFailure",
                  "Could not get raw pose at t=%u", (TimeStamp_t)t_request);
      return;
    }

    const Transform3d& liftPoseWrtCamera = _robot->GetLiftTransformWrtCamera(histState.GetLiftAngle_rad(),
                                                                             histState.GetHeadAngle_rad());

    std::vector<Point3f> liftCrossBar;
    liftPoseWrtCamera.ApplyTo(_liftCrossBarSource, liftCrossBar);

    std::vector<Point2f> liftCrossBarProj;
    _camera->Project3dPoints(liftCrossBar, liftCrossBarProj);

    _camera->AddOccluder(liftCrossBarProj, liftPoseWrtCamera.GetTranslation().Length());
  }

  Vision::ImageRGB GetUndistorted(const Vision::ImageRGB& img, const Vision::Camera& camera)
  {
    Vision::ImageRGB img_undistort;
    img.Undistort(*camera.GetCalibration(), img_undistort);
    return img_undistort;
  }


  Vision::Image GetUndistorted(const Vision::Image& img, const Vision::Camera& camera)
  {
    Vision::Image img_undistort;
    img.Undistort(*camera.GetCalibration(), img_undistort);
    return img_undistort;
  }

  Result VisionComponent::SendCompressedImage(const Vision::CompressedImage& img, const std::string& identifier)
  {
    if(!_robot->HasExternalInterface())
    {
      LOG_ERROR("VisionComponent.CompressAndSendImage.NoExternalInterface", "");
      return RESULT_FAIL;
    }

    ImageChunk m;
    const bool vizConnected = _robot->GetContext()->GetVizManager()->IsConnected();
    if(!vizConnected && !_sendProtoImageChunks)
    {
      return RESULT_OK;
    }

    const std::vector<u8>& compressedBuffer = img.GetCompressedBuffer();

    std::string data;
    external_interface::ImageChunk* imageChunk = nullptr;
    external_interface::GatewayWrapper wrapper;

    const u32 kMaxChunkSize = static_cast<u32>(ImageConstants::IMAGE_CHUNK_SIZE);
    u32 bytesRemainingToSend = static_cast<u32>(compressedBuffer.size());

    static u32 imgID = 0;
    u8 chunkId = 0;
    u8 imageChunkCount = ceilf((f32)bytesRemainingToSend / kMaxChunkSize);
    u8 displayIndex;
    ++imgID;

    auto displayIndexIter = _vizDisplayIndexMap.find(identifier);
    if(displayIndexIter == _vizDisplayIndexMap.end())
    {
      // New identifier
      displayIndex = Util::numeric_cast<s32>(_vizDisplayIndexMap.size());
      _vizDisplayIndexMap.emplace(identifier, displayIndex); // NOTE: this will increase size() for next time
    }
    else
    {
      displayIndex = displayIndexIter->second;
    }

    // Construct a clad ImageChunk
    if(vizConnected)
    {
      m.height = img.GetNumRows();
      m.width  = img.GetNumCols();

      // Use the identifier value as the display index
      m.displayIndex = displayIndex;

      m.imageId = imgID;

      m.frameTimeStamp = img.GetTimestamp();
      m.imageChunkCount = imageChunkCount;
      if(img.GetNumChannels() == 1)
      {
        m.imageEncoding = Vision::ImageEncoding::JPEGGray;
      }
      else
      {
        m.imageEncoding = Vision::ImageEncoding::JPEGColor;
      }
    }

    // Construct a proto ImageChunk
    if(_sendProtoImageChunks)
    {
      imageChunk = new external_interface::ImageChunk();
      imageChunk->set_height((u32)img.GetNumRows());
      imageChunk->set_width((u32)img.GetNumCols());

      imageChunk->set_display_index((u32)displayIndex);

      imageChunk->set_image_id(imgID);
      imageChunk->set_frame_time_stamp(img.GetTimestamp());
      imageChunk->set_image_chunk_count((u32)imageChunkCount);
      if(img.GetNumChannels() == 1)
      {
        imageChunk->set_image_encoding(external_interface::ImageChunk_ImageEncoding_JPEG_GRAY);
      }
      else
      {
        imageChunk->set_image_encoding(external_interface::ImageChunk_ImageEncoding_JPEG_COLOR);
      }
    }

    while (bytesRemainingToSend > 0)
    {
      u32 chunkSize = std::min(bytesRemainingToSend, kMaxChunkSize);

      auto startIt = compressedBuffer.begin() + (compressedBuffer.size() - bytesRemainingToSend);
      auto endIt = startIt + chunkSize;

      if(_sendProtoImageChunks)
      {
        imageChunk->set_chunk_id((u32)chunkId);
        data.assign(startIt, endIt);
        imageChunk->set_data(std::move(data));

        wrapper.set_allocated_image_chunk(imageChunk);
        _robot->GetGatewayInterface()->Broadcast(wrapper);
        imageChunk = wrapper.release_image_chunk();
      }

      if(vizConnected)
      {
        m.chunkId = chunkId;
        m.data = std::vector<u8>(startIt, endIt);

        _robot->Broadcast(ExternalInterface::MessageEngineToGame(ImageChunk(m)));
        // Forward the image chunks to Viz as well (Note that this does nothing if
        // sending images is disabled in VizManager)
        _robot->GetContext()->GetVizManager()->SendImageChunk(m);
      }

      bytesRemainingToSend -= chunkSize;
      ++chunkId;
    }

    Util::SafeDelete(imageChunk);

    return RESULT_OK;

  }

  Result VisionComponent::ClearCalibrationImages()
  {
    if(nullptr == _visionSystem || !_visionSystem->IsInitialized())
    {
      LOG_ERROR("VisionComponent.ClearCalibrationImages.VisionSystemNotReady", "");
      return RESULT_FAIL;
    }
    else
    {
      Lock();
      Result res = _visionSystem->ClearCalibrationImages();
      Unlock();

      return res;
    }
  }

  size_t VisionComponent::GetNumStoredCameraCalibrationImages() const
  {
    return _visionSystem->GetNumStoredCalibrationImages();
  }

  Result VisionComponent::GetCalibrationPoseToRobot(size_t whichPose, Pose3d& p) const
  {
    Result lastResult = RESULT_FAIL;

    auto & calibPoses = _visionSystem->GetCalibrationPoses();
    if(whichPose >= calibPoses.size()) {
      LOG_WARNING("VisionComponent.WriteCalibrationPoseToRobot.InvalidPoseIndex",
                  "Requested %zu, only %zu available", whichPose, calibPoses.size());
    } else {
      auto & calibImages = _visionSystem->GetCalibrationImages();
      DEV_ASSERT_MSG(calibImages.size() >= calibPoses.size(),
                     "VisionComponent.WriteCalibrationPoseToRobot.SizeMismatch",
                     "Expecting at least %zu calibration images, got %zu",
                     calibPoses.size(), calibImages.size());

      if(!calibImages[whichPose].dotsFound) {
        LOG_INFO("VisionComponent.WriteCalibrationPoseToRobot.PoseNotComputed",
                         "Dots not found in image %zu, no pose available",
                         whichPose);
      } else {
        p = calibPoses[whichPose];
        lastResult = RESULT_OK;
      }
    }

    return lastResult;
  }

  std::list<std::vector<u8> > VisionComponent::GetCalibrationImageJpegData(u8* dotsFoundMask) const
  {
    const auto& calibImages = _visionSystem->GetCalibrationImages();

    // Jpeg compress images
    u32 imgIdx = 0;
    u8 usedMask = 0;
    std::list<std::vector<u8> > rawJpegData;
    for (auto const& calibImage : calibImages)
    {
      const Vision::Image& img = calibImage.img;

      if(calibImage.dotsFound) {
        usedMask |= ((u8)1 << imgIdx);
      }

      // Compress to jpeg
      std::vector<u8> imgVec;
      cv::imencode(".jpg", img.get_CvMat_(), imgVec, std::vector<int>({CV_IMWRITE_JPEG_QUALITY, 50}));

      Vision::Image imgUndistorted(img.GetNumRows(),img.GetNumCols());
      DEV_ASSERT(_camera->IsCalibrated(), "VisionComponent.GetCalibrationImageJpegData.NoCalibration");
      img.Undistort(*_camera->GetCalibration(), imgUndistorted);

      std::vector<u8> imgVecUndistort;
      cv::imencode(".jpg", imgUndistorted.get_CvMat_(), imgVecUndistort, std::vector<int>({CV_IMWRITE_JPEG_QUALITY, 50}));

      /*
      std::string imgFilename = "savedImg_" + std::to_string(imgIdx) + ".jpg";
      FILE* fp = fopen(imgFilename.c_str(), "w");
      fwrite(imgVec.data(), imgVec.size(), 1, fp);
      fclose(fp);
      */

      rawJpegData.emplace_back(std::move(imgVec));
      rawJpegData.emplace_back(std::move(imgVecUndistort));

      ++imgIdx;
    }

    if (dotsFoundMask) {
      *dotsFoundMask = usedMask;
    }

    return rawJpegData;
  }

  Result VisionComponent::ComputeCameraPoseVsIdeal(const Quad2f& obsQuad, Pose3d& pose) const
  {
    // Define real size of target, w.r.t. its center as origin
    static const f32 kDotSpacingX_mm = 40.f;
    static const f32 kDotSpacingY_mm = 27.f;
    static const Quad3f targetQuad(Point3f(-0.5f*kDotSpacingX_mm, -0.5f*kDotSpacingY_mm, 0.f),
                                   Point3f(-0.5f*kDotSpacingX_mm,  0.5f*kDotSpacingY_mm, 0.f),
                                   Point3f( 0.5f*kDotSpacingX_mm, -0.5f*kDotSpacingY_mm, 0.f),
                                   Point3f( 0.5f*kDotSpacingX_mm,  0.5f*kDotSpacingY_mm, 0.f));

    // Distance between target center and intersection of camera's optical axis with the
    // target, due to 4-degree look-down.
    static const f32 kTargetCenterVertDist_mm = 4.2f;

    // Compute pose of target's center w.r.t. the camera
    Pose3d targetWrtCamera;
    Result result = _camera->ComputeObjectPose(obsQuad, targetQuad, targetWrtCamera);
    if(RESULT_OK != result) {
      LOG_WARNING("VisionComponent.FindFactorTestDotCentroids.ComputePoseFailed", "");
      return result;
    }

    // Define the pose of the target (center) w.r.t. the ideal camera.
    // NOTE: 4.2mm is
    //       The center of the target lies along the ideal (unrotated) camera's optical axis
    //       and the target dots are defined in that coordinate frame (x,y) at upper left.
    const f32 kTargetDist_mm = kTargetCenterVertDist_mm / std::tan(DEG_TO_RAD(4));
    const Pose3d targetWrtIdealCamera(0, Z_AXIS_3D(), {0.f, 0.f, kTargetDist_mm});

    // Compute the pose of the camera w.r.t. the ideal camera
    pose = targetWrtCamera.GetInverse() * targetWrtIdealCamera;

    return RESULT_OK;
  }

  Result VisionComponent::FindFactoryTestDotCentroids(const Vision::Image& image,
                                                      ExternalInterface::RobotCompletedFactoryDotTest& msg)
  {
    // Expected locations for each dot with a generous search area around each
    static const s32 kSearchSize_pix = 80;
    static const std::array<Point2i,4> kExpectedDotCenters_pix{{
      /* Upper Left  */ Point2i(40,   40),
      /* Lower Left  */ Point2i(40,  200),
      /* Upper Right */ Point2i(280,  40),
      /* Lower Right */ Point2i(280, 200)
    }};

    static const s32 kDotDiameter_pix = 25;
    static const f32 kDotArea_pix = (f32)(kDotDiameter_pix*kDotDiameter_pix) * 0.25f * M_PI_F;
    static const f32 kMinAreaFrac = 0.75f; // relative to circular dot area
    static const f32 kMaxAreaFrac = 1.25f; // relative to dot bounding box

    // Enable drawing the image with detected centroids and search areas
    // NOTE: Don't check this in set to true!
    const bool kDrawDebugDisplay = false;

    // Binarize and compute centroid for the ROI around each expected dot location
    for(s32 iDot=0; iDot < 4; ++iDot)
    {
      auto & expectedCenter = kExpectedDotCenters_pix[iDot];

      Rectangle<s32> roiRect(expectedCenter.x()-kSearchSize_pix/2, expectedCenter.y()-kSearchSize_pix/2,
                             kSearchSize_pix, kSearchSize_pix);

      // Use Otsu inverted thresholding to get the dark stuff as "on" with an automatically-computed threshold
      Vision::Image roi;
      image.GetROI(roiRect).CopyTo(roi);
      cv::threshold(roi.get_CvMat_(), roi.get_CvMat_(), 128, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

      // Find the connected component closest to center (that is of reasonable area)
      // NOTE: The squares of the checkerboard pattern will hopefully be too small
      //       if they come out as individual components and too large if they
      //       get connected together due to blurring / compression.
      Array2d<s32> labels;
      std::vector<Vision::Image::ConnectedComponentStats> stats;
      const s32 N = roi.GetConnectedComponents(labels, stats);
      const Point2f center(0.5f*(f32)roiRect.GetWidth(), 0.5f*(f32)roiRect.GetHeight());
      f32 bestDistFromCenter = std::numeric_limits<f32>::max();
      size_t largestArea = 0;
      s32 bestComp = 0;
      for(s32 iComp=1; iComp<N; ++iComp) // Note: skipping "background" component 0
      {
        // Use the bounding box for the max area check so that checkboard squares
        // that get linked together look even bigger (and therefore too large)
        const Rectangle<s32>& bbox = stats[iComp].boundingBox;
        if(bbox.Area() < kMaxAreaFrac * kDotDiameter_pix * kDotDiameter_pix)
        {
          const size_t currentArea = stats[iComp].area;
          if(currentArea > kDotArea_pix*kMinAreaFrac && currentArea > largestArea)
          {
            const f32 d = ComputeDistanceBetween(stats[iComp].centroid, center);
            if(d < bestDistFromCenter) {
              bestDistFromCenter = d;
              bestComp = iComp;
              largestArea = currentArea;
            }
          }
        }
      }

      if(bestComp == 0) {
        LOG_WARNING("VisionComponent.FindFactoryTestDotCentroids.NotComponentLargeEnough",
                    "DotArea=%.1f, MinFrac=%.2f", kDotArea_pix, kMinAreaFrac);
        return RESULT_FAIL;
      }

      // Put centroid in original image coordinates (not ROI coordinates)
      msg.dotCenX_pix[iDot] = stats[bestComp].centroid.x() + roiRect.GetX();
      msg.dotCenY_pix[iDot] = stats[bestComp].centroid.y() + roiRect.GetY();
    }

    if(_camera->IsCalibrated())
    {
      // Go ahead and compute the camera pose using the dots if the camera is already calibrated

      const Quad2f obsQuad(Point2f(msg.dotCenX_pix[0], msg.dotCenY_pix[0]),
                           Point2f(msg.dotCenX_pix[1], msg.dotCenY_pix[1]),
                           Point2f(msg.dotCenX_pix[2], msg.dotCenY_pix[2]),
                           Point2f(msg.dotCenX_pix[3], msg.dotCenY_pix[3]));

      Pose3d pose;
      Result poseResult = ComputeCameraPoseVsIdeal(obsQuad, pose);
      if(RESULT_OK != poseResult) {
        LOG_WARNING("VisionComponent.FindFactoryTestDotCentroids.ComputePoseFailed", "");
      } else {
        msg.camPoseX_mm = pose.GetTranslation().x();
        msg.camPoseY_mm = pose.GetTranslation().y();
        msg.camPoseZ_mm = pose.GetTranslation().z();

        msg.camPoseRoll_rad  = pose.GetRotation().GetAngleAroundZaxis().ToFloat();
        msg.camPosePitch_rad = pose.GetRotation().GetAngleAroundXaxis().ToFloat();
        msg.camPoseYaw_rad   = pose.GetRotation().GetAngleAroundYaxis().ToFloat();

        msg.didComputePose = true;
      }
    }

    msg.headAngle = _robot->GetComponent<FullRobotPose>().GetHeadAngle();
    msg.success = true;

    if(kDrawDebugDisplay)
    {
      Vision::ImageRGB debugDisp(image);
      for(s32 iDot=0; iDot<4; ++iDot) {
        debugDisp.DrawCircle(Point2f(msg.dotCenX_pix[iDot], msg.dotCenY_pix[iDot]), NamedColors::RED, 3);

        Rectangle<f32> roiRect(kExpectedDotCenters_pix[iDot].x()-kSearchSize_pix/2,
                               kExpectedDotCenters_pix[iDot].y()-kSearchSize_pix/2,
                               kSearchSize_pix, kSearchSize_pix);
        debugDisp.DrawRect(roiRect, NamedColors::GREEN, 2);
      }
      debugDisp.Display("FactoryTestFindDots", 0);
    }

    return RESULT_OK;
  } // FindFactoryTestDotCentroids()

  Result VisionComponent::SaveFaceAlbumToFile(const std::string& path)
  {
    Lock();
    Result result = _visionSystem->SaveFaceAlbum(path);
    Unlock();

    if(RESULT_OK != result) {
      LOG_WARNING("VisionComponent.SaveFaceAlbum.SaveToFileFailed",
                  "AlbumFile: %s", path.c_str());
    }
    return result;
  }

  inline static std::string GetFullFaceAlbumPath(const CozmoContext* context, const std::string& pathIn)
  {
    const std::string fullPath = context->GetDataPlatform()->pathToResource(Util::Data::Scope::Persistent,
                                                                            Util::FileUtils::FullFilePath({"faceAlbums", pathIn}));
    return fullPath;
  }

  Result VisionComponent::SaveFaceAlbum()
  {
    const std::string fullFaceAlbumPath = GetFullFaceAlbumPath(_context, _faceAlbumName);
    return SaveFaceAlbumToFile(fullFaceAlbumPath);
  }

  Result VisionComponent::LoadFaceAlbumFromFile(const std::string& path)
  {
    std::list<Vision::LoadedKnownFace> loadedFaces;
    Result loadResult = LoadFaceAlbumFromFile(path, loadedFaces);
    if(RESULT_OK == loadResult)
    {
      BroadcastLoadedNamesAndIDs(loadedFaces);
    }

    return loadResult;
  }

  Result VisionComponent::LoadFaceAlbum()
  {
    const std::string fullFaceAlbumPath = GetFullFaceAlbumPath(_context, _faceAlbumName);
    return LoadFaceAlbumFromFile(fullFaceAlbumPath);
  }

  Result VisionComponent::LoadFaceAlbumFromFile(const std::string& path, std::list<Vision::LoadedKnownFace>& loadedFaces)
  {
    Lock();
    Result result = _visionSystem->LoadFaceAlbum(path, loadedFaces);
    Unlock();

    if(RESULT_OK != result) {
      LOG_WARNING("VisionComponent.LoadFaceAlbum.LoadFromFileFailed",
                  "AlbumFile: %s", path.c_str());
    }

    return result;
  }

  bool VisionComponent::CanAddNamedFace()
  {
    Lock();
    const bool canAdd = _visionSystem->CanAddNamedFace();
    Unlock();
    return canAdd;
  }

  void VisionComponent::AssignNameToFace(Vision::FaceID_t faceID, const std::string& name, Vision::FaceID_t mergeWithID)
  {
    // These are "failsafe" checks to avoid getting duplicate names in the face records.
    // This is a pretty gross, heavy-handed way to achieve this, but is meant to try
    // to catch bugs in the complex EnrollFace behavior in the wild.
    const auto& idsWithName = GetFaceIDsWithName(name);
    if(mergeWithID == Vision::UnknownFaceID)
    {
      if(!idsWithName.empty())
      {
        // This should not happen (caller error?), but for now wallpaper over it and avoid duplicate names polluting
        // the system
        mergeWithID = *idsWithName.begin(); // Just use first. Existence of multiple will be checked in next 'if'
        std::string idStr;
        for(auto idWithName : idsWithName)
        {
          idStr += std::to_string(idWithName);
          idStr += " ";
        }
        LOG_ERROR("VisionComponent.AssignNameToFace.DuplicateNameWithoutMerge",
                  "Name '%s' already in use (IDs:%s) with no mergeID specified. Forcing merge with ID:%d",
                  Util::HidePersonallyIdentifiableInfo(name.c_str()), idStr.c_str(), mergeWithID);
      }
    }
    if(mergeWithID != Vision::UnknownFaceID) // deliberate recheck of mergeWithID, not "else"
    {
      if(idsWithName.size() != 1)
      {
        // THIS IS VERY BAD! WE HAVE SOMEHOW ENROLLED TWO RECORDS WITH THE SAME NAME!
        std::string idStr;
        for(auto idWithName : idsWithName)
        {
          idStr += std::to_string(idWithName);
          idStr += " ";
        }
        LOG_ERROR("VisionComponent.AssignNameToFace.MultipleIDsWithSameName",
                  "Found %zu IDs with name '%s': %s",
                  idsWithName.size(), Util::HidePersonallyIdentifiableInfo(name.c_str()), idStr.c_str());
      }
    }

    // Pair this name and ID in the vision system
    Lock();
    _visionSystem->AssignNameToFace(faceID, name, mergeWithID);
    int numTotal = static_cast<int>(_visionSystem->GetEnrolledNames().size());
    Unlock();
    {
      DASMSG(vision_enrolled_names_new, "vision.enrolled_names.new", "A face was assigned a name");
      DASMSG_SET(i1, faceID, "The face ID (int)");
      DASMSG_SET(i2, numTotal, "total number of enrolled faces");
      DASMSG_SEND();
    }
  }

  void VisionComponent::SetFaceEnrollmentMode(Vision::FaceID_t forFaceID, s32 numEnrollments, bool forceNewID)
  {
    _visionSystem->SetFaceEnrollmentMode(forFaceID, numEnrollments, forceNewID);
  }

#if ANKI_DEV_CHEATS
  void VisionComponent::SaveAllRecognitionImages(const std::string& imagePathPrefix)
  {
    _visionSystem->SaveAllRecognitionImages(imagePathPrefix);
  }

  void VisionComponent::DeleteAllRecognitionImages()
  {
    _visionSystem->DeleteAllRecognitionImages();
  }
#endif

  Result VisionComponent::EraseFace(Vision::FaceID_t faceID)
  {
    Lock();
    Result result = _visionSystem->EraseFace(faceID);
    int numRemaining = static_cast<int>(_visionSystem->GetEnrolledNames().size());
    Unlock();
    if(RESULT_OK == result) {
      // Update face album on disk
      SaveFaceAlbum();

      // Send back confirmation
      ExternalInterface::RobotErasedEnrolledFace msg;
      msg.faceID  = faceID;
      msg.name    = "";
      _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
      {
        DASMSG(vision_enrolled_names_erase, "vision.enrolled_names.erase", "An enrolled face/name was erased");
        DASMSG_SET(i1, faceID, "The face ID (int)");
        DASMSG_SET(i2, numRemaining, "total number of remaining enrolled faces");
        DASMSG_SEND();
      }
      return RESULT_OK;
    } else {
      return RESULT_FAIL;
    }
  }

  void VisionComponent::EraseAllFaces()
  {
    _robot->GetFaceWorld().ClearAllFaces();
    Lock();
    _visionSystem->EraseAllFaces();
    Unlock();
    SaveFaceAlbum();
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotErasedAllEnrolledFaces()));
  }

  void VisionComponent::RequestEnrolledNames()
  {
    ExternalInterface::EnrolledNamesResponse response;
    Lock();
    response.faces = _visionSystem->GetEnrolledNames();
    Unlock();

    _robot->Broadcast( ExternalInterface::MessageEngineToGame( std::move(response) ) );
  }

  Result VisionComponent::RenameFace(Vision::FaceID_t faceID, const std::string& oldName, const std::string& newName)
  {
    if(oldName == newName)
    {
      LOG_INFO("VisionComponent.RenameFace.SameOldAndNewNames",
               "Ignoring request to rename face %d from %s to %s",
               faceID,
               Util::HidePersonallyIdentifiableInfo(oldName.c_str()),
               Util::HidePersonallyIdentifiableInfo(newName.c_str()));
      {
        DASMSG(vision_enrolled_names_no_change, "vision.enrolled_names.no_change",
               "An enrolled face/name was left unchanged");
        DASMSG_SET(i1, faceID, "The face ID (int)");
        DASMSG_SEND();
      }
      return RESULT_OK;
    }

    Vision::RobotRenamedEnrolledFace renamedFace;
    Lock();
    Result result = _visionSystem->RenameFace(faceID, oldName, newName, renamedFace);
    int numTotal = static_cast<int>(_visionSystem->GetEnrolledNames().size());
    Unlock();

    if(RESULT_OK == result)
    {
      SaveFaceAlbum();
      _robot->Broadcast(ExternalInterface::MessageEngineToGame( std::move(renamedFace) ));
      {
        DASMSG(vision_enrolled_names_modify, "vision.enrolled_names.modify", "An enrolled face/name was modified");
        DASMSG_SET(i1, faceID, "The face ID (int)");
        DASMSG_SET(i2, numTotal, "total number of enrolled faces");
        DASMSG_SEND();
      }
    }

    return result;
  }

  bool VisionComponent::IsNameTaken(const std::string& name)
  {
    return !GetFaceIDsWithName(name).empty();
  }

  std::set<Vision::FaceID_t> VisionComponent::GetFaceIDsWithName(const std::string& name)
  {
    Lock();
    auto namePairList = _visionSystem->GetEnrolledNames();
    Unlock();

    std::set<Vision::FaceID_t> idsWithName;
    for( const auto& nameEntry : namePairList ) {
      if( Anki::Util::StringCaseInsensitiveEquals(nameEntry.name, name) ) {
        idsWithName.insert(nameEntry.faceID);
      }
    }
    return idsWithName;
  }

  void VisionComponent::BroadcastLoadedNamesAndIDs(const std::list<Vision::LoadedKnownFace>& loadedFaces) const
  {
    // Notify about the newly-available names and IDs, and create wave files
    // for the names if they don't already exist, so we've already got them
    // when we want to say them at some point later.
    using namespace ExternalInterface;
    _robot->Broadcast(MessageEngineToGame(RobotErasedAllEnrolledFaces()));
    for(auto & loadedFace : loadedFaces)
    {

      LOG_INFO("VisionComponent.BroadcastLoadedNamesAndIDs", "broadcasting loaded face id: %d",
               loadedFace.faceID);

      _robot->Broadcast(MessageEngineToGame( Vision::LoadedKnownFace(loadedFace) ));
    }

    _robot->GetFaceWorld().InitLoadedKnownFaces(loadedFaces);
  }

  void VisionComponent::FakeImageProcessed(RobotTimeStamp_t t)
  {
    _lastProcessedImageTimeStamp_ms = t;
  }

  Vision::CameraParams VisionComponent::GetCurrentCameraParams() const
  {
    return _visionSystem->GetCurrentCameraParams();
  }

  void VisionComponent::EnableAutoExposure(bool enable)
  {
    EnableMode(VisionMode::AutoExp, enable);
  }

  void VisionComponent::EnableWhiteBalance(bool enable)
  {
    EnableMode(VisionMode::WhiteBalance, enable);
  }

  void VisionComponent::SetAndDisableCameraControl(const Vision::CameraParams& params)
  {
    const Result result = _visionSystem->SetNextCameraParams(params);
    if(RESULT_OK != result)
    {
      LOG_WARNING("VisionComponent.SetAndDisableCameraControl.SetNextCameraParamsFailed", "");
      return;
    }

    // Disable AE and WB computation on the vision thread
    EnableWhiteBalance(false);
    EnableAutoExposure(false);

    // Directly set the specified camera values, since they won't be coming from the
    // VisionSystem in a VisionProcessingResult anymore. Also manually update Viz
    auto cameraService = CameraService::getInstance();
    if(cameraService)
    {
      cameraService->CameraSetParameters(params.exposureTime_ms,
                                         params.gain);
      cameraService->CameraSetWhiteBalanceParameters(params.whiteBalanceGainR,
                                                     params.whiteBalanceGainG,
                                                     params.whiteBalanceGainB);

      _vizManager->SendCameraParams(params);
    }
  }

  s32 VisionComponent::GetMinCameraExposureTime_ms() const
  {
    return _visionSystem->GetMinCameraExposureTime_ms();
  }

  s32 VisionComponent::GetMaxCameraExposureTime_ms() const
  {
    return _visionSystem->GetMaxCameraExposureTime_ms();
  }

  f32 VisionComponent::GetMinCameraGain() const
  {
    return _visionSystem->GetMinCameraGain();
  }

  f32 VisionComponent::GetMaxCameraGain() const
  {
    return _visionSystem->GetMaxCameraGain();
  }

  bool VisionComponent::ReleaseImage(Vision::ImageBuffer& buffer)
  {
    bool r = true;
    if (buffer.HasValidData()) {
      auto cameraService = CameraService::getInstance();
      r = cameraService->CameraReleaseFrame(buffer.GetImageId());
      if (r) {
        buffer.Invalidate();
      }
    }
    return r;

  }

  bool VisionComponent::IsProcessingImages()
  {
    // We will be processing images as long as we are not paused
    // or if we are paused then we are only processing images as long
    // we have an image to process
    return !_paused || _visionSystemInput.locked;
  }

  bool VisionComponent::CaptureImage(Vision::ImageBuffer& buffer)
  {
    if(!_enableImageCapture)
    {
      return false;
    }

    if(_captureFormatState == CaptureFormatState::WaitingForProcessingToStop)
    {
      return false;
    }

    auto cameraService = CameraService::getInstance();

    const int numRows = cameraService->CameraGetHeight();
    const int numCols = cameraService->CameraGetWidth();

    if(IsCameraCalibrationSet())
    {
      // This resolution should match the resolution at which we calibrated
      DEV_ASSERT_MSG(numRows == GetCameraCalibration()->GetNrows() &&
                     numCols == GetCameraCalibration()->GetNcols(),
                     "VisionComponent.CaptureAndSendImage.ResolutionMismatch",
                     "Calibration:%dx%d CameraService:%dx%d",
                     GetCameraCalibration()->GetNrows(),
                     GetCameraCalibration()->GetNcols(),
                     numCols,numRows);
    }

    // Get image buffer. Only request an image from a timestamp for which we have both robot state history _and_ IMU
    // history.
    u32 newestStateHistoryTimeStamp = (u32)_robot->GetStateHistory()->GetNewestTimeStamp();
    const auto& imuHistory = _robot->GetImuComponent().GetImuHistory();
    if (!imuHistory.empty()) {
      newestStateHistoryTimeStamp = std::min(newestStateHistoryTimeStamp, (u32) imuHistory.back().timestamp);
    }
    const bool gotImage = cameraService->CameraGetFrame(newestStateHistoryTimeStamp, buffer);
    const EngineTimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    if(gotImage)
    {
      buffer.SetSensorResolution(cameraService->CameraGetSensorHeight(),
                                 cameraService->CameraGetSensorWidth());

      _lastImageCaptureTime_ms = currTime_ms;
    }
    else
    {
      // No image captured, and we're not currently waiting for a format change, nor are we in power save mode
      auto const& powerStateMgr = _robot->GetComponent<PowerStateManager>();
      if (!IsWaitingForCaptureFormatChange() &&
          !powerStateMgr.InPowerSaveMode() &&
          (_lastImageCaptureTime_ms > 0) &&
          (currTime_ms > _lastImageCaptureTime_ms + kMaxExpectedTimeBetweenCapturedFrames_ms))
      {
        LOG_WARNING("VisionComponent.CaptureImage.TooLongSinceFrameWasCaptured",
                    "last: %dms, now: %dms",
                    (TimeStamp_t)_lastImageCaptureTime_ms, (TimeStamp_t)currTime_ms);
      }
    }

    return gotImage;
  }

  void VisionComponent::SetLiftCrossBar()
  {
    const f32 padding = LIFT_HARDWARE_FALL_SLACK_MM;
    const std::vector<Point3f> liftCrossBar{
      // NOTE: adding points for front and back because which will be outermost in projection
      //       depends on lift angle, so let Occluder, which uses bounding box of all the
      //       points, take care of that for us

      // Top:
      Point3f(LIFT_FRONT_WRT_WRIST_JOINT, -LIFT_XBAR_WIDTH*0.5f, LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT),
      Point3f(LIFT_FRONT_WRT_WRIST_JOINT,  LIFT_XBAR_WIDTH*0.5f, LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT),
      Point3f(LIFT_BACK_WRT_WRIST_JOINT,  -LIFT_XBAR_WIDTH*0.5f, LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT),
      Point3f(LIFT_BACK_WRT_WRIST_JOINT,   LIFT_XBAR_WIDTH*0.5f, LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT),

      // Bottom
      Point3f(LIFT_FRONT_WRT_WRIST_JOINT, -LIFT_XBAR_WIDTH*0.5f, LIFT_XBAR_BOTTOM_WRT_WRIST_JOINT - padding),
      Point3f(LIFT_FRONT_WRT_WRIST_JOINT,  LIFT_XBAR_WIDTH*0.5f, LIFT_XBAR_BOTTOM_WRT_WRIST_JOINT - padding),
      Point3f(LIFT_BACK_WRT_WRIST_JOINT,  -LIFT_XBAR_WIDTH*0.5f, LIFT_XBAR_BOTTOM_WRT_WRIST_JOINT - padding),
      Point3f(LIFT_BACK_WRT_WRIST_JOINT,   LIFT_XBAR_WIDTH*0.5f, LIFT_XBAR_BOTTOM_WRT_WRIST_JOINT - padding),
    };

    _liftCrossBarSource = std::move(liftCrossBar);
  }

  bool VisionComponent::SetCameraCaptureFormat(Vision::ImageEncoding format)
  {
    // If the future is already valid then we are currently waiting for
    // a previous format change to take effect
    if(_captureFormatState != CaptureFormatState::None)
    {
      LOG_WARNING("VisionComponent.SetCameraCaptureFormat.StillSettingPrevFormat",
                  "Still waiting for previous format %s to be applied",
                  EnumToString(_desiredImageFormat));
      return false;
    }

    Vision::ImageEncoding currentFormat = GetCurrentImageFormat();
    // Don't do anything if already in requested format
    if(format == currentFormat)
    {
      return true;
    }

    // Pause and wait for VisionSystem to finish processing the current image
    // before changing formats since that will release all shared camera memory
    Pause(true);

    _desiredImageFormat = format;

    _captureFormatState = CaptureFormatState::WaitingForProcessingToStop;

    LOG_INFO("VisionComponent.SetCameraCaptureFormat.RequestingSwitch",
             "From %s to %s",
             ImageEncodingToString(currentFormat), ImageEncodingToString(_desiredImageFormat));
    
    return true;
  }

  void VisionComponent::UpdateCaptureFormatChange(s32 gotNumRows)
  {
    switch(_captureFormatState)
    {
      case CaptureFormatState::None:
      {
        return;
      }

      case CaptureFormatState::WaitingForProcessingToStop:
      {
        Lock();

        // If we don't have an image to process
        // meaning the VisionSystem has finished processing
        if(!_visionSystemInput.locked)
        {
          // Make sure to also clear image cache
          _visionSystem->ClearImageCache();

          // Tell the camera to change format now that nothing is
          // using the shared camera memory
          auto cameraService = CameraService::getInstance();
          cameraService->CameraSetCaptureFormat(_desiredImageFormat);

          LOG_INFO("VisionComponent.UpdateCaptureFormatChange.SwitchToWaitForFrame",
                   "Now in %s", ImageEncodingToString(_desiredImageFormat));
          
          _captureFormatState = CaptureFormatState::WaitingForFrame;
        }

        Unlock();

        return;
      }

      case CaptureFormatState::WaitingForFrame:
      {
        LOG_INFO("VisionComponent.UpdateCaptureFormatChange.WaitingForFrameWithNewFormat", "");
        
        s32 expectedNumRows = 0;
        switch(_desiredImageFormat)
        {
          case Vision::ImageEncoding::RawRGB:
            expectedNumRows = DEFAULT_CAMERA_RESOLUTION_HEIGHT;
            break;

          case Vision::ImageEncoding::YUV420sp:
            expectedNumRows = CAMERA_SENSOR_RESOLUTION_HEIGHT;
            break;

          case Vision::ImageEncoding::BAYER:
            expectedNumRows = CAMERA_SENSOR_RESOLUTION_HEIGHT;
            break;

          default:
            LOG_ERROR("VisionComponent.UpdateCaptureFormatChange.BadDesiredFormat", "%s",
                      ImageEncodingToString(_desiredImageFormat));
            return;
        }

        if(gotNumRows == expectedNumRows)
        {
          DEV_ASSERT(_paused, "VisionComponent.UpdateCaptureFormatChange.ExpectingVisionComponentToBePaused");

          LOG_INFO("VisionComponent.UpdateCaptureFormatChange.FormatChangeComplete",
                   "New format: %s, NumRows=%d", ImageEncodingToString(_desiredImageFormat), gotNumRows);
          
          _captureFormatState = CaptureFormatState::None;
          _desiredImageFormat = Vision::ImageEncoding::NoneImageEncoding;
          Pause(false); // now that state/format are updated, un-pause the vision system

        }

        return;
      }
    }
  }

  bool VisionComponent::IsWaitingForCaptureFormatChange() const
  {
    return (CaptureFormatState::None != _captureFormatState);
  }

#pragma mark -
#pragma mark Message Handlers

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::EnableVisionMode& payload)
  {
    EnableMode(payload.mode, payload.enable);
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::VisionWhileMoving& msg)
  {
    EnableVisionWhileRotatingFast(msg.enable);
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::VisionRunMode& msg)
  {
    SetIsSynchronous(msg.isSync);
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::EraseEnrolledFaceByID& msg)
  {
    EraseFace(msg.faceID);
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::EraseAllEnrolledFaces& msg)
  {
    EraseAllFaces();
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::RequestEnrolledNames& msg)
  {
    RequestEnrolledNames();
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::SaveFaceAlbumToFile& msg)
  {
    // NOTE: Renames/deletes will propagate to the _faceAlbumName loaded from vision_config! (not msg.path)
    if(msg.isRelativePath) {
      SaveFaceAlbumToFile(GetFullFaceAlbumPath(_context, msg.path));
    }
    else {
      SaveFaceAlbumToFile(msg.path);
    }
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::LoadFaceAlbumFromFile& msg)
  {
    // NOTE: Renames/deletes will propagate to the _faceAlbumName loaded from vision_config! (not msg.path)
    if(msg.isRelativePath) {
      LoadFaceAlbumFromFile(GetFullFaceAlbumPath(_context, msg.path));
    }
    else {
      LoadFaceAlbumFromFile(msg.path);
    }
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::UpdateEnrolledFaceByID& msg)
  {
    RenameFace(msg.faceID, msg.oldName, msg.newName);
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::EnableColorImages& msg)
  {
    // TODO: EnableColorImages probably shouldn't affect what kind of image
    //       VisionComponent deals with, but it could be repurposed to determine
    //       what gets sent up to game.
    LOG_WARNING("VisionComponent.HandleEnableColorImages.NotImplemented", "");
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::SetCameraSettings& payload)
  {
    if(payload.enableAutoExposure)
    {
      LOG_INFO("VisionComponent.HandleSetCameraSettings.Auto",
               "Enabling auto exposure and auto whitebalance");
      EnableAutoExposure(true);
      EnableWhiteBalance(true);
    }
    else
    {
      auto const& currentParams = _visionSystem->GetCurrentCameraParams();
      Vision::CameraParams params(payload.exposure_ms, payload.gain,
                                  currentParams.whiteBalanceGainR,
                                  currentParams.whiteBalanceGainG,
                                  currentParams.whiteBalanceGainB);
      
      LOG_INFO("VisionComponent.HandleSetCameraSettings.Manual",
               "Setting camera params to: Exp:%dms / %.3f, WB:%.3f,%.3f,%.3f",
               params.exposureTime_ms, params.gain,
               params.whiteBalanceGainR, params.whiteBalanceGainG, params.whiteBalanceGainB);
      
      SetAndDisableCameraControl(params);
    }
  }

  void VisionComponent::SetSaveImageParameters(const ImageSaverParams& params)
  {
    if(nullptr != _visionSystem)
    {
      std::string fullPath(params.path);
      if(fullPath.empty())
      {
        // If no path specified, default to <cachePath>/camera/images
        const std::string cachePath = _robot->GetContext()->GetDataPlatform()->pathToResource(Util::Data::Scope::Cache, "camera");
        fullPath = Util::FileUtils::FullFilePath({cachePath, "images"});
      }

      _visionSystem->SetSaveParameters(params);

      if(params.mode != ImageSendMode::Off)
      {
        EnableMode(VisionMode::SaveImages, true);
      }

      LOG_DEBUG("VisionComponent.SetSaveImageParameters.SaveImages",
                "Setting image save mode to %s. Saving to: %s",
                EnumToString(params.mode), fullPath.c_str());
    }
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::SaveImages& payload)
  {
    ImageSaverParams params(payload.path, payload.mode, payload.qualityOnRobot, "",
                            Vision::ImageCacheSize::Half, 0.f, 1.f, payload.removeRadialDistortion);
    SetSaveImageParameters(params);
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::SetCameraCaptureFormat& msg)
  {
    SetCameraCaptureFormat(msg.format);
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::RobotConnectionResponse& msg)
  {
    if (msg.result == RobotConnectionResult::Success)
    {
      NVStorageComponent::NVStorageReadCallback readCamCalibCallback = [this](u8* data, size_t size, NVStorage::NVResult res)
      {
        if (res == NVStorage::NVResult::NV_OKAY) {
          CameraCalibration payload;

          if (size != NVStorageComponent::MakeWordAligned(payload.Size())) {
            LOG_WARNING("VisionComponent.ReadCameraCalibration.SizeMismatch",
                        "Expected %zu, got %zu",
                        NVStorageComponent::MakeWordAligned(payload.Size()), size);
            FaultCode::DisplayFaultCode(FaultCode::NO_CAMERA_CALIB);
            return;
          } else {

            payload.Unpack(data, size);

            std::stringstream ss;
            ss << "[";
            for(int i = 0; i < payload.distCoeffs.size() - 1; ++i)
            {
              ss << payload.distCoeffs[i] << ", ";
            }
            ss << payload.distCoeffs.back() << "]";

            LOG_INFO("VisionComponent.ReadCameraCalibration.Recvd",
                             "Received new %dx%d camera calibration from robot. (fx: %f, fy: %f, cx: %f, cy: %f, distCoeffs: %s)",
                             payload.ncols, payload.nrows,
                             payload.focalLength_x, payload.focalLength_y,
                             payload.center_x, payload.center_y,
                             ss.str().c_str());

            // Convert calibration data in payload to a shared CameraCalibration object
            auto calib = std::make_shared<Vision::CameraCalibration>(payload.nrows,
                                                                     payload.ncols,
                                                                     payload.focalLength_x,
                                                                     payload.focalLength_y,
                                                                     payload.center_x,
                                                                     payload.center_y,
                                                                     payload.skew,
                                                                     payload.distCoeffs);

            SetCameraCalibration(calib);

            // Compute FOV from focal length and send
            CameraFOVInfo msg(calib->ComputeHorizontalFOV().ToFloat(), calib->ComputeVerticalFOV().ToFloat());
            if (_robot->SendMessage(RobotInterface::EngineToRobot(std::move(msg))) != RESULT_OK) {
              LOG_WARNING("VisionComponent.ReadCameraCalibration.SendCameraFOVFailed", "");
            }
          }
        }
        // If this is the factory test and we failed to read calibration then use a dummy one
        // since we should be getting a real one during playpen
        else if(FACTORY_TEST)
        {
          LOG_WARNING("VisionComponent.ReadCameraCalibration.Failed", "");

          // TEMP HACK: Use dummy calibration for now since final camera not available yet
          LOG_WARNING("VisionComponent.ReadCameraCalibration.UsingDummyV2Calibration", "");

          // Calibration computed from Inverted Box target using one of the proto robots
          // Should be close enough for other robots without calibration to use
          const std::array<f32, 8> distortionCoeffs = {{-0.03822904514363595, -0.2964213946476391, -0.00181089972406104, 0.001866070303033584, 0.1803429725181202,
            0, 0, 0}};

          auto calib = std::make_shared<Vision::CameraCalibration>(360,
                                          640,
                                          364.7223064012286,
                                          366.1693698832141,
                                          310.6264440545544,
                                          196.6729350209868,
                                          0,
                                          distortionCoeffs);

          SetCameraCalibration(calib);

          // Compute FOV from focal length and send
          CameraFOVInfo msg(calib->ComputeHorizontalFOV().ToFloat(), calib->ComputeVerticalFOV().ToFloat());
          if (_robot->SendMessage(RobotInterface::EngineToRobot(std::move(msg))) != RESULT_OK) {
            LOG_WARNING("VisionComponent.ReadCameraCalibration.SendCameraFOVFailed", "");
          }
        }
        else
        {
          LOG_ERROR("VisionComponent.ReadCameraCalibration.Failed", "");
          FaultCode::DisplayFaultCode(FaultCode::NO_CAMERA_CALIB);
          return;
        }

        Enable(true);
      };

      _robot->GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_CameraCalib, readCamCalibCallback);

    }
  }

  void VisionComponent::AddAllowedTrackedFace(const Vision::FaceID_t faceID)
  {
    _visionSystem->AddAllowedTrackedFace(faceID);
  }

  void VisionComponent::ClearAllowedTrackedFaces()
  {
    _visionSystem->ClearAllowedTrackedFaces();
  }

  void VisionComponent::SendImages(VisionProcessingResult& result)
  {
    if(ANKI_DEV_CHEATS)
    {
      // Display any debug images left by the vision system
      if(kSendDebugImages)
      {
        for(auto & debugGray : result.debugImages)
        {
          SendCompressedImage(debugGray.second, debugGray.first);
        }
      }
    }
    else if(!result.debugImages.empty())
    {
      // We do not expect to have debug images to draw without dev cheats enabled
      std::string str;
      for(auto & debugImg : result.debugImages)
      {
        str += debugImg.first;
        str += " ";
      }

      LOG_ERROR("VisionComponent.UpdateAllResults.DebugImagesPresent",
                "%s", str.c_str());
    }

    if(result.modesProcessed.Contains(VisionMode::Viz))
    {
      SendCompressedImage(result.compressedDisplayImg, "camera");
    }
  }

  void VisionComponent::UpdateForCalibration()
  {
    // VIC-7177 Fix storing images for camera calibration
    // Store image for calibration or factory test (*before* we swap image with _nextImg below!)
    // NOTE: This means we do decoding on main thread, but this is just for the factory
    //       test, so I'm not going to the trouble to store encoded images for calibration
    // if (_storeNextImageForCalibration || _doFactoryDotTest)
    // {
    //   // If we were moving too fast at the timestamp the image was taken then don't use it for
    //   // calibration or dot test purposes
    //   auto const& imuHistory = _robot->GetImuComponent().GetImuHistory();
    //   const bool wasRotatingTooFast = (_visionWhileMovingFastEnabled ?
    //                                    false :
    //                                    imuHistory.WasRotatingTooFast(image.GetTimestamp(),
    //                                                                  DEG_TO_RAD(0.1),
    //                                                                  DEG_TO_RAD(0.1), 3));
    //   if(!wasRotatingTooFast)
    //   {
    //     Vision::Image imageGray = image.ToGray();

    //     if(_storeNextImageForCalibration)
    //     {
    //       _storeNextImageForCalibration = false;
    //       if (IsModeEnabled(VisionMode::Calibration)) {
    //         LOG_INFO("VisionComponent.SetNextImage.SkippingStoringImageBecauseAlreadyCalibrating", "");
    //       } else {
    //         Lock();
    //         Result result = _visionSystem->AddCalibrationImage(imageGray, _calibTargetROI);
    //         Unlock();

    //         if(RESULT_OK != result) {
    //           LOG_INFO("VisionComponent.SetNextImage.AddCalibrationImageFailed", "");
    //         }
    //       }
    //     } // if(_storeNextImageForCalibration)

    //     if(_doFactoryDotTest)
    //     {
    //       _doFactoryDotTest = false;

    //       ExternalInterface::RobotCompletedFactoryDotTest msg;
    //       Result dotResult = FindFactoryTestDotCentroids(imageGray, msg);
    //       if(RESULT_OK != dotResult) {
    //         LOG_WARNING("VisionComponent.SetNextImage.FactoryDotTestFailed", "");
    //       }
    //       _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));

    //     } // if(_doFactoryDotTest)

    //   }
    //   else {
    //     LOG_DEBUG("VisionComponent.SetNextImage.SkippingStorageForCalibrationBecauseMoving", "");
    //   }
    // } // if (_storeNextImageForCalibration || _doFactoryDotTest)
  }

  Vision::ImageEncoding VisionComponent::GetCurrentImageFormat() const
  {
    return _visionSystemInput.imageBuffer.GetFormat();
  }

  void VisionComponent::EnableImageCapture(bool enable)
  {
    LOG_INFO("VisionComponent.EnableImageCapture",
                     "%s image capture",
                     (enable ? "Enabling" : "Disabling"));

    // If going from disabled to enabled then reset
    // _lastProcessedImageTimeStamp_ms since it could be really old
    if(!_enableImageCapture && enable)
    {
      _lastProcessedImageTimeStamp_ms = 0;
      _lastReceivedImageTimeStamp_ms = 0;
    }

    _enableImageCapture = enable;
    CameraService::getInstance()->PauseCamera(!enable);
  }

  void VisionComponent::CaptureOneFrame()
  {
    // If image capture is already enabled this will do nothing
    // otherwise it will enable image capture so we can capture a single image
    // Image capture will automatically be disabled once an image is captured
    EnableImageCapture(true);
    _captureOneImage = true;
  }

  void VisionComponent::SetupVisionModeConsoleVars()
  {
    #if REMOTE_CONSOLE_ENABLED
    for(auto  m = VisionMode(0); m < VisionMode::Count; m++)
    {
      auto& pair = _visionModeConsoleVars[static_cast<u32>(m)];
      pair.first = new Util::ConsoleVar<bool>(pair.second,
                                              EnumToString(m),
                                              "Vision.General.VisionModes",
                                              false);
    }
    #endif
  }

  void VisionComponent::UpdateVisionModeConsoleVars()
  {
    #if REMOTE_CONSOLE_ENABLED
    // Keep track of previous console var values to know when the new ones change
    static std::array<bool, static_cast<u32>(VisionMode::Count)> prevConsoleVars;

    for(int i = 0; i < _visionModeConsoleVars.size(); i++)
    {
      auto& pair = _visionModeConsoleVars[i];

      // If the console var value has changed
      if(pair.second != prevConsoleVars[i])
      {
        // Enable/disable the vision mode as well as subscribing/unsubscribing to the vision mode
        EnableMode(static_cast<VisionMode>(i), pair.second);

        // Need to subscribe/unsubscribe as when you are enabling/disabling VisionModes with the
        // console var it is very likely not scheduled. The regular subscription mechanism will
        // remove all subscriptions to all modes for the subscriber as it accepts a full list of
        // all desired subscriptions. The DevOnly calls allow you to update individual mode
        // subscriptions for the subscriber.
        if(prevConsoleVars[i])
        {
          _robot->GetVisionScheduleMediator().DevOnly_SelfUnsubscribeVisionMode({static_cast<VisionMode>(i)});
        }
        else
        {
          _robot->GetVisionScheduleMediator().DevOnly_SelfSubscribeVisionMode({static_cast<VisionMode>(i)});
        }

        prevConsoleVars[i] = pair.second;
      }
    }
    #endif
  }

  void VisionComponent::EnableImageSending(bool enable)
  {
    EnableMode(VisionMode::Viz, enable);
  }

  void VisionComponent::EnableMirrorMode(bool enable)
  {
    EnableMode(VisionMode::MirrorMode, enable);
  }

  void VisionComponent::UpdateImageReceivedChecks(bool gotImage)
  {
    // Display a fault code in case we have not received an image from the camera for some amount of time
    // and we aren't in power save mode in which case we expect to not be receiving images
    // and image capture is enabled
    // This is a catch-all for any issues with the camera server/daemon should something go wrong and we
    // stop receiving images
    static const EngineTimeStamp_t kNoImageFaultCodeTimeout_ms = 60000;

    // If we haven't got an image in this amount of time, try deleting and restarting the camera
    // service just in case that knocks something loose
    static const EngineTimeStamp_t kNoImageRestartCamera_ms = kNoImageFaultCodeTimeout_ms/4;
    static const EngineTimeStamp_t kRestartCameraDelay_ms = 1000;
    static EngineTimeStamp_t sTimeSinceValidImg_ms = 0;

    const bool inPowerSave = _robot->GetComponent<PowerStateManager>().InPowerSaveMode();
    if(!gotImage && !inPowerSave && _enableImageCapture)
    {
      const EngineTimeStamp_t curTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      if(sTimeSinceValidImg_ms == 0)
      {
        sTimeSinceValidImg_ms = curTime_ms;
      }
      // If it has been way too long since we last received an image, show a fault code
      else if(curTime_ms - sTimeSinceValidImg_ms > kNoImageFaultCodeTimeout_ms)
      {
        FaultCode::DisplayFaultCode(FaultCode::CAMERA_STOPPED);
        sTimeSinceValidImg_ms = 0;
      }
      // If we haven't received an image for some amount of time, try restarting camera
      else if(curTime_ms - sTimeSinceValidImg_ms > kNoImageRestartCamera_ms)
      {
        // First we need to stop the camera
        if(_restartingCameraTime_ms == 0)
        {
          _restartingCameraTime_ms = curTime_ms;
          LOG_WARNING("VisionComponent.Update.StoppingCamera",
                      "Too long without valid image, restarting camera");
          auto cameraService = CameraService::getInstance();
          cameraService->DeleteCamera();
        }
        // Some time after stopping the camera, try to start it back up
        // Stopping/Starting are asynchonous so we need to wait a bit between the calls
        else if(_restartingCameraTime_ms != 0 &&
                curTime_ms - _restartingCameraTime_ms > kRestartCameraDelay_ms)
        {
          // Prevent the camera restart checks from triggering again until we either
          // start getting images again or the CAMERA_STOPPED fault code triggers
          _restartingCameraTime_ms = 1;
          LOG_WARNING("VisionComponent.Update.RestartingCamera",
                      "Too long without valid image, starting camera back up");
          auto cameraService = CameraService::getInstance();
          cameraService->InitCamera();
        }
      }
    }
    else
    {
      sTimeSinceValidImg_ms = 0;
      _restartingCameraTime_ms = 0;
    }
  }

} // namespace Vector
} // namespace Anki
