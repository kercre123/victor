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


#include "engine/actions/basicActions.h"
#include "camera/cameraService.h"
#include "engine/ankiEventUtil.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/animationComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/photographyManager.h"
#include "engine/components/visionComponent.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/navMap/mapComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/faceWorld.h"
#include "engine/petWorld.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotStateHistory.h"
#include "engine/vision/visionModesHelpers.h"
#include "engine/vision/visionSystem.h"
#include "engine/viz/vizManager.h"

#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/image_impl.h"
#include "coretech/vision/engine/trackedFace.h"
#include "coretech/vision/engine/observableObjectLibrary_impl.h"
#include "coretech/vision/engine/visionMarker.h"
#include "coretech/vision/shared/MarkerCodeDefinitions.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/opencvThreading.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/robot/config.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/boundedWhile.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/logging/DAS.h"
#include "util/threading/threadPriority.h"
#include "util/bitFlags/bitFlags.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/imageTypes.h"

#include "aiComponent/beiConditions/conditions/conditionEyeContact.h"

#include "opencv2/highgui/highgui.hpp"

namespace Anki {
namespace Cozmo {

#if ANKI_CPU_PROFILER_ENABLED
  CONSOLE_VAR_ENUM(u8, kVisionComponent_Logging,   ANKI_CPU_CONSOLEVARGROUP, 0, Util::CpuProfiler::CpuProfilerLogging());
#endif

  CONSOLE_VAR(f32, kHeadTurnSpeedThreshBlock_degs, "WasRotatingTooFast.Block.Head_deg/s",   10.f);
  CONSOLE_VAR(f32, kBodyTurnSpeedThreshBlock_degs, "WasRotatingTooFast.Block.Body_deg/s",   30.f);
  CONSOLE_VAR(u8,  kNumImuDataToLookBack,          "WasRotatingTooFast.Face.NumToLookBack", 5);

  CONSOLE_VAR(bool, kDisplayProcessedImagesOnly, "Vision.General", true);

  // Quality of images sent to game/viz
  // Set to -1 to display "locally" with img.Display()
  // Set to 0 to disable sending altogether (to save bandwidth) -- disables camera feed AND debug images
  CONSOLE_VAR(s32,  kImageCompressQuality,  "Vision.General", 50);

  // Whether or not to do rolling shutter correction for physical robots
  CONSOLE_VAR(bool, kRollingShutterCorrectionEnabled, "Vision.PreProcessing", true);
  CONSOLE_VAR(f32,  kMinCameraGain,                   "Vision.PreProcessing", 0.1f);

  // Amount of time we sleep when paused, waiting for next image, and after processing each image
  // (in order to provide a little breathing room for main thread)
  CONSOLE_VAR_RANGED(u8, kVision_MinSleepTime_ms, "Vision.General", 2, 1, 10);

  // Set to a value greater than 0 to randomly drop that fraction of frames, for testing
  CONSOLE_VAR_RANGED(f32, kSimulateDroppedFrameFraction, "Vision.General", 0.f, 0.f, 1.f); // DO NOT COMMIT > 0!

  CONSOLE_VAR(bool, kVisualizeObservedMarkersIn3D,  "Vision.General", false);
  // If > 0, displays detected marker names in Viz Camera Display (still at fixed scale) and
  // and in mirror mode (at specified scale)
  CONSOLE_VAR_RANGED(f32,  kDisplayMarkerNamesScale,"Vision.General", 0.f, 0.f, 1.f);
  CONSOLE_VAR(bool, kDisplayUndistortedImages,      "Vision.General", false);

  CONSOLE_VAR(bool, kEnableMirrorMode,              "Vision.General", false);
  CONSOLE_VAR(bool, kDisplayDetectionsInMirrorMode, "Vision.General", false); // objects, faces, markers
  CONSOLE_VAR(bool, kDisplayEyeContactInMirrorMode, "Vision.General", false);
  CONSOLE_VAR(f32,  kMirrorModeGamma,               "Vision.General", 1.f);

  // Hack to continue drawing detected objects for a bit after they are detected
  // since object detection is slow
  CONSOLE_VAR(u32, kKeepDrawingSalientPointsFor_ms, "Vision.General", 150);

  namespace JsonKey
  {
    const char * const ImageQualityGroup = "ImageQuality";
    const char * const PerformanceLogging = "PerformanceLogging";
    const char * const DropStatsWindowLength = "DropStatsWindowLength_sec";
    const char * const ImageQualityAlertDuration = "TimeBeforeErrorMessage_ms";
    const char * const ImageQualityAlertSpacing = "RepeatedErrorMessageInterval_ms";
    const char * const InitialExposureTime = "InitialExposureTime_ms";
    const char * const OpenCvThreadMode = "NumOpenCvThreads";
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
  }

  VisionComponent::VisionComponent()
  : IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::Vision)
  {
  } // VisionSystem()


  void VisionComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps)
  {
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
      helper.SubscribeGameToEngine<MessageGameToEngineTag::DevSubscribeVisionModes>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::DevUnsubscribeVisionModes>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SetCameraCaptureFormat>();

      // Separate list for engine messages to listen to:
      helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotConnectionResponse>();
    }

    // "Special" viz identifier for the main camera feed
    _vizDisplayIndexMap["camera"] = 0;

    auto& context = dependentComps.GetComponent<ContextWrapper>().context;

    if (nullptr != context->GetDataPlatform())
    {
      ReadVisionConfig(context->GetDataLoader()->GetRobotVisionConfig());
    }
  }

  void VisionComponent::ReadVisionConfig(const Json::Value& config)
  {
    _isInitialized = false;

    // Helper macro for grabbing a parameter from Json config and printing an
    // error message and returning failure if it doesn't exist
#   define GET_JSON_PARAMETER(__json__, __fieldName__, __variable__) \
    do { \
      if(!JsonTools::GetValueOptional(__json__, __fieldName__, __variable__)) { \
        PRINT_NAMED_ERROR("Vision.Init.MissingJsonParameter", "%s", __fieldName__); \
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
      PRINT_NAMED_ERROR("VisionComponent.Init.VisionSystemInitFailed", "");
      return;
    }

    // Request face album data from the robot
    std::string faceAlbumName;
    JsonTools::GetValueOptional(config, "FaceAlbum", faceAlbumName);
    if(faceAlbumName.empty() || faceAlbumName == "robot") {
      result = LoadFaceAlbumFromRobot();
      if(RESULT_OK != result) {
        PRINT_NAMED_WARNING("VisionComponent.Init.LoadFaceAlbumFromRobotFailed", "");
      }
    } else {
      // Erase all faces on the robot
      EraseAllFaces();

      std::list<Vision::LoadedKnownFace> loadedFaces;
      result = _visionSystem->LoadFaceAlbum(faceAlbumName, loadedFaces);
      BroadcastLoadedNamesAndIDs(loadedFaces);

      if(RESULT_OK != result) {
        PRINT_NAMED_WARNING("VisionComponent.Init.LoadFaceAlbumFromFileFailed",
                            "AlbumFile: %s", faceAlbumName.c_str());
      }
    }

    const f32 kCameraFrameRate_fps = 15.f;
    _dropStats.SetChannelName("VisionComponent");
    _dropStats.SetRecentWindowLength(kDropStatsWindowLength_sec * kCameraFrameRate_fps);

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
      EnableVisionWhileMovingFast(false);
    }
  } // SetCameraCalibration()


  void VisionComponent::SetIsSynchronous(bool isSynchronous) {
    if(isSynchronous && !_isSynchronous) {
      PRINT_NAMED_INFO("VisionComponent.SetSynchronousMode.SwitchToSync", "");
      if(_running) {
        Stop();
      }
      _isSynchronous = true;
    }
    else if(!isSynchronous && _isSynchronous) {
      PRINT_NAMED_INFO("VisionComponent.SetSynchronousMode.SwitchToAsync", "");
      _isSynchronous = false;
      Start();
    }
    _visionSystem->SetFaceRecognitionIsSynchronous(_isSynchronous);
  }

  void VisionComponent::Start()
  {
    if(!IsCameraCalibrationSet()) {
      PRINT_NAMED_ERROR("VisionComponent.Start",
                        "Camera calibration must be set to start VisionComponent.");
      return;
    }

    if(_running) {
      PRINT_NAMED_INFO("VisionComponent.Start.Restarting",
                       "Thread already started, calling Stop() and then restarting (paused:%d).",
                       _paused);
      Stop();
    } else {
      PRINT_NAMED_INFO("VisionComponent.Start",
                       "Starting vision processing thread (paused:%d)",
                       _paused);
    }

    _running = true;
    // Note that we're giving the Processor a pointer to "this", so we
    // have to ensure this VisionSystem object outlives the thread.
    _processingThread = std::thread(&VisionComponent::Processor, this);
    //_processingThread.detach();

  }

  void VisionComponent::Stop()
  {
    _running = false;

    // Wait for processing thread to die before destructing since we gave it
    // a reference to *this
    if(_processingThread.joinable()) {
      _processingThread.join();
    }

    _currentImg.Clear();
  }


  VisionComponent::~VisionComponent()
  {
    Vision::Image::CloseAllDisplayWindows();
    Vision::ImageRGB::CloseAllDisplayWindows();

    Stop();

    Util::SafeDelete(_visionSystem);
  } // ~VisionSystem()

  TimeStamp_t VisionComponent::GetLastProcessedImageTimeStamp() const
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
    if(nullptr != _visionSystem) {
      return _visionSystem->SetNextMode(mode, enable);
    } else {
      PRINT_NAMED_ERROR("VisionComponent.EnableMode.NullVisionSystem", "");
      return RESULT_FAIL;
    }
  }

  Result VisionComponent::PushNextModeSchedule(AllVisionModesSchedule&& schedule)
  {
    if(nullptr != _visionSystem) {
      return _visionSystem->PushNextModeSchedule(std::move(schedule));
    } else {
      PRINT_NAMED_ERROR("VisionComponent.PushModeSchedule.NullVisionSystem", "");
      return RESULT_FAIL;
    }
  }

  Result VisionComponent::PopCurrentModeSchedule()
  {
    if(nullptr != _visionSystem) {
      return _visionSystem->PopModeSchedule();
    } else {
      PRINT_NAMED_ERROR("VisionComponent.PopModeSchedule.NullVisionSystem", "");
      return RESULT_FAIL;
    }
  }

  bool VisionComponent::IsModeEnabled(VisionMode mode) const
  {
    if(nullptr != _visionSystem) {
      return _visionSystem->IsModeEnabled(mode);
    } else {
      return false;
    }
  }

  static Result GetImageHistState(const Robot&      robot,
                                  const TimeStamp_t imageTimeStamp,
                                  HistRobotState&   imageHistState,
                                  TimeStamp_t&      imageHistTimeStamp)
  {
    // Handle the (rare, Webots-test-only?) possibility that the image timstamp is _newer_
    // than the latest thing in history. In that case, we'll just use the last pose information
    // we have, since we can't really interpolate.
    const TimeStamp_t requestedTimeStamp = std::min(imageTimeStamp, robot.GetStateHistory()->GetNewestTimeStamp());

    Result lastResult = robot.GetStateHistory()->ComputeStateAt(requestedTimeStamp, imageHistTimeStamp, imageHistState, true);

    return lastResult;
  }

  void VisionComponent::UpdateDependent(const RobotCompMap& dependentComps)
  {
    if(!_isInitialized) {
      PRINT_NAMED_WARNING("VisionComponent.Update.NotInitialized", "");
      return;
    }

    if (!_enabled) {
      PRINT_PERIODIC_CH_INFO(200, "VisionComponent", "VisionComponent.Update.NotEnabled", "If persistent, camera calibration is probably missing");
      return;
    }

    if(!IsCameraCalibrationSet())
    {
      PRINT_NAMED_WARNING("VisionComponent.Update.NoCameraCalibration",
                          "Camera calibration should be set before calling UpdateDependent().");
      return;
    }

    // Don't bother updating while we are waiting for a capture format change to take effect
    UpdateCaptureFormatChange();
    if(CaptureFormatState::WaitingForProcessingToStop == _captureFormatState)
    {
      return;
    }

    if(_bufferedImg.IsEmpty())
    {
      // We don't yet have a next image. Get one from camera.
      const bool gotImage = CaptureImage(_bufferedImg);
      _hasStartedCapturingImages = true;

      if(gotImage)
      {
        DEV_ASSERT(!_bufferedImg.IsEmpty(), "VisionComponent.Update.EmptyImageAfterCapture");

        UpdateCaptureFormatChange(_bufferedImg.GetNumRows());
        if(CaptureFormatState::WaitingForFrame == _captureFormatState)
        {
          return;
        }
        
        // Compress to jpeg and send to game and viz
        // Do this before setting next image since it swaps the image and invalidates it
        Result lastResult = CompressAndSendImage(_bufferedImg, kImageCompressQuality, "camera");
        DEV_ASSERT(RESULT_OK == lastResult, "VisionComponent.CompressAndSendImage.Failed");

        // Track how fast we are receiving frames
        if(_lastReceivedImageTimeStamp_ms > 0) {
          // Time should not move backwards!
          const bool timeWentBackwards = _bufferedImg.GetTimestamp() < _lastReceivedImageTimeStamp_ms;
          if (timeWentBackwards)
          {
            PRINT_NAMED_WARNING("VisionComponent.SetNextImage.UnexpectedTimeStamp",
                                "Current:%u Last:%u",
                                _bufferedImg.GetTimestamp(), _lastReceivedImageTimeStamp_ms);

            // This should be recoverable (it could happen if we receive a bunch of garbage image data)
            // so reset the lastReceived and lastProcessed timestamps so we can set them fresh next time
            // we get an image
            _lastReceivedImageTimeStamp_ms = 0;
            _lastProcessedImageTimeStamp_ms = 0;
            ReleaseImage(_bufferedImg);
            return;
          }
          _framePeriod_ms = _bufferedImg.GetTimestamp() - _lastReceivedImageTimeStamp_ms;
        }
        _lastReceivedImageTimeStamp_ms = _bufferedImg.GetTimestamp();
      }
    }

    if(_bufferedImg.IsEmpty()) // Recheck, b/c we may have just captured one
    {
      // If we still don't have a buffered image, log a DEBUG message
      if(!FACTORY_TEST)
      {
        PRINT_CH_DEBUG("VisionComponent", "VisionComponent.Update.WaitingForBufferedImage", "Tick:%zu",
                       BaseStationTimer::getInstance()->GetTickCount());
      }
    }
    else // !_bufferedImg.IsEmpty()
    {
      // Have an image buffered, so now try to get the corresponding historical state

      const bool imageOlderThanOldestState = (_bufferedImg.GetTimestamp() < _robot->GetStateHistory()->GetOldestTimeStamp());
      if(imageOlderThanOldestState)
      {
        // Special case: we're trying to process an image with a timestamp older than the oldest thing in
        // state history. This can happen at startup, or possibly when we delocalize and clear state
        // history. Just drop this image.
        PRINT_CH_DEBUG("VisionComponent", "VisionComponent.Update.DroppingImageOlderThanStateHistory",
                       "ImageTime=%d OldestState=%d",
                       _bufferedImg.GetTimestamp(), _robot->GetStateHistory()->GetOldestTimeStamp());

        ReleaseImage(_bufferedImg);

        return;
      }

      // Do we have anything in state history at least as new as this image yet?
      // If so, go ahead and use the buffered image to set the "next" image to be processed.
      // If not, wait until next Update(), when we'll still have this _bufferedImg
      //  and will recheck to see if we've got the correspondind robot state info in history yet.
      const bool haveHistStateAtLeastAsNewAsImage = (_robot->GetStateHistory()->GetNewestTimeStamp() >= _bufferedImg.GetTimestamp());
      if(!haveHistStateAtLeastAsNewAsImage)
      {
        // These messages are too spammy for factory test
        if(!FACTORY_TEST)
        {
          PRINT_CH_DEBUG("VisionComponent", "VisionComponent.Update.WaitingForState",
                         "CapturedImageTime:%u NewestStateInHistory:%u",
                         _bufferedImg.GetTimestamp(), _robot->GetStateHistory()->GetNewestTimeStamp());
        }
      }
      else
      {
        // At this point, we have an image + robot state, continue processing

        // "Mirror mode": draw images we process to the robot's screen
        if(_drawImagesToScreen || kEnableMirrorMode)
        {
          // TODO: Add this as a lambda you can register with VisionComponent for things you want to
          //       do with image when captured?

          // Send as face display animation
          auto & animComponent = _robot->GetAnimationComponent();
          if (animComponent.GetAnimState_NumProcAnimFaceKeyframes() < 5) // Don't get too far ahead
          {
            static Vision::ImageRGB screenImg(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
            _bufferedImg.Resize(screenImg, Vision::ResizeMethod::NearestNeighbor);

            // Flip image around the y axis (before we draw anything on it)
            cv::flip(screenImg.get_CvMat_(), screenImg.get_CvMat_(), 1);

            for(auto & modFcn : _screenImageModFuncs)
            {
              modFcn(screenImg);
            }
            _screenImageModFuncs.clear();

            static Vision::ImageRGB565 img565(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);

            // Use gamma to make it easier to see
            static std::array<u8,256> gammaLUT{};
            static f32 currentGamma = 0.f;
            if(!Util::IsFltNear(currentGamma, kMirrorModeGamma)) {
              currentGamma = kMirrorModeGamma;
              const f32 invGamma = 1.f / currentGamma;
              const f32 divisor = 1.f / 255.f;
              for(s32 value=0; value<256; ++value)
              {
                gammaLUT[value] = std::round(255.f * std::powf((f32)value * divisor, invGamma));
              }
            }

            img565.SetFromImageRGB(screenImg, gammaLUT);

            animComponent.DisplayFaceImage(img565, AnimationComponent::DEFAULT_STREAMING_FACE_DURATION_MS, false);
          }
        }

        SetNextImage(_bufferedImg);
        ReleaseImage(_bufferedImg);
      } // if(!haveHistStateAtLeastAsNewAsImage)
    } // if(_bufferedImg.IsEmpty())


    UpdateAllResults();
    return;
  }

  Result VisionComponent::SetNextImage(Vision::ImageRGB& image)
  {

    // Fill in the pose data for the given image, by querying robot history
    HistRobotState imageHistState;
    TimeStamp_t imageHistTimeStamp;

    Result lastResult = GetImageHistState(*_robot, image.GetTimestamp(), imageHistState, imageHistTimeStamp);

    if(lastResult == RESULT_FAIL_ORIGIN_MISMATCH)
    {
      // Don't print a warning for this case: we expect not to get pose history
      // data successfully
      PRINT_CH_INFO("VisionComponent", "VisionComponent.SetNextImage.OriginMismatch",
                    "Could not get pose data for t=%u due to origin mismatch. Returning OK", image.GetTimestamp());
      return RESULT_OK;
    }
    else if(lastResult != RESULT_OK)
    {
      PRINT_NAMED_WARNING("VisionComponent.SetNextImage.StateHistoryFail",
                          "Unable to get computed pose at image timestamp of %u. (rawStates: have %zu from %u:%u) (visionStates: have %zu from %u:%u)",
                          image.GetTimestamp(),
                          _robot->GetStateHistory()->GetNumRawStates(),
                          _robot->GetStateHistory()->GetOldestTimeStamp(),
                          _robot->GetStateHistory()->GetNewestTimeStamp(),
                          _robot->GetStateHistory()->GetNumVisionStates(),
                          _robot->GetStateHistory()->GetOldestVisionOnlyTimeStamp(),
                          _robot->GetStateHistory()->GetNewestVisionOnlyTimeStamp());
      return lastResult;
    }

    // Get most recent pose data in history
    Anki::Cozmo::HistRobotState lastHistState;
    _robot->GetStateHistory()->GetLastStateWithFrameID(_robot->GetPoseFrameID(), lastHistState);

    {
      const Pose3d& cameraPose = _robot->GetHistoricalCameraPose(imageHistState, imageHistTimeStamp);
      Matrix_3x3f groundPlaneHomography;
      const bool groundPlaneVisible = LookupGroundPlaneHomography(imageHistState.GetHeadAngle_rad(),
                                                                  groundPlaneHomography);
      Lock();
      _nextPoseData.Set(imageHistTimeStamp,
                        imageHistState,
                        cameraPose,
                        groundPlaneVisible,
                        groundPlaneHomography,
                        _imuHistory);
      Unlock();
    }

    // Experimental:
    //UpdateOverheadMap(image, _nextPoseData);

    // Store image for calibration or factory test (*before* we swap image with _nextImg below!)
    // NOTE: This means we do decoding on main thread, but this is just for the factory
    //       test, so I'm not going to the trouble to store encoded images for calibration
    if (_storeNextImageForCalibration || _doFactoryDotTest)
    {
      // If we were moving too fast at the timestamp the image was taken then don't use it for
      // calibration or dot test purposes
      if(!WasRotatingTooFast(image.GetTimestamp(), DEG_TO_RAD(0.1), DEG_TO_RAD(0.1), 3))
      {
        Vision::Image imageGray = image.ToGray();

        if(_storeNextImageForCalibration)
        {
          _storeNextImageForCalibration = false;
          if (IsModeEnabled(VisionMode::ComputingCalibration)) {
            PRINT_NAMED_INFO("VisionComponent.SetNextImage.SkippingStoringImageBecauseAlreadyCalibrating", "");
          } else {
            Lock();
            Result result = _visionSystem->AddCalibrationImage(imageGray, _calibTargetROI);
            Unlock();

            if(RESULT_OK != result) {
              PRINT_NAMED_INFO("VisionComponent.SetNextImage.AddCalibrationImageFailed", "");
            }
          }
        } // if(_storeNextImageForCalibration)

        if(_doFactoryDotTest)
        {
          _doFactoryDotTest = false;

          ExternalInterface::RobotCompletedFactoryDotTest msg;
          Result dotResult = FindFactoryTestDotCentroids(imageGray, msg);
          if(RESULT_OK != dotResult) {
            PRINT_NAMED_WARNING("VisionComponent.SetNextImage.FactoryDotTestFailed", "");
          }
          _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));

        } // if(_doFactoryDotTest)

      }
      else {
        PRINT_NAMED_DEBUG("VisionComponent.SetNextImage.SkippingStorageForCalibrationBecauseMoving", "");
      }
    } // if (_storeNextImageForCalibration || _doFactoryDotTest)

    if(_paused)
    {
      _vizManager->SetText(VizManager::VISION_MODE, NamedColors::CYAN,
                           "Vision: <PAUSED>");
    }
    else
    {
      if(_isSynchronous)
      {
        UpdateVisionSystem(_nextPoseData, image);
      }
      else
      {
        ANKI_CPU_PROFILE("VC::SetNextImage.LockedSwap");
        Lock();
        
        const bool isDroppingFrame = !_nextImg.IsEmpty() || (kSimulateDroppedFrameFraction > 0.f &&
                                                             _robot->GetContext()->GetRandom()->RandDbl() < kSimulateDroppedFrameFraction);
        if(isDroppingFrame)
        {
          PRINT_CH_DEBUG("VisionComponent",
                         "VisionComponent.SetNextImage.DroppedFrame",
                         "Setting next image with t=%u, but existing next image from t=%u not yet processed (currently on t=%u).",
                         image.GetTimestamp(),
                         _nextImg.GetTimestamp(),
                         _currentImg.GetTimestamp());
        }
        _dropStats.Update(isDroppingFrame);
        
        // Make given image the new "next" image, to be processed once the vision thread is done with "current"
        std::swap(_nextImg, image);
        
        DEV_ASSERT(!_nextImg.IsEmpty(), "VisionComponent.SetNextImage.NextImageEmpty");
        
        Unlock();
      }
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
    for(f32 headAngle_rad = MIN_HEAD_ANGLE; headAngle_rad <= MAX_HEAD_ANGLE;
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

      if(_camera->IsWithinFieldOfView(imgQuad[Quad::CornerName::TopLeft]) ||
         _camera->IsWithinFieldOfView(imgQuad[Quad::CornerName::BottomLeft]))
      {
        // Only store this homography if the ROI still projects into the image
        _groundPlaneHomographyLUT[headAngle_rad] = H;
      } else {
        PRINT_CH_INFO("VisionComponent",
                      "VisionComponent.PopulateGroundPlaneHomographyLUT.MaxHeadAngleReached",
                      "Stopping at %.1fdeg", RAD_TO_DEG(headAngle_rad));
        break;
      }
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
      PRINT_NAMED_WARNING("VisionComponent.LookupGroundPlaneHomography.KeyNotFound",
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

    //      PRINT_NAMED_DEBUG("VisionComponent.LookupGroundPlaneHomography.HeadAngleDiff",
    //                        "Requested = %.2fdeg, Returned = %.2fdeg, Diff = %.2fdeg",
    //                        RAD_TO_DEG(atHeadAngle), RAD_TO_DEG(iter->first),
    //                        RAD_TO_DEG(std::abs(atHeadAngle - iter->first)));

    H = iter->second;
    return true;

  } // LookupGroundPlaneHomography()

  bool VisionComponent::IsDisplayingProcessedImagesOnly() const
  {
    return kDisplayProcessedImagesOnly;
  }

  void VisionComponent::UpdateVisionSystem(const VisionPoseData&   poseData,
                                           const Vision::ImageRGB& image)
  {
    ANKI_CPU_PROFILE("VC::UpdateVisionSystem");
    
    DEV_ASSERT((ImageEncoding::RawRGB == _currentImageFormat) || (ImageEncoding::YUV420sp == _currentImageFormat),
               "VisionComponent.UpdateVisionSystem.UnsupportedFormat");
    
    ANKI_VERIFY((_currentImageFormat == ImageEncoding::RawRGB && image.GetNumRows()==DEFAULT_CAMERA_RESOLUTION_HEIGHT) ||
                (_currentImageFormat == ImageEncoding::YUV420sp && image.GetNumRows()==CAMERA_SENSOR_RESOLUTION_HEIGHT),
                "VisionComponent.UpdateVisionSystem.FormatSizeMismatch",
                "Format: %s, NumRows:%d", ImageEncodingToString(_currentImageFormat), image.GetNumRows());
    
    const f32 sensorToFullDownsampleFactor = (_currentImageFormat == ImageEncoding::RawRGB ?
                                              1.f : // Sensor and Full are the same
                                              (f32)DEFAULT_CAMERA_RESOLUTION_HEIGHT / (f32)CAMERA_SENSOR_RESOLUTION_HEIGHT);
    const Vision::ResizeMethod resizeMethod = Vision::ResizeMethod::Linear;
    Result result = _visionSystem->Update(poseData, image, sensorToFullDownsampleFactor, resizeMethod);
    if(RESULT_OK != result) {
      PRINT_NAMED_WARNING("VisionComponent.UpdateVisionSystem.UpdateFailed", "");
    }

    _vizManager->SetText(VizManager::VISION_MODE, NamedColors::CYAN,
                         "Vision: %s", _visionSystem->GetCurrentModeName().c_str());
  }


  void VisionComponent::Processor()
  {
    PRINT_NAMED_INFO("VisionComponent.Processor",
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

    while (_running) {

      if(_paused) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kVision_MinSleepTime_ms));
        continue;
      }

      ANKI_CPU_TICK("VisionComponent", 100.0, Util::CpuProfiler::CpuProfilerLoggingTime(kVisionComponent_Logging));

      if (!_currentImg.IsEmpty())
      {
        ANKI_CPU_PROFILE("ProcessImage");
        // There is an image to be processed; do so now
        UpdateVisionSystem(_currentPoseData, _currentImg);

        Lock();
        // Clear it when done.
        ReleaseImage(_currentImg);
        Unlock();
      }

      if(!_nextImg.IsEmpty())
      {
        ANKI_CPU_PROFILE("SwapInNewImage");
        // We have an image waiting to be processed: swap it in (avoid copy)
        Lock();
        std::swap(_currentImg, _nextImg);
        std::swap(_currentPoseData, _nextPoseData);
        ReleaseImage(_nextImg);
        // Need to unlock "_nextImg"
        Unlock();
      }
      else
      {
        ANKI_CPU_PROFILE("SleepForNextImage");
        // Waiting on next image
        std::this_thread::sleep_for(std::chrono::milliseconds(kVision_MinSleepTime_ms));
      }

    } // while(_running)

    ANKI_CPU_REMOVE_THIS_THREAD();

    PRINT_CH_INFO("VisionComponent", "VisionComponent.Processor.TerminatedVisionSystemThread",
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
    filter.SetAllowedFamilies(std::set<ObjectFamily>{
      ObjectFamily::Block, ObjectFamily::Mat, ObjectFamily::LightCube, ObjectFamily::Charger
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
          PRINT_NAMED_WARNING("VisionComponent.VisualizeObservedMarkerIn3D.BadPose",
                              "Could not estimate pose of marker. Not visualizing.");
        } else {
          if(markerPose.GetWithRespectTo(marker.GetSeenBy().GetPose().FindRoot(), markerPose) == true) {
            _robot->GetContext()->GetVizManager()->DrawGenericQuad(quadID++, blockMarker->Get3dCorners(markerPose), NamedColors::OBSERVED_QUAD);
          } else {
            PRINT_NAMED_WARNING("VisionComponent.VisualizeObservedMarkerIn3D.MarkerOriginNotCameraOrigin",
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
        if(IsDisplayingProcessedImagesOnly() && (kImageCompressQuality > 0))
        {
          // The assumption is that all the chunks of this encoded image have already been sent to
          // the viz manager (e.g. forwarded by the robot message handler)
          _vizManager->DisplayCameraImage(result.timestamp);
        }

        using localHandlerType = Result(VisionComponent::*)(const VisionProcessingResult& procResult);
        auto tryAndReport = [this, &result, &anyFailures]( localHandlerType handler, VisionMode mode )
        {
          if (!result.modesProcessed.IsBitFlagSet(mode) && (mode != VisionMode::Count))
          {
            return;
          }

          // Call the passed in member handler to look at the result
          if (RESULT_OK != (this->*handler)(result))
          {
            PRINT_NAMED_ERROR("VisionComponent.UpdateAllResults.LocalHandlerFailed",
                              "%s Failed", EnumToString(mode));
            anyFailures = true;
          }
        };

        // NOTE: UpdateVisionMarkers will also update BlockWorld (which broadcasts
        //  object observations and should be done before sending RobotProcessedImage below!)
        tryAndReport(&VisionComponent::UpdateVisionMarkers,        VisionMode::DetectingMarkers);

        // NOTE: UpdateFaces will also update FaceWorld (which broadcasts face observations
        //  and should be done before sending RobotProcessedImage below!)
        tryAndReport(&VisionComponent::UpdateFaces,                VisionMode::DetectingFaces);

        // NOTE: UpdatePets will also update PetWorld (which broadcasts pet face observations
        //  and should be done before sending RobotProcessedImage below!)
        tryAndReport(&VisionComponent::UpdatePets,                 VisionMode::DetectingPets);

        tryAndReport(&VisionComponent::UpdateMotionCentroid,       VisionMode::DetectingMotion);
        tryAndReport(&VisionComponent::UpdateOverheadEdges,        VisionMode::DetectingOverheadEdges);
        tryAndReport(&VisionComponent::UpdateComputedCalibration,  VisionMode::ComputingCalibration);
        tryAndReport(&VisionComponent::UpdateImageQuality,         VisionMode::CheckingQuality);
        tryAndReport(&VisionComponent::UpdateWhiteBalance,         VisionMode::CheckingWhiteBalance);
        tryAndReport(&VisionComponent::UpdateLaserPoints,          VisionMode::DetectingLaserPoints);
        tryAndReport(&VisionComponent::UpdateSalientPoints,        VisionMode::Count); // Use Count here to always call UpdateSalientPoints
        tryAndReport(&VisionComponent::UpdateVisualObstacles,      VisionMode::DetectingVisualObstacles);
        tryAndReport(&VisionComponent::UpdatePhotoManager,         VisionMode::SavingImages);
        tryAndReport(&VisionComponent::UpdateDetectedIllumination, VisionMode::DetectingIllumination);
        // Display any debug images left by the vision system
        if(ANKI_DEV_CHEATS)
        {
          if(kImageCompressQuality > 0)
          {
            // Send any images in the debug image lists to Viz for display
            // Resize to fit display, but don't if it would make the image larger (to save bandwidth)
            const bool kOnlyResizeIfSmaller = true;
            const s32  kDisplayNumRows = 360; // TODO: Get these from VizManager perhaps?
            const s32  kDisplayNumCols = 640; //   "
            for(auto & debugGray : result.debugImages) {
              debugGray.second.SetTimestamp(result.timestamp); // Ensure debug image has timestamp matching result
              debugGray.second.ResizeKeepAspectRatio(kDisplayNumRows, kDisplayNumCols,
                                                     Vision::ResizeMethod::Linear, kOnlyResizeIfSmaller);
              CompressAndSendImage(debugGray.second, kImageCompressQuality, debugGray.first);
            }
            for(auto & debugRGB : result.debugImageRGBs) {
              debugRGB.second.SetTimestamp(result.timestamp); // Ensure debug image has timestamp matching result
              debugRGB.second.ResizeKeepAspectRatio(kDisplayNumRows, kDisplayNumCols,
                                                    Vision::ResizeMethod::Linear, kOnlyResizeIfSmaller);
              CompressAndSendImage(debugRGB.second, kImageCompressQuality, debugRGB.first);
            }
          }
          else if(kImageCompressQuality == -1)
          {
            // Display debug images locally
            for(auto & debugGray : result.debugImages) {
              debugGray.second.Display(debugGray.first.c_str());
            }
            for(auto & debugRGB : result.debugImageRGBs) {
              debugRGB.second.Display(debugRGB.first.c_str());
            }
          }
        }
        else if(!result.debugImages.empty() || !result.debugImageRGBs.empty())
        {
          // We do not expect to have debug images to draw without dev cheats enabled
          std::string grayStr;
          for(auto & debugGray : result.debugImages)
          {
            grayStr += debugGray.first;
            grayStr += " ";
          }

          std::string rgbStr;
          for(auto & debugRGB : result.debugImageRGBs)
          {
            rgbStr += debugRGB.first;
            rgbStr += " ";
          }

          PRINT_NAMED_ERROR("VisionComponent.UpdateAllResults.DebugImagesPresent",
                            "Gray:%s RGB:%s",
                            grayStr.empty() ? "<none>" : grayStr.c_str(),
                            rgbStr.empty() ? "<none>" : rgbStr.c_str());
        }

        // Store frame rate and last image processed time. Time should only move forward.
        // NOTE: Neural nets run asynchronously, so we ignore those results for this purpose.
        if(!result.modesProcessed.IsBitFlagSet(VisionMode::RunningNeuralNet))
        {
          DEV_ASSERT(result.timestamp >= _lastProcessedImageTimeStamp_ms, "VisionComponent.UpdateAllResults.BadTimeStamp");
          _processingPeriod_ms = result.timestamp - _lastProcessedImageTimeStamp_ms;
          _lastProcessedImageTimeStamp_ms = result.timestamp;
        }

        auto visionModesList = std::vector<VisionMode>();
        for (VisionMode mode = VisionMode::Idle; mode < VisionMode::Count; ++mode)
        {
          if (result.modesProcessed.IsBitFlagSet(mode))
          {
            visionModesList.push_back(mode);
          }
        }

        // Send the processed image message last
        {
          using namespace ExternalInterface;

          u8 imageMean = 0;
          if(result.modesProcessed.IsBitFlagSet(VisionMode::ComputingStatistics))
          {
            imageMean = result.imageMean;
          }

          _robot->Broadcast(MessageEngineToGame(RobotProcessedImage(result.timestamp,
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

  static Rectangle<f32> DisplayMirroredRectHelper(f32 x_topLeft, f32 y_topLeft, f32 width, f32 height)
  {
    // TODO: Figure out the original image resolution?
    const f32 heightScale = (f32)FACE_DISPLAY_HEIGHT / (f32)DEFAULT_CAMERA_RESOLUTION_HEIGHT;
    const f32 widthScale  = (f32)FACE_DISPLAY_WIDTH / (f32)DEFAULT_CAMERA_RESOLUTION_WIDTH;

    const f32 x_topRight = x_topLeft + width; // will become upper left after mirroring
    const Rectangle<f32> rect((f32)FACE_DISPLAY_WIDTH - widthScale*x_topRight, // mirror rectangle for display
                              y_topLeft * heightScale,
                              width * widthScale,
                              height * heightScale);

    return rect;
  }

  template<typename T>
  static Point<2,T> MirrorPointHelper(const Point<2,T>& pt)
  {
    // TODO: figure out original image resolution?
    constexpr f32 xmax = (f32)DEFAULT_CAMERA_RESOLUTION_WIDTH;
    constexpr f32 heightScale = (f32)FACE_DISPLAY_HEIGHT / (f32)DEFAULT_CAMERA_RESOLUTION_HEIGHT;
    constexpr f32 widthScale  = (f32)FACE_DISPLAY_WIDTH / (f32)DEFAULT_CAMERA_RESOLUTION_WIDTH;

    const Point<2,T> pt_mirror(widthScale*(xmax - pt.x()), pt.y()*heightScale);
    return pt_mirror;
  }

  static Quad2f DisplayMirroredQuadHelper(const Quad2f& quad)
  {
    // Mirror x coordinates, swap left/right points, and scale for each point in the quad:
    const Quad2f quad_mirrored(MirrorPointHelper(quad.GetTopRight()),
                               MirrorPointHelper(quad.GetBottomRight()),
                               MirrorPointHelper(quad.GetTopLeft()),
                               MirrorPointHelper(quad.GetBottomLeft()));

    return quad_mirrored;
  }

  template<typename T>
  static Polygon<2,T> DisplayMirroredPolyHelper(const Polygon<2,T>& poly)
  {
    Polygon<2,T> poly_mirrored;
    for(auto & pt : poly)
    {
      poly_mirrored.emplace_back(MirrorPointHelper(pt));
    }

    return poly_mirrored;
  }

  Result VisionComponent::UpdateVisionMarkers(const VisionProcessingResult& procResult)
  {
    Result lastResult = RESULT_OK;

    std::list<Vision::ObservedMarker> observedMarkers;

    if(!procResult.observedMarkers.empty())
    {
      // Get historical robot pose at this processing result's timestamp to get
      // head angle and to attach as parent of the camera pose.
      TimeStamp_t t;
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
        PRINT_CH_INFO("VisionComponent", "VisionComponent.UpdateVisionMarkers.HistoricalPoseNotFound",
                      "Time: %u, hist: %u to %u",
                      procResult.timestamp,
                      _robot->GetStateHistory()->GetOldestTimeStamp(),
                      _robot->GetStateHistory()->GetNewestTimeStamp());
        return RESULT_OK;
      }

      if(!_robot->IsPoseInWorldOrigin(histStatePtr->GetPose())) {
        PRINT_CH_INFO("VisionComponent", "VisionComponent.UpdateVisionMarkers.OldOrigin",
                      "Ignoring observed marker from origin %s (robot origin is %s)",
                      histStatePtr->GetPose().FindRoot().GetName().c_str(),
                      _robot->GetWorldOrigin().GetName().c_str());
        return RESULT_OK;
      }

      // If we get here, ComputeAndInsertPoseIntoHistory() should have succeeded
      // and this should be true
      assert(procResult.timestamp == t);

      // If we were moving too fast at timestamp t then don't queue this marker
      if(WasRotatingTooFast(t,
                            DEG_TO_RAD(kBodyTurnSpeedThreshBlock_degs),
                            DEG_TO_RAD(kHeadTurnSpeedThreshBlock_degs)))
      {
        return RESULT_OK;
      }

      Vision::Camera histCamera = _robot->GetHistoricalCamera(*histStatePtr, procResult.timestamp);

      // Note: we deliberately make a copy of the vision markers in observedMarkers
      // as we loop over them here, because procResult is const but we want to modify
      // each marker to hook up its camera to pose history
      for(auto visionMarker : procResult.observedMarkers)
      {
        if(visionMarker.GetTimeStamp() != procResult.timestamp)
        {
          PRINT_NAMED_ERROR("VisionComponent.UpdateVisionMarkers.MismatchedTimestamps",
                            "Marker t=%u vs. ProcResult t=%u",
                            visionMarker.GetTimeStamp(), procResult.timestamp);
          continue;
        }

        // Remove observed markers whose historical poses have become invalid.
        // This shouldn't happen! If it does, robotStateMsgs may be buffering up somewhere.
        // Increasing history time window would fix this, but it's not really a solution.
        if ((visionMarker.GetSeenBy().GetID() == GetCamera().GetID()) &&
            !_robot->GetStateHistory()->IsValidKey(histStateKey))
        {
          PRINT_NAMED_WARNING("VisionComponent.Update.InvalidHistStateKey", "key=%d", histStateKey);
          continue;
        }

        // Update the marker's camera to use a pose from pose history, and
        // create a new marker with the updated camera
        visionMarker.SetSeenBy(histCamera);

        const Quad2f& corners = visionMarker.GetImageCorners();
        const ColorRGBA& drawColor = (visionMarker.GetCode() == Vision::MARKER_UNKNOWN ?
                                      NamedColors::BLUE : NamedColors::RED);
        _vizManager->DrawCameraQuad(corners, drawColor, NamedColors::GREEN);

        if(Util::IsFltGTZero(kDisplayMarkerNamesScale))
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

        if(kDisplayDetectionsInMirrorMode)
        {
          const auto& quad = visionMarker.GetImageCorners();
          const auto& name = std::string(visionMarker.GetCodeName());
          std::function<void (Vision::ImageRGB&)> modFcn = [quad,drawColor,name](Vision::ImageRGB& img)
          {
            //const Rectangle<f32> rect(quad);
            //img.DrawRect(DisplayMirroredRectHelper(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight()), drawColor, 3);
            img.DrawQuad(DisplayMirroredQuadHelper(quad), drawColor, 3);
            if(Util::IsFltGTZero(kDisplayMarkerNamesScale))
            {
              img.DrawText({1., img.GetNumRows()-1}, name.substr(strlen("MARKER_"),std::string::npos),
                           drawColor, kDisplayMarkerNamesScale);
            }
          };

          AddDrawScreenModifier(modFcn);
        }

        observedMarkers.push_back(std::move(visionMarker));
      }
    } // if(!procResult.observedMarkers.empty())

    lastResult = _robot->GetBlockWorld().UpdateObservedMarkers(observedMarkers);
    if(RESULT_OK != lastResult)
    {
      PRINT_NAMED_WARNING("VisionComponent.UpdateVisionResults.BlockWorldUpdateFailed", "");
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
       PRINT_NAMED_INFO("VisionComponent.Update",
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
        PRINT_NAMED_DEBUG("VisionComponent.UpdateFaces.ReachedEnrollmentCount",
                          "Count=%d", faceDetection.GetNumEnrollments());

        _robot->GetFaceWorld().SetFaceEnrollmentComplete(true);
      }

      if(kDisplayDetectionsInMirrorMode)
      {
        const auto& rect = faceDetection.GetRect();
        const auto& name = faceDetection.GetName();

        std::function<void (Vision::ImageRGB&)> modFcn = [rect, name](Vision::ImageRGB& img)
        {
          img.DrawRect(DisplayMirroredRectHelper(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight()),
                       NamedColors::YELLOW, 3);
          if(!name.empty())
          {
            img.DrawText({1.f, img.GetNumRows()-1}, name, NamedColors::YELLOW, 0.6f, true);
          }
        };

        AddDrawScreenModifier(modFcn);
      }
    }

    if(kDisplayEyeContactInMirrorMode)
    {
      const u32 maxTimeSinceSeenFaceToLook_ms = ConditionEyeContact::GetMaxTimeSinceTrackedFaceUpdated_ms();
      const bool making_eye_contact = _robot->GetFaceWorld().IsMakingEyeContact(maxTimeSinceSeenFaceToLook_ms);
      if(making_eye_contact)
      {
        std::function<void (Vision::ImageRGB&)> modFcn = [](Vision::ImageRGB& img)
        {
          // Put eye contact indicator right in the middle
          const f32 x = .5f * (f32)DEFAULT_CAMERA_RESOLUTION_WIDTH;
          const f32 y = .5f * (f32)DEFAULT_CAMERA_RESOLUTION_HEIGHT;
          const f32 width = .2f * (f32)DEFAULT_CAMERA_RESOLUTION_WIDTH;
          const f32 height = .2f * (f32)DEFAULT_CAMERA_RESOLUTION_HEIGHT;
          img.DrawFilledRect(DisplayMirroredRectHelper(x, y, width, height), NamedColors::YELLOW);
        };

        AddDrawScreenModifier(modFcn);
      }
    }

    lastResult = _robot->GetFaceWorld().Update(procResult.faces);
    if(RESULT_OK != lastResult)
    {
      PRINT_NAMED_WARNING("VisionComponent.UpdateFaces.FaceWorldUpdateFailed", "");
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
    TimeStamp_t currentTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

    auto iter = _salientPointsToDraw.begin();
    while(iter != _salientPointsToDraw.end() && iter->first < currentTime_ms - kKeepDrawingSalientPointsFor_ms)
    {
      iter = _salientPointsToDraw.erase(iter);
    }

    if(procResult.modesProcessed.IsBitFlagSet(VisionMode::RunningNeuralNet))
    {
      for(auto const& detectedObject : procResult.salientPoints)
      {
        // TODO: Notify behaviors somehow
        _salientPointsToDraw.push_back({currentTime_ms, detectedObject});
      }
    }

    s32 colorIndex = 0;
    for(auto const& salientPointToDraw : _salientPointsToDraw)
    {
      const auto& object = salientPointToDraw.second;

      const Poly2f poly(object.shape);
      const ColorRGBA color = (object.description.empty() ? NamedColors::RED : ColorRGBA::CreateFromColorIndex(colorIndex++));
      _vizManager->DrawCameraPoly(poly, color);
      const std::string caption(object.description + "[" + std::to_string((s32)std::round(100.f*object.score))
                                + "] t:" + std::to_string(object.timestamp));
      _vizManager->DrawCameraText(Point2f(object.x_img, object.y_img), caption, color);

      if(kDisplayDetectionsInMirrorMode)
      {
        const Point2f centroid(object.x_img, object.y_img);
        PRINT_NAMED_INFO("VisionComponent.UpdateSalientPoints.MirrorMode",
                         "Drawing %s poly with %d points. Centroid: (%.2f,%.2f)",
                         EnumToString(object.salientType), (int)poly.size(), centroid.x(), centroid.y());
        std::string str(object.description);
        if(!str.empty())
        {
          str += ":" + std::to_string((s32)std::round(object.score*100.f));
        }
        std::function<void (Vision::ImageRGB&)> modFcn = [str,centroid,poly,color](Vision::ImageRGB& img)
        {
          const Point2f mirroredCentroid = MirrorPointHelper(centroid);
          if(!str.empty())
          {
            const bool kDropShadow = true;
            const bool kCentered = true;
            img.DrawText(mirroredCentroid, str, NamedColors::YELLOW, 0.6f, kDropShadow, 1, kCentered);
          }
          img.DrawFilledCircle(mirroredCentroid, color, 3);
          img.DrawPoly(DisplayMirroredPolyHelper(poly), color, 2);
        };

        AddDrawScreenModifier(modFcn);
      }
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

  Result VisionComponent::UpdateImageQuality(const VisionProcessingResult& procResult)
  {
    if(!_robot->IsPhysical() || procResult.imageQuality == ImageQuality::Unchecked)
    {
      // Nothing to do
      return RESULT_OK;
    }

    // If auto exposure is not enabled don't set camera settings
    if(_enableAutoExposure)
    {
      SetExposureSettings(procResult.cameraParams.exposureTime_ms,
                          procResult.cameraParams.gain);
    }

    if(procResult.imageQuality != _lastImageQuality || _currentQualityBeginTime_ms==0)
    {
      // Just switched image qualities
      _currentQualityBeginTime_ms = procResult.timestamp;
      _waitForNextAlert_ms = kImageQualityAlertDuration_ms; // for first alert, use "duration" time
      _lastBroadcastImageQuality = ImageQuality::Unchecked; // i.e. reset so we definitely broadcast again
    }
    else if(_lastBroadcastImageQuality != ImageQuality::Good) // Don't keep broadcasting once in Good state
    {
      const TimeStamp_t timeWithThisQuality_ms = procResult.timestamp - _currentQualityBeginTime_ms;

      if(timeWithThisQuality_ms > _waitForNextAlert_ms)
      {
        // If we get here, we've been in a new image quality for long enough to trust it and broadcast.
        EngineErrorCode errorCode = EngineErrorCode::Count;

        switch(_lastImageQuality)
        {
          case ImageQuality::Unchecked:
            // can't get here due to IF above (but need case to prevent compiler error without default)
            assert(false);
            break;

          case ImageQuality::Good:
            errorCode = EngineErrorCode::ImageQualityGood;
            break;

          case ImageQuality::TooDark:
            errorCode = EngineErrorCode::ImageQualityTooDark;
            break;

          case ImageQuality::TooBright:
            errorCode = EngineErrorCode::ImageQualityTooBright;
            break;
        }

        PRINT_NAMED_INFO("robot.vision.image_quality", "%s", EnumToString(errorCode));

        PRINT_CH_DEBUG("VisionComponent",
                       "VisionComponent.UpdateImageQuality.BroadcastingImageQualityChange",
                       "Seeing %s for more than %u > %ums, broadcasting %s",
                       EnumToString(procResult.imageQuality), timeWithThisQuality_ms,
                       _waitForNextAlert_ms, EnumToString(errorCode));

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

  Result VisionComponent::UpdateWhiteBalance(const VisionProcessingResult& procResult)
  {
    if(_robot->IsPhysical() && _enableWhiteBalance)
    {
      SetWhiteBalanceSettings(procResult.cameraParams.whiteBalanceGainR,
                              procResult.cameraParams.whiteBalanceGainG,
                              procResult.cameraParams.whiteBalanceGainB);
    }

    return RESULT_OK;
  }

  Result VisionComponent::UpdatePhotoManager(const VisionProcessingResult& procResult)
  {
    _robot->GetPhotographyManager().SetLastPhotoTimeStamp(procResult.timestamp);
    return RESULT_OK;
  }
  
  Result VisionComponent::UpdateDetectedIllumination(const VisionProcessingResult& procResult)
  {
    ExternalInterface::RobotObservedIllumination msg( procResult.illumination );
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
    return RESULT_OK;
  }

  bool VisionComponent::WasHeadRotatingTooFast(TimeStamp_t t,
                                               const f32 headTurnSpeedLimit_radPerSec,
                                               const int numImuDataToLookBack) const
  {
    // Check to see if the robot's body or head are
    // moving too fast to queue this marker
    if(!_visionWhileMovingFastEnabled)
    {
      if(numImuDataToLookBack > 0)
      {
        return (_imuHistory.IsImuDataBeforeTimeGreaterThan(t,
                                                           numImuDataToLookBack,
                                                           0, headTurnSpeedLimit_radPerSec, 0));
      }

      ImuDataHistory::ImuData prev, next;
      if(!_imuHistory.GetImuDataBeforeAndAfter(t, prev, next))
      {
        PRINT_CH_INFO("VisionComponent",
                      "VisionComponent.VisionComponent.WasHeadRotatingTooFast.NoIMUData",
                      "Could not get next/previous imu data for timestamp %u", t);
        return true;
      }

      if(ABS(prev.rateY) > headTurnSpeedLimit_radPerSec ||
         ABS(next.rateY) > headTurnSpeedLimit_radPerSec)
      {
        return true;
      }
    }
    return false;
  }

  bool VisionComponent::WasBodyRotatingTooFast(TimeStamp_t t,
                                               const f32 bodyTurnSpeedLimit_radPerSec,
                                               const int numImuDataToLookBack) const
  {
    // Check to see if the robot's body or head are
    // moving too fast to queue this marker
    if(!_visionWhileMovingFastEnabled)
    {
      if(numImuDataToLookBack > 0)
      {
        return (_imuHistory.IsImuDataBeforeTimeGreaterThan(t,
                                                           numImuDataToLookBack,
                                                           0, 0, bodyTurnSpeedLimit_radPerSec));
      }

      ImuDataHistory::ImuData prev, next;
      if(!_imuHistory.GetImuDataBeforeAndAfter(t, prev, next))
      {
        PRINT_CH_INFO("VisionComponent", "VisionComponent.VisionComponent.WasBodyRotatingTooFast",
                      "Could not get next/previous imu data for timestamp %u", t);
        return true;
      }

      if(ABS(prev.rateZ) > bodyTurnSpeedLimit_radPerSec ||
         ABS(next.rateZ) > bodyTurnSpeedLimit_radPerSec)
      {
        return true;
      }
    }
    return false;
  }

  bool VisionComponent::WasRotatingTooFast(TimeStamp_t t,
                                           const f32 bodyTurnSpeedLimit_radPerSec,
                                           const f32 headTurnSpeedLimit_radPerSec,
                                           const int numImuDataToLookBack) const
  {
    return (WasHeadRotatingTooFast(t, headTurnSpeedLimit_radPerSec, numImuDataToLookBack) ||
            WasBodyRotatingTooFast(t, bodyTurnSpeedLimit_radPerSec, numImuDataToLookBack));
  }

  void VisionComponent::AddLiftOccluder(const TimeStamp_t t_request)
  {
    // TODO: More precise check for position of lift in FOV given head angle
    HistRobotState histState;
    TimeStamp_t t;
    Result result = _robot->GetStateHistory()->GetRawStateAt(t_request, t, histState, false);

    if(RESULT_FAIL_ORIGIN_MISMATCH == result)
    {
      // Not a warning, this can legitimately happen
      PRINT_CH_INFO("VisionComponent",
                    "VisionComponent.VisionComponent.AddLiftOccluder.StateHistoryOriginMismatch",
                    "Could not get pose at t=%u due to origin change. Skipping.", t_request);
      return;
    }
    else if(RESULT_OK != result)
    {
      PRINT_NAMED_WARNING("VisionComponent.WasLiftInFOV.StateHistoryFailure",
                          "Could not get raw pose at t=%u", t_request);
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

  template<class PixelType>
  Result VisionComponent::CompressAndSendImage(const Vision::ImageBase<PixelType>& img, s32 quality, const std::string& identifier)
  {
    if(quality == 0 || !_robot->GetContext()->GetVizManager()->IsConnected())
    {
      // Don't send anything
      return RESULT_OK;
    }

    if(!_robot->HasExternalInterface()) {
      PRINT_NAMED_ERROR("VisionComponent.CompressAndSendImage.NoExternalInterface", "");
      return RESULT_FAIL;
    }

    static cv::Mat_<PixelType> sMat(img.GetNumRows(), img.GetNumCols());
    if(sMat.rows != img.GetNumRows() || sMat.cols != img.GetNumCols())
    {
      sMat.release();
      sMat.create(img.GetNumRows(), img.GetNumCols());
    }

    ImageChunk m;
    m.height = img.GetNumRows();
    m.width  = img.GetNumCols();

    const std::vector<int> compressionParams = {
      CV_IMWRITE_JPEG_QUALITY, quality
    };

    // Convert to BGR so that imencode works
    img.ConvertToShowableFormat(sMat);

    std::vector<u8> compressedBuffer;
    cv::imencode(".jpg",  sMat, compressedBuffer, compressionParams);

    const u32 kMaxChunkSize = static_cast<u32>(ImageConstants::IMAGE_CHUNK_SIZE);
    u32 bytesRemainingToSend = static_cast<u32>(compressedBuffer.size());

    // Use the identifier value as the display index
    m.displayIndex = 0;
    auto displayIndexIter = _vizDisplayIndexMap.find(identifier);
    if(displayIndexIter == _vizDisplayIndexMap.end())
    {
      // New identifier
      m.displayIndex = Util::numeric_cast<s32>(_vizDisplayIndexMap.size());
      _vizDisplayIndexMap.emplace(identifier, m.displayIndex); // NOTE: this will increase size() for next time
    }
    else
    {
      m.displayIndex = displayIndexIter->second;
    }

    static u32 imgID = 0;
    m.imageId = ++imgID;

    m.frameTimeStamp = img.GetTimestamp();
    m.chunkId = 0;
    m.imageChunkCount = ceilf((f32)bytesRemainingToSend / kMaxChunkSize);
    if(img.GetNumChannels() == 1) {
      m.imageEncoding = ImageEncoding::JPEGGray;
    } else {
      m.imageEncoding = ImageEncoding::JPEGColor;
    }

    while (bytesRemainingToSend > 0) {
      u32 chunkSize = std::min(bytesRemainingToSend, kMaxChunkSize);

      auto startIt = compressedBuffer.begin() + (compressedBuffer.size() - bytesRemainingToSend);
      auto endIt = startIt + chunkSize;
      m.data = std::vector<u8>(startIt, endIt);

      if (_robot->GetImageSendMode() != ImageSendMode::Off) {
        _robot->Broadcast(ExternalInterface::MessageEngineToGame(ImageChunk(m)));
      }

      // Forward the image chunks to Viz as well (Note that this does nothing if
      // sending images is disabled in VizManager)
      _robot->GetContext()->GetVizManager()->SendImageChunk(_robot->GetID(), m);

      bytesRemainingToSend -= chunkSize;
      ++m.chunkId;
    }

    return RESULT_OK;

  } // CompressAndSendImage()

  // Explicit instantiation for grayscale and RGB
  template Result VisionComponent::CompressAndSendImage<u8>(const Vision::ImageBase<u8>& img,
                                                            s32 quality, const std::string& identifier);

  template Result VisionComponent::CompressAndSendImage<Vision::PixelRGB>(const Vision::ImageBase<Vision::PixelRGB>& img,
                                                                          s32 quality, const std::string& identifier);

  Result VisionComponent::ClearCalibrationImages()
  {
    if(nullptr == _visionSystem || !_visionSystem->IsInitialized())
    {
      PRINT_NAMED_ERROR("VisionComponent.ClearCalibrationImages.VisionSystemNotReady", "");
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
      PRINT_NAMED_WARNING("VisionComponent.WriteCalibrationPoseToRobot.InvalidPoseIndex",
                          "Requested %zu, only %zu available", whichPose, calibPoses.size());
    } else {
      auto & calibImages = _visionSystem->GetCalibrationImages();
      DEV_ASSERT_MSG(calibImages.size() >= calibPoses.size(),
                     "VisionComponent.WriteCalibrationPoseToRobot.SizeMismatch",
                     "Expecting at least %zu calibration images, got %zu",
                     calibPoses.size(), calibImages.size());

      if(!calibImages[whichPose].dotsFound) {
        PRINT_NAMED_INFO("VisionComponent.WriteCalibrationPoseToRobot.PoseNotComputed",
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
      PRINT_NAMED_WARNING("VisionComponent.FindFactorTestDotCentroids.ComputePoseFailed", "");
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
        PRINT_NAMED_WARNING("VisionComponent.FindFactoryTestDotCentroids.NotComponentLargeEnough",
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
        PRINT_NAMED_WARNING("VisionComponent.FindFactoryTestDotCentroids.ComputePoseFailed", "");
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

  inline static size_t GetPaddedNumBytes(size_t numBytes)
  {
    // Pad to a multiple of 4 for NVStorage
    const size_t paddedNumBytes = (numBytes + 3) & ~0x03;
    DEV_ASSERT(paddedNumBytes % 4 == 0, "EnrolledFaceEntry.Serialize.PaddedSizeNotMultipleOf4");

    return paddedNumBytes;
  }

  Result VisionComponent::SaveFaceAlbumToRobot()
  {
    return SaveFaceAlbumToRobot(nullptr, nullptr);
  }

  Result VisionComponent::SaveFaceAlbumToRobot(std::function<void(NVStorage::NVResult)> albumCallback,
                                               std::function<void(NVStorage::NVResult)> enrollCallback)
  {
    Result lastResult = RESULT_OK;

    _albumData.clear();
    _enrollData.clear();

    // Get album data from vision system
    this->Lock();
    lastResult = _visionSystem->GetSerializedFaceData(_albumData, _enrollData);
    this->Unlock();

    if(lastResult != RESULT_OK)
    {
      PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbumToRobot.GetSerializedFaceDataFailed",
                          "GetSerializedFaceData failed");
      return lastResult;
    }

    const u32 maxAlbumSize = _robot->GetNVStorageComponent().GetMaxSizeForEntryTag(NVStorage::NVEntryTag::NVEntry_FaceAlbumData);
    if(_albumData.size() >= maxAlbumSize)
    {
      PRINT_NAMED_ERROR("VisionComponent.SaveFaceAlbumToRobot.AlbumDataTooLarge",
                        "Album data is %zu max size is %u",
                        _albumData.size(),
                        maxAlbumSize);
      return RESULT_FAIL_INVALID_SIZE;
    }

    const u32 maxEnrollSize = _robot->GetNVStorageComponent().GetMaxSizeForEntryTag(NVStorage::NVEntryTag::NVEntry_FaceEnrollData);
    if(_enrollData.size() >= maxEnrollSize)
    {
      PRINT_NAMED_ERROR("VisionComponent.SaveFaceAlbumToRobot.EnrollDataTooLarge",
                        "Enroll data is %zu max size is %u",
                        _enrollData.size(),
                        maxEnrollSize);
      return RESULT_FAIL_INVALID_SIZE;
    }

    // Callback to display result when NVStorage.Write finishes
    NVStorageComponent::NVStorageWriteEraseCallback saveAlbumCallback = [this,albumCallback](NVStorage::NVResult result) {
      if(result == NVStorage::NVResult::NV_OKAY) {
        PRINT_NAMED_INFO("VisionComponent.SaveFaceAlbumToRobot.AlbumSuccess",
                         "Successfully completed saving album data to robot");
      } else {
        PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbumToRobot.AlbumFailure",
                            "Failed saving album data to robot: %s",
                            EnumToString(result));
        _robot->BroadcastEngineErrorCode(EngineErrorCode::WriteFacesToRobot);
      }

      if(nullptr != albumCallback) {
        albumCallback(result);
      }
    };
    // Callback to display result when NVStorage.Write finishes
    NVStorageComponent::NVStorageWriteEraseCallback saveEnrollCallback = [this,enrollCallback](NVStorage::NVResult result) {
      if(result == NVStorage::NVResult::NV_OKAY) {
        PRINT_NAMED_INFO("VisionComponent.SaveFaceAlbumToRobot.EnrollSuccess",
                         "Successfully completed saving enroll data to robot");
      } else {
        PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbumToRobot.EnrollFailure",
                            "Failed saving enroll data to robot: %s",
                            EnumToString(result));
        _robot->BroadcastEngineErrorCode(EngineErrorCode::WriteFacesToRobot);
      }

      if(nullptr != enrollCallback) {
        enrollCallback(result);
      }
    };


    if(_albumData.empty() && _enrollData.empty()) {
      PRINT_NAMED_INFO("VisionComponent.SaveFaceAlbumToRobot.EmptyAlbumData", "Will erase robot album data");
      // Special case: no face data, so erase what's on the robot so it matches
      bool sendSucceeded = _robot->GetNVStorageComponent().Erase(NVStorage::NVEntryTag::NVEntry_FaceAlbumData, saveAlbumCallback);
      if(!sendSucceeded) {
        PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbumToRobot.SendEraseAlbumDataFail", "");
        lastResult = RESULT_FAIL;
      } else {
        sendSucceeded = _robot->GetNVStorageComponent().Erase(NVStorage::NVEntryTag::NVEntry_FaceEnrollData, saveEnrollCallback);
        if(!sendSucceeded) {
          PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbumToRobot.SendEraseEnrollDataFail", "");
        }
      }
      return lastResult;
    }

    // Pad to a multiple of 4 for NVStorage
    _albumData.resize(GetPaddedNumBytes(_albumData.size()));
    _enrollData.resize(GetPaddedNumBytes(_enrollData.size()));

    // If one of the data is not empty, neither should be (we can't have album data with
    // no enroll data or vice versa)
    DEV_ASSERT(!_albumData.empty() && !_enrollData.empty(), "VisionComponent.SaveFaceAlbumToRobot.BadAlbumOrEnrollData");

    // Use NVStorage to save it
    bool sendSucceeded = _robot->GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_FaceAlbumData,
                                                              _albumData.data(), _albumData.size(), saveAlbumCallback);
    if(!sendSucceeded) {
      PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbumToRobot.SendWriteAlbumDataFail", "");
      lastResult = RESULT_FAIL;
    } else {
      sendSucceeded = _robot->GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_FaceEnrollData,
                                                         _enrollData.data(), _enrollData.size(), saveEnrollCallback);
      if(!sendSucceeded) {
        PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbumToRobot.SendWriteEnrollDataFail", "");
        lastResult = RESULT_FAIL;
      }
    }

    PRINT_NAMED_INFO("VisionComponent.SaveFaceAlbumToRobot.Initiated",
                     "Initiated save of %zu-byte album data and %zu-byte enroll data to NVStorage",
                     _albumData.size(), _enrollData.size());

    PRINT_NAMED_INFO("robot.vision.save_face_album_data_size_bytes", "%zu", _albumData.size());
    PRINT_NAMED_INFO("robot.vision.save_face_enroll_data_size_bytes", "%zu", _enrollData.size());

    return lastResult;
  } // SaveFaceAlbumToRobot()

  Result VisionComponent::LoadFaceAlbumFromRobot()
  {
    Result lastResult = RESULT_OK;

    DEV_ASSERT(_visionSystem != nullptr, "VisionComponent.LoadFaceAlbumFromRobot.VisionSystemNotReady");

    NVStorageComponent::NVStorageReadCallback readCallback =
    [this](u8* data, size_t size, NVStorage::NVResult result)
    {
      if(result == NVStorage::NVResult::NV_OKAY)
      {
        // Read completed: try to use the data to update the face album/enroll data
        Lock();
        std::list<Vision::LoadedKnownFace> loadedFaces;
        Result setResult = _visionSystem->SetSerializedFaceData(_albumData, _enrollData, loadedFaces);
        Unlock();

        if(RESULT_OK == setResult) {
          PRINT_NAMED_INFO("VisionComponent.LoadFaceAlbumFromRobot.Success",
                           "Finished setting %zu-byte album data and %zu-byte enroll data",
                           _albumData.size(), _enrollData.size());

          PRINT_CH_INFO("VisionComponent", "VisionComponent.LoadFaceAlbumFromRobot.Success", "Number of Loaded Faces: %zu",
                        loadedFaces.size());

          BroadcastLoadedNamesAndIDs(loadedFaces);

        } else {
          PRINT_NAMED_WARNING("VisionComponent.LoadFaceAlbumFromRobot.Failure",
                              "Failed setting %zu-byte album data and %zu-byte enroll data",
                              _albumData.size(), _enrollData.size());
        }
      } else if (result == NVStorage::NVResult::NV_NOT_FOUND) {
        PRINT_NAMED_INFO("VisionComponent.LoadFaceAlbumFromRobot.ReadFaceEnrollDataNotFound", "");
      } else {
        PRINT_NAMED_WARNING("VisionComponent.LoadFaceAlbumFromRobot.ReadFaceEnrollDataFail",
                            "NVResult = %s", EnumToString(result));
      }

      _albumData.clear();
      _enrollData.clear();

    }; // ReadCallback

    _albumData.clear();
    _enrollData.clear();

    // NOTE: We don't run the callback until both data items have been read
    bool sendSucceeded = _robot->GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_FaceAlbumData,
                                                             {}, &_albumData, false);

    if(!sendSucceeded) {
      PRINT_NAMED_WARNING("VisionComponent.LoadFaceAlbumFromRobot.ReadFaceAlbumDataFail", "");
      lastResult = RESULT_FAIL;
    } else {
      sendSucceeded = _robot->GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_FaceEnrollData,
                                                          readCallback, &_enrollData, false);
      if(!sendSucceeded) {
        PRINT_NAMED_WARNING("VisionComponent.LoadFaceAlbumFromRobot.ReadFaceEnrollDataFail", "");
        lastResult = RESULT_FAIL;
      }
    }

    PRINT_NAMED_INFO("VisionComponent.LoadFaceAlbumToRobot.Initiated",
                     "Initiated load of album and enroll data from NVStorage");

    return lastResult;
  } // LoadFaceAlbumFromRobot()


  Result VisionComponent::SaveFaceAlbumToFile(const std::string& path)
  {
    Lock();
    Result result = _visionSystem->SaveFaceAlbum(path);
    Unlock();

    if(RESULT_OK != result) {
      PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbum.SaveToFileFailed",
                          "AlbumFile: %s", path.c_str());
    }
    return result;
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

  Result VisionComponent::LoadFaceAlbumFromFile(const std::string& path, std::list<Vision::LoadedKnownFace>& loadedFaces)
  {
    Result result = _visionSystem->LoadFaceAlbum(path, loadedFaces);

    if(RESULT_OK != result) {
      PRINT_NAMED_WARNING("VisionComponent.LoadFaceAlbum.LoadFromFileFailed",
                          "AlbumFile: %s", path.c_str());
    } else {
      result = SaveFaceAlbumToRobot();
    }

    return result;
  }


  void VisionComponent::AssignNameToFace(Vision::FaceID_t faceID, const std::string& name, Vision::FaceID_t mergeWithID)
  {
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

  void VisionComponent::SetFaceEnrollmentMode(Vision::FaceEnrollmentPose pose,
 																						  Vision::FaceID_t forFaceID,
																						  s32 numEnrollments)

  {
    _visionSystem->SetFaceEnrollmentMode(pose, forFaceID, numEnrollments);
  }

  Result VisionComponent::EraseFace(Vision::FaceID_t faceID)
  {
    Lock();
    Result result = _visionSystem->EraseFace(faceID);
    int numRemaining = static_cast<int>(_visionSystem->GetEnrolledNames().size());
    Unlock();
    if(RESULT_OK == result) {
      // Update robot
      SaveFaceAlbumToRobot();
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
    Lock();
    _visionSystem->EraseAllFaces();
    Unlock();
    SaveFaceAlbumToRobot();
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
    Vision::RobotRenamedEnrolledFace renamedFace;
    Lock();
    Result result = _visionSystem->RenameFace(faceID, oldName, newName, renamedFace);
    int numTotal = static_cast<int>(_visionSystem->GetEnrolledNames().size());
    Unlock();

    if(RESULT_OK == result)
    {
      SaveFaceAlbumToRobot();
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
    Lock();
    auto namePairList = _visionSystem->GetEnrolledNames();
    Unlock();

    // Enrolled names are guaranteed to be lowercase
    // ** The UserName class will handle matching for us when it's implemented **
    
    auto tolower = [](const char c) { return std::tolower(c); };
    std::string lcName{name};
    std::transform(lcName.begin(), lcName.end(), lcName.begin(), tolower);
    for( const auto& nameEntry : namePairList ) {
      if( nameEntry.name == lcName ) {
        return true;
      }
    }
    return false;
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

      PRINT_CH_INFO("VisionComponent", "VisionComponent.BroadcastLoadedNamesAndIDs", "broadcasting loaded face id: %d",
                    loadedFace.faceID);

      _robot->Broadcast(MessageEngineToGame( Vision::LoadedKnownFace(loadedFace) ));
    }

    _robot->GetFaceWorld().InitLoadedKnownFaces(loadedFaces);
  }

  void VisionComponent::FakeImageProcessed(TimeStamp_t t)
  {
    _lastProcessedImageTimeStamp_ms = t;
  }

  CameraParams VisionComponent::GetCurrentCameraParams() const
  {
    return _visionSystem->GetCurrentCameraParams();
  }

  void VisionComponent::EnableAutoExposure(bool enable)
  {
    _enableAutoExposure = enable;
    _visionSystem->SetNextMode(VisionMode::CheckingQuality, enable);
  }

  void VisionComponent::EnableWhiteBalance(bool enable)
  {
    _enableWhiteBalance = enable;
    _visionSystem->SetNextMode(VisionMode::CheckingWhiteBalance, enable);
  }

  void VisionComponent::SetAndDisableAutoExposure(u16 exposure_ms, f32 gain)
  {
    SetExposureSettings(exposure_ms, gain);
    EnableAutoExposure(false);
  }

  void VisionComponent::SetAndDisableWhiteBalance(f32 gainR, f32 gainG, f32 gainB)
  {
    SetWhiteBalanceSettings(gainR, gainG, gainB);
    EnableWhiteBalance(false);
  }

  void VisionComponent::SetExposureSettings(const s32 exposure_ms, const f32 gain)
  {
    if(!_visionSystem->IsExposureValid(exposure_ms) || !_visionSystem->IsGainValid(gain))
    {
      return;
    }

    const u16 exposure_ms_u16 = Util::numeric_cast<u16>(exposure_ms);

    PRINT_CH_INFO("VisionComponent",
                  "VisionComponent.SetCameraSettings",
                  "Exp:%ums Gain:%f",
                  exposure_ms,
                  gain);

    _visionSystem->SetNextCameraExposure(exposure_ms, gain);

    _vizManager->SendCameraParams(_visionSystem->GetCurrentCameraParams());

    auto cameraService = CameraService::getInstance();
    cameraService->CameraSetParameters(exposure_ms_u16, gain);

    _robot->Broadcast(ExternalInterface::MessageEngineToGame(
                        ExternalInterface::CurrentCameraParams(gain, exposure_ms_u16, _enableAutoExposure) ));
  }

  void VisionComponent::SetWhiteBalanceSettings(f32 gainR, f32 gainG, f32 gainB)
  {
    if(!_visionSystem->IsGainValid(gainR) ||
       !_visionSystem->IsGainValid(gainG) ||
       !_visionSystem->IsGainValid(gainB))
    {
      PRINT_PERIODIC_CH_INFO(100, "VisionComponent", "VisionComponent.SetWhiteBalance.InvalidGains",
                             "gainR=%f gainG=%f gainB=%f", gainR, gainG, gainB);
      return;
    }

    PRINT_CH_INFO("VisionComponent",
                  "VisionComponent.SetWhiteBalanceSettings",
                  "GainR:%f GainG:%f GainB:%f",
                  gainR, gainG, gainB);

    _visionSystem->SetNextCameraWhiteBalance(gainR, gainG, gainB);

    auto cameraService = CameraService::getInstance();
    cameraService->CameraSetWhiteBalanceParameters(gainR, gainG, gainB);

    _vizManager->SendCameraParams(_visionSystem->GetCurrentCameraParams());

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

  bool VisionComponent::ReleaseImage(Vision::ImageRGB& image)
  {
    bool r = true;
    if (!image.IsEmpty()) {
      auto cameraService = CameraService::getInstance();
      r = cameraService->CameraReleaseFrame(image.GetImageId());
      if (r) {
        image.Clear();
        image.SetImageId(0);
      }
    }
    return r;
  }

  bool VisionComponent::CaptureImage(Vision::ImageRGB& image_out)
  {
    // Wait until the current async conversion is finished before getting another
    // frame
    if(_cvtYUV2RGBFuture.valid())
    {
      if(_cvtYUV2RGBFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
      {
        image_out = _cvtYUV2RGBFuture.get();
        return true;
      }

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

    // Get image buffer
    u8* buffer = nullptr;
    u32 imageId = 0;
    TimeStamp_t imageCaptureSystemTimestamp_ms = 0;
    ImageEncoding format;

    const bool gotImage = cameraService->CameraGetFrame(buffer, imageId, imageCaptureSystemTimestamp_ms, format);
    if(gotImage)
    {
      if(format == ImageEncoding::YUV420sp)
      {
        // Create async future to do the YUV to RGB conversion since it is slow
        _cvtYUV2RGBFuture = std::async(std::launch::async,
          [image_out, buffer, imageCaptureSystemTimestamp_ms, imageId]() mutable -> Vision::ImageRGB
          {
            image_out.Allocate(CAMERA_SENSOR_RESOLUTION_HEIGHT, CAMERA_SENSOR_RESOLUTION_WIDTH);

            // Wrap the YUV data in an opencv mat
            // A 1280x720 YUV420sp image is 1280x1080 bytes
            // A full Y channel followed by interleaved 2x2 subsampled UV
            // Calculate the number of rows of bytes of YUV data that corresponds to
            // CAMERA_SENSOR_RESOLUTION_WIDTH cols of data
            constexpr u16 kNumYUVRows =
              (2 * (CAMERA_SENSOR_RESOLUTION_HEIGHT/2 * CAMERA_SENSOR_RESOLUTION_WIDTH/2) /
               CAMERA_SENSOR_RESOLUTION_WIDTH) + CAMERA_SENSOR_RESOLUTION_HEIGHT;
            cv::Mat yuv(kNumYUVRows, CAMERA_SENSOR_RESOLUTION_WIDTH, CV_8UC1, buffer);
            // Convert from YUV to BGR directly into image_out
            // BGR appears to actually result in RGB data, there is a similar bug
            // when converting from RGB565 to RGB
            cv::cvtColor(yuv, image_out.get_CvMat_(), cv::COLOR_YUV2BGR_NV21);

            // Create image with proper imageID and timestamp
            image_out.SetTimestamp(imageCaptureSystemTimestamp_ms);
            image_out.SetImageId(imageId);

            return image_out;
          });

        return false;
      }
      else if(format == ImageEncoding::RawRGB)
      {
        // Create ImageRGB object from image buffer
        image_out = Vision::ImageRGB(numRows, numCols, buffer);
      }
      else
      {
        PRINT_NAMED_ERROR("VisionComponent.CaptureImage.UnexpectedFormat",
                          "Camera returned unexpected image format %s",
                          EnumToString(format));
        return false;
      }

      if(kDisplayUndistortedImages)
      {
        Vision::ImageRGB imgUndistorted(numRows,numCols);
        DEV_ASSERT(_camera->IsCalibrated(), "VisionComponent.GetCalibrationImageJpegData.NoCalibration");
        image_out.Undistort(*_camera->GetCalibration(), imgUndistorted);
        imgUndistorted.Display("UndistortedImage");
      }

      // Create image with proper imageID and timestamp
      image_out.SetTimestamp(imageCaptureSystemTimestamp_ms);
      image_out.SetImageId(imageId);
    }

    return gotImage;
  }

  f32 VisionComponent::GetBodyTurnSpeedThresh_degPerSec() const
  {
    return kBodyTurnSpeedThreshBlock_degs;
  }

  void VisionComponent::SetPhysicalRobot(const bool isPhysical)
  {
    const f32 padding = isPhysical ? LIFT_HARDWARE_FALL_SLACK_MM : 0.f;
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

  bool VisionComponent::SetCameraCaptureFormat(ImageEncoding format)
  {
    // If the future is already valid then we are currently waiting for
    // a previous format change to take effect
    if(_captureFormatState != CaptureFormatState::None)
    {
      PRINT_NAMED_WARNING("VisionComponent.SetCameraCaptureFormat.StillSettingPrevFormat",
                          "Still waiting for previous format %s to be applied",
                          EnumToString(_desiredImageFormat));
      return false;
    }

    // Pause and wait for VisionSystem to finish processing the current image
    // before changing formats since that will release all shared camera memory
    Pause(true);
    
    // Clear the next image so that it doesn't get swapped in to current immediately
    Lock();
    ReleaseImage(_nextImg);
    Unlock();

    _desiredImageFormat = format;

    _captureFormatState = CaptureFormatState::WaitingForProcessingToStop;

    PRINT_CH_INFO("VisionComponent", "VisionComponent.SetCamerCaptureFormat.RequestingSwitch",
                  "From %s to %s",
                  ImageEncodingToString(_currentImageFormat), ImageEncodingToString(_desiredImageFormat));
    
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
        PRINT_CH_INFO("VisionComponent", "VisionComponent.UpdateCaptureFormatChange.WaitingForProcessingToStop", "");
        Lock();
        const bool curImgEmpty = _currentImg.IsEmpty();

        // If the current image is empty so
        // VisionSystem has finished processing it
        if(curImgEmpty)
        {
          // Make sure to also clear image cache
          _visionSystem->ClearImageCache();

          // Tell the camera to change format now that nothing is
          // using the shared camera memory
          auto cameraService = CameraService::getInstance();
          cameraService->CameraSetCaptureFormat(_desiredImageFormat);

          PRINT_CH_INFO("VisionComponent", "VisionComponent.UpdateCaptureFormatChange.SwitchToWaitForFrame",
                        "Now in %s", ImageEncodingToString(_currentImageFormat));
          
          _captureFormatState = CaptureFormatState::WaitingForFrame;
        }

        Unlock();

        return;
      }

      case CaptureFormatState::WaitingForFrame:
      {
        PRINT_CH_INFO("VisionComponent", "VisionComponent.UpdateCaptureFormatChange.WaitingForFrameWithNewFormat", "");
        
        s32 expectedNumRows = 0;
        switch(_desiredImageFormat)
        {
          case ImageEncoding::RawRGB:
            expectedNumRows = DEFAULT_CAMERA_RESOLUTION_HEIGHT;
            break;
            
          case ImageEncoding::YUV420sp:
            expectedNumRows = CAMERA_SENSOR_RESOLUTION_HEIGHT;
            break;
            
          default:
            PRINT_NAMED_ERROR("VisionComponent.UpdateCaptureFormatChange.BadDesiredFormat", "%s",
                              ImageEncodingToString(_desiredImageFormat));
            return;
        }
        
        if(gotNumRows == expectedNumRows)
        {
          DEV_ASSERT(_paused, "VisionComponent.UpdateCaptureFormatChange.ExpectingVisionComponentToBePaused");
          _captureFormatState = CaptureFormatState::None;
          _currentImageFormat = _desiredImageFormat;
          _desiredImageFormat = ImageEncoding::NoneImageEncoding;
          Pause(false); // now that state/format are updated, un-pause the vision system
          PRINT_CH_INFO("VisionComponent", "VisionComponent.UpdateCaptureFormatChange.FormatChangeComplete",
                        "New format: %s, NumRows=%d", ImageEncodingToString(_currentImageFormat), gotNumRows);
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
    EnableVisionWhileMovingFast(msg.enable);
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

  // Helper function to get the full path to face albums if isRelative=true
  inline static std::string GetFullFaceAlbumPath(const CozmoContext* context, const std::string& pathIn, bool isRelative)
  {
    if(isRelative) {
      return context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                        Util::FileUtils::FullFilePath({"config", "basestation", "faceAlbums", pathIn}));
    } else {
      return pathIn;
    }
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::SaveFaceAlbumToFile& msg)
  {
    SaveFaceAlbumToFile(GetFullFaceAlbumPath(_context, msg.path, msg.isRelativePath));
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::LoadFaceAlbumFromFile& msg)
  {
    LoadFaceAlbumFromFile(GetFullFaceAlbumPath(_context, msg.path, msg.isRelativePath));
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
    PRINT_NAMED_WARNING("VisionComponent.HandleEnableColorImages.NotImplemented", "");
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::SetCameraSettings& payload)
  {
    EnableAutoExposure(payload.enableAutoExposure);

    // If we are not enabling auto exposure (we are disabling it) then set the exposure and gain
    if(!payload.enableAutoExposure)
    {
      SetExposureSettings(payload.exposure_ms, payload.gain);
    }
  }

  void VisionComponent::SetSaveImageParameters(const ImageSendMode saveMode,
                                               const std::string& path,
                                               const std::string& basename,
                                               const int8_t onRobotQuality,
                                               const Vision::ImageCache::Size& saveSize,
                                               const bool removeRadialDistortion,
                                               const f32 thumbnailScaleFraction)
  {
    if(nullptr != _visionSystem)
    {
      std::string fullPath(path);
      if(fullPath.empty())
      {
        // If no path specified, default to <cachePath>/camera/images
        const std::string cachePath = _robot->GetContext()->GetDataPlatform()->pathToResource(Util::Data::Scope::Cache, "camera");
        fullPath = Util::FileUtils::FullFilePath({cachePath, "images"});
      }
      
      _visionSystem->SetSaveParameters(saveMode,
                                       fullPath,
                                       basename,
                                       onRobotQuality,
                                       saveSize,
                                       removeRadialDistortion,
                                       thumbnailScaleFraction);

      if(saveMode != ImageSendMode::Off)
      {
        EnableMode(VisionMode::SavingImages, true);
      }

      PRINT_CH_DEBUG("VisionComponent", "VisionComponent.SetSaveImageParameters.SaveImages",
                     "Setting image save mode to %s. Saving to: %s",
                     EnumToString(saveMode), fullPath.c_str());
    }
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::SaveImages& payload)
  {
    SetSaveImageParameters(payload.mode, payload.path, "",
                           payload.qualityOnRobot,
                           Vision::ImageCache::Size::Full,
                           payload.removeRadialDistortion);
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::DevSubscribeVisionModes& payload)
  {
    _robot->GetVisionScheduleMediator().DevOnly_SelfSubscribeVisionMode(GetVisionModesFromFlags(payload.bitFlags));
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::DevUnsubscribeVisionModes& payload)
  {
    _robot->GetVisionScheduleMediator().DevOnly_SelfUnsubscribeVisionMode(GetVisionModesFromFlags(payload.bitFlags));
  }

  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::SetCameraCaptureFormat& msg)
  {
    SetCameraCaptureFormat(msg.format);
  }

  std::set<VisionMode> VisionComponent::GetVisionModesFromFlags(u32 bitflags) const
  {
    Util::BitFlags32<VisionMode> flags;
    flags.SetFlags(bitflags);
    std::set<VisionMode> visionModes;
    for(uint32_t i=0; i<VisionModeNumEntries; ++i) {
      auto mode = static_cast<VisionMode>(i);
      if(flags.IsBitFlagSet(mode)) {
        visionModes.insert(mode);
      }
    }
    return visionModes;
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
            PRINT_NAMED_WARNING("VisionComponent.ReadCameraCalibration.SizeMismatch",
                                "Expected %zu, got %zu",
                                NVStorageComponent::MakeWordAligned(payload.Size()), size);
          } else {

            payload.Unpack(data, size);

            std::stringstream ss;
            ss << "[";
            for(int i = 0; i < payload.distCoeffs.size() - 1; ++i)
            {
              ss << payload.distCoeffs[i] << ", ";
            }
            ss << payload.distCoeffs.back() << "]";

            PRINT_NAMED_INFO("VisionComponent.ReadCameraCalibration.Recvd",
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
              PRINT_NAMED_WARNING("VisionComponent.ReadCameraCalibration.SendCameraFOVFailed", "");
            }
          }
        }
	// If this is the factory test and we failed to read calibration then use a dummy one
	// since we should be getting a real one during playpen
	else if(FACTORY_TEST)
	{
          PRINT_NAMED_WARNING("VisionComponent.ReadCameraCalibration.Failed", "");

          // TEMP HACK: Use dummy calibration for now since final camera not available yet
          PRINT_NAMED_WARNING("VisionComponent.ReadCameraCalibration.UsingDummyV2Calibration", "");

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
            PRINT_NAMED_WARNING("VisionComponent.ReadCameraCalibration.SendCameraFOVFailed", "");
          }
        }
	else
	{
	  PRINT_NAMED_ERROR("VisionComponent.ReadCameraCalibration.Failed", "");
	  return;
	}

        Enable(true);
      };

      _robot->GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_CameraCalib, readCamCalibCallback);

    }
  }

} // namespace Cozmo
} // namespace Anki
