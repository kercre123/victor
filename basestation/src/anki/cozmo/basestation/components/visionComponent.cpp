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


#include "anki/cozmo/basestation/actions/basicActions.h"
#ifdef COZMO_V2
#include "anki/cozmo/basestation/androidHAL/androidHAL.h"
#endif
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/petWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotStateHistory.h"
#include "anki/cozmo/basestation/visionModesHelpers.h"
#include "anki/cozmo/basestation/visionSystem.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "anki/vision/basestation/image_impl.h"
#include "anki/vision/basestation/trackedFace.h"
#include "anki/vision/basestation/observableObjectLibrary_impl.h"
#include "anki/vision/MarkerCodeDefinitions.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/helpers/boundedWhile.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/robot/config.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/imageTypes.h"

#include "opencv2/highgui/highgui.hpp"

namespace Anki {
namespace Cozmo {

  CONSOLE_VAR(f32, kHeadTurnSpeedThreshBlock_degs, "WasRotatingTooFast.Block.Head_deg/s",   10.f);
  CONSOLE_VAR(f32, kBodyTurnSpeedThreshBlock_degs, "WasRotatingTooFast.Block.Body_deg/s",   30.f);
  CONSOLE_VAR(f32, kHeadTurnSpeedThreshFace_degs,  "WasRotatingTooFast.Face.Head_deg/s",    10.f);
  CONSOLE_VAR(f32, kBodyTurnSpeedThreshFace_degs,  "WasRotatingTooFast.Face.Body_deg/s",    30.f);
  CONSOLE_VAR(u8,  kNumImuDataToLookBack,          "WasRotatingTooFast.Face.NumToLookBack", 5);
  CONSOLE_VAR(f32, kHeadTurnSpeedThreshPet_degs,   "WasRotatingTooFast.Pet.Head_deg/s",     10.f);
  CONSOLE_VAR(f32, kBodyTurnSpeedThreshPet_degs,   "WasRotatingTooFast.Pet.Body_deg/s",     30.f);
  
  
  // Whether or not to do rolling shutter correction for physical robots
  CONSOLE_VAR(bool, kRollingShutterCorrectionEnabled, "Vision.PreProcessing", true);
  CONSOLE_VAR(f32,  kMinCameraGain,                   "Vision.PreProcessing", 0.1f);
  
  // Amount of time we sleep when paused, waiting for next image, and after processing each image
  // (in order to provide a little breathing room for main thread)
  CONSOLE_VAR_RANGED(u8, kVision_MinSleepTime_ms, "Vision.General", 2, 1, 10);
  
  // Set to a value greater than 0 to randomly drop that fraction of frames, for testing
  CONSOLE_VAR_RANGED(f32, kSimulateDroppedFrameFraction, "Vision.General", 0.f, 0.f, 1.f); // DO NOT COMMIT > 0!
  
  CONSOLE_VAR(bool, kVisualizeObservedMarkersIn3D, "Vision.General", false);
  CONSOLE_VAR(bool, kDrawMarkerNames,              "Vision.General", false); // In viz camera view
  
  
  namespace JsonKey
  {
    const char * const ImageQualityGroup = "ImageQuality";
    const char * const PerformanceLogging = "PerformanceLogging";
    const char * const DropStatsWindowLength = "DropStatsWindowLength_sec";
    const char * const ImageQualityAlertDuration = "TimeBeforeErrorMessage_ms";
    const char * const ImageQualityAlertSpacing = "RepeatedErrorMessageInverval_ms";
    const char * const InitialExposureTime = "InitialExposureTime_ms";
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
  
  VisionComponent::VisionComponent(Robot& robot, const CozmoContext* context)
  : _robot(robot)
  , _context(context)
  , _vizManager(context->GetVizManager())
  , _camera(robot.GetID())
  {
    std::string dataPath("");
    if(context->GetDataPlatform() != nullptr) {
      dataPath = context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                            Util::FileUtils::FullFilePath({"config", "basestation", "vision"}));
    } else {
      PRINT_NAMED_WARNING("VisionComponent.Constructor.NullDataPlatform",
                          "Insantiating VisionSystem with no context and/or data platform.");
    }
    
    _visionSystem = new VisionSystem(dataPath, _vizManager);
    
    // Set up event handlers
    if(nullptr != context && nullptr != context->GetExternalInterface())
    {
      using namespace ExternalInterface;
      auto helper = MakeAnkiEventUtil(*context->GetExternalInterface(), *this, _signalHandles);
      
      // In alphabetical order:
      helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableVisionMode>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::EraseAllEnrolledFaces>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::EraseEnrolledFaceByID>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::LoadFaceAlbumFromFile>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SaveFaceAlbumToFile>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::UpdateEnrolledFaceByID>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::VisionRunMode>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::VisionWhileMoving>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SetCameraSettings>();

      // Separate list for engine messages to listen to:
      helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotConnectionResponse>();
    }
    
  } // VisionSystem()

  
  Result VisionComponent::Init(const Json::Value& config)
  {
    _isInitialized = false;
    
    // Helper macro for grabbing a parameter from Json config and printing an
    // error message and returning failure if it doesn't exist
#   define GET_JSON_PARAMETER(__json__, __fieldName__, __variable__) \
    do { \
      if(!JsonTools::GetValueOptional(__json__, __fieldName__, __variable__)) { \
        PRINT_NAMED_ERROR("VisionSystem.Init.MissingJsonParameter", "%s", __fieldName__); \
        return RESULT_FAIL; \
    }} while(0)

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
      return result;
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
    return RESULT_OK;
    
  } //Init()
  
  void VisionComponent::SetCameraCalibration(Vision::CameraCalibration& camCalib)
  {
    if(_camCalib != camCalib || !_isCamCalibSet)
    {
      _camCalib = camCalib;
      _camera.SetSharedCalibration(&_camCalib);
      _isCamCalibSet = true;
     
      Lock();
      _visionSystem->UpdateCameraCalibration(_camCalib);
      Unlock();
     
      if(!_isSynchronous)
      {
        Start();
      }
      
      // Got a new calibration: rebuild the LUT for ground plane homographies
      PopulateGroundPlaneHomographyLUT();
      
      // Fine-tune calibration using tool code dots
      //_robot.GetActionList().QueueActionNext(new ReadToolCodeAction(_robot));
    }
    
    if(kRollingShutterCorrectionEnabled)
    {
      _visionSystem->ShouldDoRollingShutterCorrection(_robot.IsPhysical());
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
    if(!_isCamCalibSet) {
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
    Stop();
    
    Util::SafeDelete(_visionSystem);
  } // ~VisionSystem()
 
  
  void VisionComponent::SetMarkerToTrack(const Vision::Marker::Code&  markerToTrack,
                        const Point2f&               markerSize_mm,
                        const Point2f&               imageCenter,
                        const f32                    radius,
                        const bool                   checkAngleX,
                        const f32                    postOffsetX_mm,
                        const f32                    postOffsetY_mm,
                        const f32                    postOffsetAngle_rad)
  {
    if(_visionSystem != nullptr) {
      Embedded::Point2f pt(imageCenter.x(), imageCenter.y());
      Vision::MarkerType markerType = static_cast<Vision::MarkerType>(markerToTrack);
      _visionSystem->SetMarkerToTrack(markerType, markerSize_mm,
                                      pt, radius, checkAngleX,
                                      postOffsetX_mm,
                                      postOffsetY_mm,
                                      postOffsetAngle_rad);
    } else {
      PRINT_NAMED_ERROR("VisionComponent.SetMarkerToTrack.NullVisionSystem",
                        "Cannot set vision marker to track before vision system is instantiated.");
    }
  }
  
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
  
  Result VisionComponent::EnableToolCodeCalibration(bool enable)
  {
    if(nullptr != _visionSystem) {
      return _visionSystem->EnableToolCodeCalibration(enable);
    } else {
      PRINT_NAMED_ERROR("VisionComponent.EnableToolCodeCalibration.NullVisionSystem", "");
      return RESULT_FAIL;
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
  
  Result VisionComponent::SetNextImage(EncodedImage& encodedImage)
  {
    if(!_isInitialized) {
      PRINT_NAMED_WARNING("VisionComponent.SetNextImage.NotInitialized", "t=%u(%d)",
                          encodedImage.GetTimeStamp(), encodedImage.GetTimeStamp());
      return RESULT_FAIL;
    }
    
    if (!_enabled) {
      PRINT_CH_INFO("VisionComponent",
                    "VisionComponent.VisionComponent.SetNextImage", "Set next image but not enabled, t=%u(%d)",
                    encodedImage.GetTimeStamp(), encodedImage.GetTimeStamp());
      return RESULT_OK;
    }
  
    // Track how fast we are receiving frames
    if(_lastReceivedImageTimeStamp_ms > 0) {
      // Time should not move backwards!
      const bool timeWentBackwards = encodedImage.GetTimeStamp() < _lastReceivedImageTimeStamp_ms;
      if (timeWentBackwards)
      {
        PRINT_NAMED_WARNING("VisionComponent.SetNextImage.UnexpectedTimeStamp",
                            "Current:%u Last:%u",
                            encodedImage.GetTimeStamp(), _lastReceivedImageTimeStamp_ms);
        
        // This should be recoverable (it could happen if we receive a bunch of garbage image data)
        // so reset the lastReceived and lastProcessd timestamps so we can set them fresh next time
        // we get an image
        _lastReceivedImageTimeStamp_ms = 0;
        _lastProcessedImageTimeStamp_ms = 0;
        return RESULT_FAIL;
      }
      _framePeriod_ms = encodedImage.GetTimeStamp() - _lastReceivedImageTimeStamp_ms;
    }
    _lastReceivedImageTimeStamp_ms = encodedImage.GetTimeStamp();
    
    if(_isCamCalibSet) {
      DEV_ASSERT(nullptr != _visionSystem, "VisionComponent.SetNextImage.NullVisionSystem");
      DEV_ASSERT(_visionSystem->IsInitialized(), "VisionComponent.SetNextImage.VisionSystemNotInitialized");

      // Populate IMU data history for this image, for rolling shutter correction
      GetImuDataHistory().CalculateTimestampForImageIMU(encodedImage.GetImageID(),
                                                        encodedImage.GetTimeStamp(),
                                                        RollingShutterCorrector::timeBetweenFrames_ms,
                                                        encodedImage.GetHeight());
    
      // Fill in the pose data for the given image, by querying robot history
      HistRobotState imageHistState;
      TimeStamp_t imageHistTimeStamp;
      
      Result lastResult = GetImageHistState(_robot, encodedImage.GetTimeStamp(), imageHistState, imageHistTimeStamp);

      if(lastResult == RESULT_FAIL_ORIGIN_MISMATCH)
      {
        // Don't print a warning for this case: we expect not to get pose history
        // data successfully
        PRINT_NAMED_INFO("VisionComponent.SetNextImage.OriginMismatch",
                         "Could not get pose data for t=%u due to origin mismatch. Returning OK", encodedImage.GetTimeStamp());
        return RESULT_OK;
      }
      else if(lastResult != RESULT_OK)
      {
        PRINT_NAMED_WARNING("VisionComponent.SetNextImage.StateHistoryFail",
                            "Unable to get computed pose at image timestamp of %d. (rawStates: have %zu from %d:%d) (visionStates: have %zu from %d:%d)",
                            encodedImage.GetTimeStamp(),
                            _robot.GetStateHistory()->GetNumRawStates(),
                            _robot.GetStateHistory()->GetOldestTimeStamp(),
                            _robot.GetStateHistory()->GetNewestTimeStamp(),
                            _robot.GetStateHistory()->GetNumVisionStates(),
                            _robot.GetStateHistory()->GetOldestVisionOnlyTimeStamp(),
                            _robot.GetStateHistory()->GetNewestVisionOnlyTimeStamp());
        return lastResult;
      }
      
      // Get most recent pose data in history
      Anki::Cozmo::HistRobotState lastHistState;
      _robot.GetStateHistory()->GetLastStateWithFrameID(_robot.GetPoseFrameID(), lastHistState);
           
      Lock();
      _nextPoseData.histState = imageHistState;
      _nextPoseData.timeStamp = imageHistTimeStamp;
      _nextPoseData.cameraPose = _robot.GetHistoricalCameraPose(_nextPoseData.histState, _nextPoseData.timeStamp);
      _nextPoseData.groundPlaneVisible = LookupGroundPlaneHomography(_nextPoseData.histState.GetHeadAngle_rad(),
                                                                     _nextPoseData.groundPlaneHomography);
      _nextPoseData.imuDataHistory = _imuHistory;
      Unlock();
      
      // Experimental:
      //UpdateOverheadMap(image, _nextPoseData);
      
      // Store image for calibration or factory test (*before* we swap encodedImage with _nextImg below!)
      // NOTE: This means we do decoding on main thread, but this is just for the factory
      //       test, so I'm not going to the trouble to store encoded images for calibration
      if (_storeNextImageForCalibration || _doFactoryDotTest)
      {
        // If we were moving too fast at the timestamp the image was taken then don't use it for
        // calibration or dot test purposes
        if(!WasRotatingTooFast(encodedImage.GetTimeStamp(), DEG_TO_RAD(0.1), DEG_TO_RAD(0.1), 3))
        {
          Vision::Image imageGray;
          Result result = encodedImage.DecodeImageGray(imageGray);
          if(RESULT_OK != result)
          {
            PRINT_NAMED_WARNING("VisionComponent.SetNextImage.DecodeFailed",
                                "Cannot use next image for calibration(%d) or dot test(%d)",
                                _storeNextImageForCalibration, _doFactoryDotTest);
          }
          else
          {
            if(_storeNextImageForCalibration)
            {
              _storeNextImageForCalibration = false;
              if (IsModeEnabled(VisionMode::ComputingCalibration)) {
                PRINT_NAMED_INFO("VisionComponent.SetNextImage.SkippingStoringImageBecauseAlreadyCalibrating", "");
              } else {
                result = _visionSystem->AddCalibrationImage(imageGray, _calibTargetROI);
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
              _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
              
            } // if(_doFactoryDotTest)
            
          } // if(decode failed)
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
      
      if(_isSynchronous)
      {
        if(!_paused) {
          UpdateVisionSystem(_nextPoseData, encodedImage);
        }
      }
      else
      {
        if(!_paused) {
          ANKI_CPU_PROFILE("VC::SetNextImage.LockedSwap");
          Lock();
          
          const bool isDroppingFrame = !_nextImg.IsEmpty() || (kSimulateDroppedFrameFraction > 0.f &&
                                                               _robot.GetContext()->GetRandom()->RandDbl() < kSimulateDroppedFrameFraction);
          if(isDroppingFrame)
          {
            PRINT_CH_DEBUG("VisionComponent",
                           "VisionComponent.SetNextImage.DroppedFrame",
                           "Setting next image with t=%u, but existing next image from t=%u not yet processed (currently on t=%u).",
                           encodedImage.GetTimeStamp(),
                           _nextImg.GetTimeStamp(),
                           _currentImg.GetTimeStamp());
          }
          _dropStats.Update(isDroppingFrame);
          
          // Make encoded image the new "next" image
          std::swap(_nextImg, encodedImage);
          
          // Because encodedImage mantains state in the form of _prevTimestamp and _nextImg has garbage data
          // so after the swap encodedImage now has garbage data so we need to reassign prevTimestamp
          encodedImage.SetPrevTimestamp(_nextImg.GetPrevTimestamp());
          
          Unlock();
        }
      }

      
    } else {
      PRINT_NAMED_WARNING("VisionComponent.Update.NoCamCalib",
                          "Camera calibration should be set before calling Update().");
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
    
  } // SetNextImage()
  
  void VisionComponent::PopulateGroundPlaneHomographyLUT(f32 angleResolution_rad)
  {
    const Pose3d& robotPose = _robot.GetPose();
    
    DEV_ASSERT(_camera.IsCalibrated(), "VisionComponent.PopulateGroundPlaneHomographyLUT.CameraNotCalibrated");
    
    const Matrix_3x3f K = _camera.GetCalibration()->GetCalibrationMatrix();
    
    GroundPlaneROI groundPlaneROI;
    
    // Loop over all possible head angles at the specified resolution and store
    // the ground plane homography for each.
    for(f32 headAngle_rad = MIN_HEAD_ANGLE; headAngle_rad <= MAX_HEAD_ANGLE;
        headAngle_rad += angleResolution_rad)
    {
      // Get the robot origin w.r.t. the camera position with the camera at
      // the current head angle
      Pose3d robotPoseWrtCamera;
#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
      bool result = robotPose.GetWithRespectTo(_robot.GetCameraPose(headAngle_rad), robotPoseWrtCamera);
      assert(result == true); // this really shouldn't fail! camera has to be in the robot's pose tree
#else
      robotPose.GetWithRespectTo(_robot.GetCameraPose(headAngle_rad), robotPoseWrtCamera);
#endif
      const RotationMatrix3d& R = robotPoseWrtCamera.GetRotationMatrix();
      const Vec3f&            T = robotPoseWrtCamera.GetTranslation();
      
      // Construct the homography mapping points on the ground plane into the
      // image plane
      const Matrix_3x3f H = K*Matrix_3x3f{R.GetColumn(0),R.GetColumn(1),T};
      
      Quad2f imgQuad;
      groundPlaneROI.GetImageQuad(H, _camCalib.GetNcols(), _camCalib.GetNrows(), imgQuad);
      
      if(_camera.IsWithinFieldOfView(imgQuad[Quad::CornerName::TopLeft]) ||
         _camera.IsWithinFieldOfView(imgQuad[Quad::CornerName::BottomLeft]))
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
    if(atHeadAngle > _groundPlaneHomographyLUT.rbegin()->first) {
      // Head angle too large
      return false;
    }
    
    auto iter = _groundPlaneHomographyLUT.lower_bound(atHeadAngle);
    
    if(iter == _groundPlaneHomographyLUT.end()) {
      PRINT_NAMED_WARNING("VisionComponent.LookupGroundPlaneHomography.KeyNotFound",
                          "Failed to find homogrphay using headangle of %.2frad (%.1fdeg) as lower bound",
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

  void VisionComponent::UpdateVisionSystem(const VisionPoseData&  poseData,
                                           const EncodedImage&    encodedImg)
  {
    ANKI_CPU_PROFILE("VC::UpdateVisionSystem");
    Result result = _visionSystem->Update(poseData, encodedImg);
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
    
    
    const char* threadName = "VisionSystem";
    #if defined(LINUX) || defined(ANDROID)
    pthread_setname_np(pthread_self(), threadName);
    #else
    pthread_setname_np(threadName);
    #endif
    
    while (_running) {
      
      if(_paused) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kVision_MinSleepTime_ms));
        continue;
      }
      
      ANKI_CPU_TICK("VisionComponent", 100.0, Util::CpuThreadProfiler::kLogFrequencyNever);

      if (!_currentImg.IsEmpty())
      {
        ANKI_CPU_PROFILE("ProcessImage");
        // There is an image to be processed; do so now
        UpdateVisionSystem(_currentPoseData, _currentImg);
        
        Lock();
        // Clear it when done.
        _currentImg.Clear();
        _nextImg.Clear();
        Unlock();
        
        // Sleep to alleviate pressure on main thread
        std::this_thread::sleep_for(std::chrono::milliseconds(kVision_MinSleepTime_ms));
      }
      else if(!_nextImg.IsEmpty())
      {
        ANKI_CPU_PROFILE("SwapInNewImage");
        // We have an image waiting to be processed: swap it in (avoid copy)
        Lock();
        std::swap(_currentImg, _nextImg);
        std::swap(_currentPoseData, _nextPoseData);
        _nextImg.Clear();
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

    PRINT_NAMED_INFO("VisionComponent.Processor",
                     "Terminated Robot VisionComponent::Processor thread");
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
          if(markerPose.GetWithRespectTo(marker.GetSeenBy().GetPose().FindOrigin(), markerPose) == true) {
            _robot.GetContext()->GetVizManager()->DrawGenericQuad(quadID++, blockMarker->Get3dCorners(markerPose), NamedColors::OBSERVED_QUAD);
          } else {
            PRINT_NAMED_WARNING("VisionComponent.VisualizeObservedMarkerIn3D.MarkerOriginNotCameraOrigin",
                                "Cannot visualize a marker whose pose origin is not the camera's origin that saw it.");
          }
        }
      }
      
      return true;
    });
    
    _robot.GetBlockWorld().FindLocatedMatchingObject(filter);
    
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
        using localHandlerType = Result(VisionComponent::*)(const VisionProcessingResult& procResult);
        auto tryAndReport = [this, &result, &anyFailures]( localHandlerType handler, VisionMode mode )
        {
          if (!result.modesProcessed.IsBitFlagSet(mode))
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
        tryAndReport(&VisionComponent::UpdateVisionMarkers,       VisionMode::DetectingMarkers);
        
        // NOTE: UpdateFaces will also update FaceWorld (which broadcasts face observations
        //  and should be done before sending RobotProcessedImage below!)
        tryAndReport(&VisionComponent::UpdateFaces,               VisionMode::DetectingFaces);
        
        // NOTE: UpdatePets will also update PetWorld (which broadcasts pet face observations
        //  and should be done before sending RobotProcessedImage below!)
        tryAndReport(&VisionComponent::UpdatePets,                VisionMode::DetectingPets);
        
        // Note: tracking mode has two associated update calls:
        tryAndReport(&VisionComponent::UpdateTrackingQuad,        VisionMode::Tracking);
        tryAndReport(&VisionComponent::UpdateDockingErrorSignal,  VisionMode::Tracking);
        
        tryAndReport(&VisionComponent::UpdateMotionCentroid,      VisionMode::DetectingMotion);
        tryAndReport(&VisionComponent::UpdateOverheadEdges,       VisionMode::DetectingOverheadEdges);
        tryAndReport(&VisionComponent::UpdateToolCode,            VisionMode::ReadingToolCode);
        tryAndReport(&VisionComponent::UpdateComputedCalibration, VisionMode::ComputingCalibration);
        tryAndReport(&VisionComponent::UpdateImageQuality,        VisionMode::CheckingQuality);
        
        // Display any debug images left by the vision system
        for(auto & debugGray : result.debugImages) {
          debugGray.second.Display(debugGray.first.c_str());
        }
        for(auto & debugRGB : result.debugImageRGBs) {
          debugRGB.second.Display(debugRGB.first.c_str());
        }
        
        // Store frame rate and last image processed time. Time should only move forward.
        DEV_ASSERT(result.timestamp >= _lastProcessedImageTimeStamp_ms, "VisionComponent.UpdateAllResults.BadTimeStamp");
        _processingPeriod_ms = result.timestamp - _lastProcessedImageTimeStamp_ms;
        _lastProcessedImageTimeStamp_ms = result.timestamp;
        
        auto visionModesList = std::vector<VisionMode>();
        for (VisionMode mode = VisionMode::Idle; mode < VisionMode::Count; ++mode)
        {
          if (result.modesProcessed.IsBitFlagSet(mode))
          {
            visionModesList.push_back(mode);
          }
        }
        
        // Send the processed image message last
        _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotProcessedImage(result.timestamp, std::move(visionModesList))));
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
      TimeStamp_t t;
      HistRobotState* histStatePtr = nullptr;
      HistStateKey histStateKey;
      
      lastResult = _robot.GetStateHistory()->ComputeAndInsertStateAt(procResult.timestamp, t, &histStatePtr, &histStateKey, true);
      
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
                      _robot.GetStateHistory()->GetOldestTimeStamp(),
                      _robot.GetStateHistory()->GetNewestTimeStamp());
        return RESULT_OK;
      }
      
      if(&histStatePtr->GetPose().FindOrigin() != _robot.GetWorldOrigin()) {
        PRINT_CH_INFO("VisionComponent", "VisionComponent.UpdateVisionMarkers.OldOrigin",
                      "Ignoring observed marker from origin %s (robot origin is %s)",
                      histStatePtr->GetPose().FindOrigin().GetName().c_str(),
                      _robot.GetWorldOrigin()->GetName().c_str());
        return RESULT_OK;
      }
      
      // If we get here, ComputeAndInsertPoseIntoHistory() should have succeeded
      // and this should be true
      assert(procResult.timestamp == t);
      
      // If we were moving too fast at timestamp t and we aren't doing rolling shutter correction
      // then don't queue this marker otherwise don't queue if only the head was moving too fast
      if(WasRotatingTooFast(t, DEG_TO_RAD(kBodyTurnSpeedThreshBlock_degs), DEG_TO_RAD(kHeadTurnSpeedThreshBlock_degs)))
      {
        return RESULT_OK;
      }
      
      Vision::Camera histCamera = _robot.GetHistoricalCamera(*histStatePtr, procResult.timestamp);
      
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
            !_robot.GetStateHistory()->IsValidKey(histStateKey))
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
        
        if(kDrawMarkerNames)
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

    lastResult = _robot.GetBlockWorld().Update(observedMarkers);
    if(RESULT_OK != lastResult)
    {
      PRINT_NAMED_WARNING("VisionComponent.UpdateVisionResults.BlockWorldUpdateFailed", "");
    }
    
    return lastResult;
    
  } // UpdateVisionMarkers()
  
  Result VisionComponent::UpdateFaces(const VisionProcessingResult& procResult)
  {
    Result lastResult = RESULT_OK;

    for(auto & updatedID : procResult.updatedFaceIDs)
    {
      _robot.GetFaceWorld().ChangeFaceID(updatedID);
    }
    
    std::list<Vision::TrackedFace> facesToUpdate;
    
    for(auto & faceDetection : procResult.faces)
    {
      /*
       PRINT_NAMED_INFO("VisionComponent.Update",
                        "Saw face at (x,y,w,h)=(%.1f,%.1f,%.1f,%.1f), "
                        "at t=%d Pose: roll=%.1f, pitch=%.1f yaw=%.1f, T=(%.1f,%.1f,%.1f).",
                        faceDetection.GetRect().GetX(), faceDetection.GetRect().GetY(),
                        faceDetection.GetRect().GetWidth(), faceDetection.GetRect().GetHeight(),
                        faceDetection.GetTimeStamp(),
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
        
        _robot.GetFaceWorld().SetFaceEnrollmentComplete(true);
      }
      
      // If we were moving too fast at the timestamp the face was detected then don't update it
      // If the detected face is being tracked than we should look farther back in imu data history
      // else we will just look at the previous and next imu data
      if((kBodyTurnSpeedThreshFace_degs > 0.f || kHeadTurnSpeedThreshFace_degs > 0.f) &&
         WasRotatingTooFast(faceDetection.GetTimeStamp(),
                          DEG_TO_RAD(kBodyTurnSpeedThreshFace_degs),
                          DEG_TO_RAD(kHeadTurnSpeedThreshFace_degs),
                          (faceDetection.IsBeingTracked() ? kNumImuDataToLookBack : 0)))
      {
        // Skip this face
        continue;
      }
      
      facesToUpdate.emplace_back(faceDetection);
    }
    
    lastResult = _robot.GetFaceWorld().Update(facesToUpdate);
    if(RESULT_OK != lastResult)
    {
      PRINT_NAMED_WARNING("VisionComponent.UpdateFaces.FaceWorldUpdateFailed", "");
    }
    
    return lastResult;
  } // UpdateFaces()
  
  Result VisionComponent::UpdatePets(const VisionProcessingResult& procResult)
  {
    std::list<Vision::TrackedPet> petsToUpdate;
    for(auto & pet : procResult.pets)
    {
      if((FLT_GT(kBodyTurnSpeedThreshPet_degs, 0.f) || FLT_GT(kHeadTurnSpeedThreshPet_degs, 0.f)) &&
         WasRotatingTooFast(pet.GetTimeStamp(),
                          DEG_TO_RAD(kBodyTurnSpeedThreshPet_degs),
                          DEG_TO_RAD(kHeadTurnSpeedThreshPet_degs),
                          (pet.IsBeingTracked() ? kNumImuDataToLookBack : 0)))
      {
        // Skip this pet
        continue;
      }
      
      petsToUpdate.emplace_back(pet);
    }
    
    Result lastResult = _robot.GetPetWorld().Update(petsToUpdate);
    
    return lastResult;
  }
  
  Result VisionComponent::UpdateTrackingQuad(const VisionProcessingResult& procResult)
  {
    for(auto & trackerQuad : procResult.trackerQuads)
    {
      // Send tracker quad info to viz
      _vizManager->SendTrackerQuad(trackerQuad.topLeft_x, trackerQuad.topLeft_y,
                                   trackerQuad.topRight_x, trackerQuad.topRight_y,
                                   trackerQuad.bottomRight_x, trackerQuad.bottomRight_y,
                                   trackerQuad.bottomLeft_x, trackerQuad.bottomLeft_y);
    }

    return RESULT_OK;
  } // UpdateTrackingQuad()
  
  Result VisionComponent::UpdateDockingErrorSignal(const VisionProcessingResult& procResult)
  {
    for(auto markerPoseWrtCamera : procResult.dockingPoses)
    {
      // Hook the pose coming out of the vision system up to the historical
      // camera at that timestamp
      HistRobotState histState;
      TimeStamp_t p_timeStamp;
      Result poseStampResult = GetImageHistState(_robot, procResult.timestamp, histState, p_timeStamp);
      if(RESULT_OK != poseStampResult)
      {
        PRINT_NAMED_WARNING("VisionComponent.UpdateDockingErrorSignal.HistoricalCameraFail",
                            "Failed to get historical camera at t=%u, Result=%0x",
                            procResult.timestamp, poseStampResult);
        return poseStampResult;
      }

      Vision::Camera histCamera = _robot.GetHistoricalCamera(histState, p_timeStamp);
      markerPoseWrtCamera.SetParent(&histCamera.GetPose());
      /*
       // Get the pose w.r.t. the (historical) robot pose instead of the camera pose
       Pose3d markerPoseWrtRobot;
       if(false == markerPoseWrtCamera.first.GetWithRespectTo(p.GetPose(), markerPoseWrtRobot)) {
       PRINT_NAMED_ERROR("VisionComponent.Update.PoseOriginFail",
       "Could not get marker pose w.r.t. robot.");
       return RESULT_FAIL;
       }
       */
      //Pose3d poseWrtRobot = poseWrtCam;
      //poseWrtRobot.PreComposeWith(camWrtRobotPose);
      Pose3d markerPoseWrtRobot(markerPoseWrtCamera);
      markerPoseWrtRobot.PreComposeWith(histCamera.GetPose());
      
      DockingErrorSignal dockErrMsg;
      dockErrMsg.timestamp = procResult.timestamp;
      dockErrMsg.x_distErr = markerPoseWrtRobot.GetTranslation().x();
      dockErrMsg.y_horErr  = markerPoseWrtRobot.GetTranslation().y();
      dockErrMsg.z_height  = markerPoseWrtRobot.GetTranslation().z();
      dockErrMsg.angleErr  = markerPoseWrtRobot.GetRotation().GetAngleAroundZaxis().ToFloat() + M_PI_2;
      
      // Visualize docking error signal
      _vizManager->SetDockingError(dockErrMsg.x_distErr,
                                   dockErrMsg.y_horErr,
                                   dockErrMsg.z_height,
                                   dockErrMsg.angleErr);

      // Try to use this for closed-loop control by sending it on to the robot
      _robot.SendRobotMessage<DockingErrorSignal>(std::move(dockErrMsg));
    }

    return RESULT_OK;
  } // UpdateDockingErrorSignal()
  
  Result VisionComponent::UpdateMotionCentroid(const VisionProcessingResult& procResult)
  {
  
    for(auto motionCentroid : procResult.observedMotions)
    {
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(motionCentroid)));
    }

    return RESULT_OK;
  } // UpdateMotionCentroid()
  
  Result VisionComponent::UpdateOverheadEdges(const VisionProcessingResult& procResult)
  {
    for(auto & edgeFrame : procResult.overheadEdges)
    {
      _robot.GetBlockWorld().ProcessVisionOverheadEdges(edgeFrame);
    }
    
    return RESULT_OK;
  }
  
  Result VisionComponent::UpdateOverheadMap(const Vision::ImageRGB& image,
                                            const VisionPoseData& poseData)
  {
    if(poseData.groundPlaneVisible)
    {
      const Matrix_3x3f& H = poseData.groundPlaneHomography;
      
      const GroundPlaneROI& roi = poseData.groundPlaneROI;
      
      Quad2f imgGroundQuad;
      roi.GetImageQuad(H, image.GetNumCols(), image.GetNumRows(), imgGroundQuad);
      
      static Vision::ImageRGB overheadMap(1000.f, 1000.f);
      
      // Need to apply a shift after the homography to put things in image
      // coordinates with (0,0) at the upper left (since groundQuad's origin
      // is not upper left). Also mirror Y coordinates since we are looking
      // from above, not below
      Matrix_3x3f InvShift{
        1.f, 0.f, roi.GetDist(), // Negated b/c we're using inv(Shift)
        0.f,-1.f, roi.GetWidthFar()*0.5f,
        0.f, 0.f, 1.f};

      Pose3d worldPoseWrtRobot = poseData.histState.GetPose().GetInverse();
      for(s32 i=0; i<roi.GetWidthFar(); ++i) {
        const u8* mask_i = roi.GetOverheadMask().GetRow(i);
        const f32 y = static_cast<f32>(i) - 0.5f*roi.GetWidthFar();
        for(s32 j=0; j<roi.GetLength(); ++j) {
          if(mask_i[j] > 0) {
            // Project ground plane point in robot frame to image
            const f32 x = static_cast<f32>(j) + roi.GetDist();
            Point3f imgPoint = H * Point3f(x,y,1.f);
            assert(imgPoint.z() > 0.f);
            const f32 divisor = 1.f / imgPoint.z();
            imgPoint.x() *= divisor;
            imgPoint.y() *= divisor;
            const s32 x_img = std::round(imgPoint.x());
            const s32 y_img = std::round(imgPoint.y());
            if(x_img >= 0 && y_img >= 0 &&
               x_img < image.GetNumCols() && y_img < image.GetNumRows())
            {
              const Vision::PixelRGB value = image(y_img, x_img);
              
              // Get corresponding map point in world coords
              Point3f mapPoint = poseData.histState.GetPose() * Point3f(x,y,0.f);
              const s32 x_map = std::round( mapPoint.x() + static_cast<f32>(overheadMap.GetNumCols())*0.5f);
              const s32 y_map = std::round(-mapPoint.y() + static_cast<f32>(overheadMap.GetNumRows())*0.5f);
              if(x_map >= 0 && y_map >= 0 &&
                 x_map < overheadMap.GetNumCols() && y_map < overheadMap.GetNumRows())
              {
                overheadMap(y_map, x_map).AlphaBlendWith(value, 0.5f);
              }
            }
          }
        }
      }
      
      Vision::ImageRGB overheadImg = roi.GetOverheadImage(image, H);
      
      static s32 updateFreq = 0;
      if(updateFreq++ == 8){ // DEBUG
        updateFreq = 0;
        Vision::ImageRGB dispImg;
        image.CopyTo(dispImg);
        dispImg.DrawQuad(imgGroundQuad, NamedColors::RED, 1);
        dispImg.Display("GroundQuad");
        overheadImg.Display("OverheadView");
        
        // Display current map with the last updated region highlighted with
        // a red border
        overheadMap.CopyTo(dispImg);
        Quad3f lastUpdate;
        poseData.histState.GetPose().ApplyTo(roi.GetGroundQuad(), lastUpdate);
        for(auto & point : lastUpdate) {
          point.x() += static_cast<f32>(overheadMap.GetNumCols()*0.5f);
          point.y() *= -1.f;
          point.y() += static_cast<f32>(overheadMap.GetNumRows()*0.5f);
        }
        dispImg.DrawQuad(lastUpdate, NamedColors::RED, 2);
        dispImg.Display("OverheadMap");
      }
    } // if ground plane is visible
    
    return RESULT_OK;
  } // UpdateOverheadMap()
  
  Result VisionComponent::UpdateToolCode(const VisionProcessingResult& procResult)
  {
    for(auto & info : procResult.toolCodes)
    {
      ExternalInterface::RobotReadToolCode msg;
      msg.info = info;
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
    }

    return RESULT_OK;
  }
  

  Result VisionComponent::UpdateComputedCalibration(const VisionProcessingResult& procResult)
  {
    for(auto & calib : procResult.cameraCalibrations)
    {
      CameraCalibration msg;
      msg.center_x = calib.GetCenter_x();
      msg.center_y = calib.GetCenter_y();
      msg.focalLength_x = calib.GetFocalLength_x();
      msg.focalLength_y = calib.GetFocalLength_y();
      msg.nrows = calib.GetNrows();
      msg.ncols = calib.GetNcols();
      msg.skew = calib.GetSkew();
      msg.distCoeffs = calib.GetDistortionCoeffs();
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
    }
  
    return RESULT_OK;
  }
  
  Result VisionComponent::UpdateImageQuality(const VisionProcessingResult& procResult)
  {
    if(!_robot.IsPhysical() || procResult.imageQuality == ImageQuality::Unchecked)
    {
      // Nothing to do
      return RESULT_OK;
    }
    
    // If auto exposure is not enabled don't set camera settings
    if(_enableAutoExposure)
    {
      SetCameraSettings(procResult.exposureTime_ms, procResult.cameraGain);
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
        
        LOG_EVENT("robot.vision.image_quality", "%s", EnumToString(errorCode));
        
        PRINT_CH_DEBUG("VisionComponent",
                       "VisionComponent.UpdateImageQuality.BroadcastingImageQualityChange",
                       "Seeing %s for more than %u > %ums, broadcasting %s",
                       EnumToString(procResult.imageQuality), timeWithThisQuality_ms,
                       _waitForNextAlert_ms, EnumToString(errorCode));
        
        using namespace ExternalInterface;
        _robot.Broadcast(MessageEngineToGame(EngineErrorCodeMessage(errorCode)));
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

  bool VisionComponent::WasHeadRotatingTooFast(TimeStamp_t t,
                                               const f32 headTurnSpeedLimit_radPerSec,
                                               const int numImuDataToLookBack)
  {
    // Check to see if the robot's body or head are
    // moving too fast to queue this marker
    if(!_visionWhileMovingFastEnabled && !_robot.IsPickingOrPlacing())
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
                                               const int numImuDataToLookBack)
  {
    // Check to see if the robot's body or head are
    // moving too fast to queue this marker
    if(!_visionWhileMovingFastEnabled && !_robot.IsPickingOrPlacing())
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
                                           const int numImuDataToLookBack)
  {
    return (WasHeadRotatingTooFast(t, headTurnSpeedLimit_radPerSec, numImuDataToLookBack) ||
            WasBodyRotatingTooFast(t, bodyTurnSpeedLimit_radPerSec, numImuDataToLookBack));
  }

  void VisionComponent::AddLiftOccluder(TimeStamp_t t_request)
  {
    // TODO: More precise check for position of lift in FOV given head angle
    HistRobotState histState;
    TimeStamp_t t;
    Result result = _robot.GetStateHistory()->GetRawStateAt(t_request, t, histState, false);
    
    if(RESULT_FAIL_ORIGIN_MISMATCH == result)
    {
      // Not a warning, this can legitimately happen
      PRINT_CH_INFO("VisionComponent",
                    "VisionComponent.VisionComponent.AddLiftOccluder.StateHistoryOriginMismatch",
                    "Cound not get pose at t=%u due to origin change. Skipping.", t_request);
      return;
    }
    else if(RESULT_OK != result)
    {
      PRINT_NAMED_WARNING("VisionComponent.WasLiftInFOV.StateHistoryFailure",
                          "Could not get raw pose at t=%u", t_request);
      return;
    }
    
    const Pose3d& liftPoseWrtCamera = _robot.GetLiftPoseWrtCamera(histState.GetLiftAngle_rad(),
                                                                  histState.GetHeadAngle_rad());
    
    const f32 padding = _robot.IsPhysical() ? LIFT_HARDWARE_FALL_SLACK_MM : 0.f;
    std::vector<Point3f> liftCrossBar{
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
    
    liftPoseWrtCamera.ApplyTo(liftCrossBar, liftCrossBar);
    
    std::vector<Point2f> liftCrossBarProj;
    _camera.Project3dPoints(liftCrossBar, liftCrossBarProj);
    _camera.AddOccluder(liftCrossBarProj, liftPoseWrtCamera.GetTranslation().Length());
  }
  
  template<class PixelType>
  Result VisionComponent::CompressAndSendImage(const Vision::ImageBase<PixelType>& img, s32 quality)
  {
    if(!_robot.HasExternalInterface()) {
      PRINT_NAMED_ERROR("VisionComponent.CompressAndSendImage.NoExternalInterface", "");
      return RESULT_FAIL;
    }
    
    Result result = RESULT_OK;
    
    ImageChunk m;
    
    const s32 captureHeight = img.GetNumRows();
    const s32 captureWidth  = img.GetNumCols();
    
    switch(captureHeight) {
      case 240:
        if (captureWidth!=320) {
          result = RESULT_FAIL;
        } else {
          m.resolution = ImageResolution::QVGA;
        }
        break;
        
      case 296:
        if (captureWidth!=400) {
          result = RESULT_FAIL;
        } else {
          m.resolution = ImageResolution::CVGA;
        }
        break;
        
      case 480:
        if (captureWidth!=640) {
          result = RESULT_FAIL;
        } else {
          m.resolution = ImageResolution::VGA;
        }
        break;
        
      default:
        result = RESULT_FAIL;
    }
    
    if(RESULT_OK != result) {
      PRINT_NAMED_ERROR("VisionComponent.CompressAndSendImage",
                        "Unrecognized resolution: %dx%d.", captureWidth, captureHeight);
      return result;
    }
    
    static u32 imgID = 0;
    const std::vector<int> compressionParams = {
      CV_IMWRITE_JPEG_QUALITY, quality
    };
    
    //cv::cvtColor(img.get_CvMat_(), img.get_CvMat_(), CV_BGR2RGB);
    
    std::vector<u8> compressedBuffer;
    cv::imencode(".jpg",  img.get_CvMat_(), compressedBuffer, compressionParams);
    
    const u32 kMaxChunkSize = static_cast<u32>(ImageConstants::IMAGE_CHUNK_SIZE);
    u32 bytesRemainingToSend = static_cast<u32>(compressedBuffer.size());
    
    m.frameTimeStamp = img.GetTimestamp();
    m.imageId = ++imgID;
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
      
      if (_robot.GetContext()->GetExternalInterface() != nullptr && _robot.GetImageSendMode() != ImageSendMode::Off) {
        _robot.Broadcast(ExternalInterface::MessageEngineToGame(ImageChunk(m)));
      }
      
      // Forward the image chunks to Viz as well (Note that this does nothing if
      // sending images is disabled in VizManager)
      _robot.GetContext()->GetVizManager()->SendImageChunk(_robot.GetID(), m);
      
      bytesRemainingToSend -= chunkSize;
      ++m.chunkId;
    }

    return RESULT_OK;
    
  } // CompressAndSendImage()
  
  // Explicit instantiation for grayscale and RGB
  template Result VisionComponent::CompressAndSendImage<u8>(const Vision::ImageBase<u8>& img,
                                                            s32 quality);
  
  template Result VisionComponent::CompressAndSendImage<Vision::PixelRGB>(const Vision::ImageBase<Vision::PixelRGB>& img,
                                                                          s32 quality);
  
  Result VisionComponent::ClearCalibrationImages()
  {
    if(nullptr == _visionSystem || !_visionSystem->IsInitialized())
    {
      PRINT_NAMED_ERROR("VisionComponent.ClearCalibrationImages.VisionSystemNotReady", "");
      return RESULT_FAIL;
    }
    else
    {
      return _visionSystem->ClearCalibrationImages();
    }
  }
  
  size_t VisionComponent::GetNumStoredCameraCalibrationImages() const
  {
    return _visionSystem->GetNumStoredCalibrationImages();
  }
  
  Result VisionComponent::GetCalibrationPoseToRobot(size_t whichPose, Pose3d& p)
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
  
  std::list<std::vector<u8> > VisionComponent::GetCalibrationImageJpegData(u8* dotsFoundMask)
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
      
      /*
      std::string imgFilename = "savedImg_" + std::to_string(imgIdx) + ".jpg";
      FILE* fp = fopen(imgFilename.c_str(), "w");
      fwrite(imgVec.data(), imgVec.size(), 1, fp);
      fclose(fp);
      */
      
      rawJpegData.emplace_back(std::move(imgVec));
      
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
    Result result = _camera.ComputeObjectPose(obsQuad, targetQuad, targetWrtCamera);
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

    if(_camera.IsCalibrated())
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
    
    msg.headAngle = _robot.GetHeadAngle();
    msg.success = true;
    
    if(kDrawDebugDisplay)
    {
      Vision::ImageRGB debugDisp(image);
      for(s32 iDot=0; iDot<4; ++iDot) {
        debugDisp.DrawPoint(Point2f(msg.dotCenX_pix[iDot], msg.dotCenY_pix[iDot]), NamedColors::RED, 3);
        
        Rectangle<f32> roiRect(kExpectedDotCenters_pix[iDot].x()-kSearchSize_pix/2,
                               kExpectedDotCenters_pix[iDot].y()-kSearchSize_pix/2,
                               kSearchSize_pix, kSearchSize_pix);
        debugDisp.DrawRect(roiRect, NamedColors::GREEN, 2);
      }
      debugDisp.Display("FactoryTestFindDots", 0);
    }
    
    return RESULT_OK;
  } // FindFactoryTestDotCentroids()
  
  Result VisionComponent::ClearToolCodeImages()
  {
    if(nullptr == _visionSystem || !_visionSystem->IsInitialized())
    {
      PRINT_NAMED_ERROR("VisionComponent.ClearToolCodeImages.VisionSystemNotReady", "");
      return RESULT_FAIL;
    }
    else
    {
      return _visionSystem->ClearToolCodeImages();
    }
  }
  
  size_t VisionComponent::GetNumStoredToolCodeImages() const
  {
    return _visionSystem->GetNumStoredToolCodeImages();
  }
  
  std::list<std::vector<u8> > VisionComponent::GetToolCodeImageJpegData()
  {
    const auto& images = _visionSystem->GetToolCodeImages();
    
    // Do jpeg compression
    std::list<std::vector<u8> > rawJpegData;
    for (auto const& img : images)
    {
      // Compress to jpeg
      std::vector<u8> imgVec;
      cv::imencode(".jpg", img.get_CvMat_(), imgVec, std::vector<int>({CV_IMWRITE_JPEG_QUALITY, 75}));

      /*
       std::string imgFilename = "savedImg_" + std::to_string(imgIdx) + ".jpg";
       FILE* fp = fopen(imgFilename.c_str(), "w");
       fwrite(imgVec.data(), imgVec.size(), 1, fp);
       fclose(fp);
       */
      
      rawJpegData.emplace_back(std::move(imgVec));
    }
    
    return rawJpegData;
  }
  
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
    
    const u32 maxAlbumSize = _robot.GetNVStorageComponent().GetMaxSizeForEntryTag(NVStorage::NVEntryTag::NVEntry_FaceAlbumData);
    if(_albumData.size() >= maxAlbumSize)
    {
      PRINT_NAMED_ERROR("VisionComponent.SaveFaceAlbumToRobot.AlbumDataTooLarge",
                        "Album data is %zu max size is %u",
                        _albumData.size(),
                        maxAlbumSize);
      return RESULT_FAIL_INVALID_SIZE;
    }
    
    const u32 maxEnrollSize = _robot.GetNVStorageComponent().GetMaxSizeForEntryTag(NVStorage::NVEntryTag::NVEntry_FaceEnrollData);
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
        _robot.BroadcastEngineErrorCode(EngineErrorCode::WriteFacesToRobot);
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
        _robot.BroadcastEngineErrorCode(EngineErrorCode::WriteFacesToRobot);
      }
      
      if(nullptr != enrollCallback) {
        enrollCallback(result);
      }
    };

    
    if(_albumData.empty() && _enrollData.empty()) {
      PRINT_NAMED_INFO("VisionComponent.SaveFaceAlbumToRobot.EmptyAlbumData", "Will erase robot album data");
      // Special case: no face data, so erase what's on the robot so it matches
      bool sendSucceeded = _robot.GetNVStorageComponent().Erase(NVStorage::NVEntryTag::NVEntry_FaceAlbumData, saveAlbumCallback);
      if(!sendSucceeded) {
        PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbumToRobot.SendEraseAlbumDataFail", "");
        lastResult = RESULT_FAIL;
      } else {
        sendSucceeded = _robot.GetNVStorageComponent().Erase(NVStorage::NVEntryTag::NVEntry_FaceEnrollData, saveEnrollCallback);
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
    bool sendSucceeded = _robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_FaceAlbumData,
                                                              _albumData.data(), _albumData.size(), saveAlbumCallback);
    if(!sendSucceeded) {
      PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbumToRobot.SendWriteAlbumDataFail", "");
      lastResult = RESULT_FAIL;
    } else {
      sendSucceeded = _robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_FaceEnrollData,
                                                         _enrollData.data(), _enrollData.size(), saveEnrollCallback);
      if(!sendSucceeded) {
        PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbumToRobot.SendWriteEnrollDataFail", "");
        lastResult = RESULT_FAIL;
      }
    }
    
    PRINT_NAMED_INFO("VisionComponent.SaveFaceAlbumToRobot.Initiated",
                     "Initiated save of %zu-byte album data and %zu-byte enroll data to NVStorage",
                     _albumData.size(), _enrollData.size());
    
    LOG_EVENT("robot.vision.save_face_album_data_size_bytes", "%zu", _albumData.size());
    LOG_EVENT("robot.vision.save_face_enroll_data_size_bytes", "%zu", _enrollData.size());
    
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
    bool sendSucceeded = _robot.GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_FaceAlbumData,
                                                             {}, &_albumData, false);
    
    if(!sendSucceeded) {
      PRINT_NAMED_WARNING("VisionComponent.LoadFaceAlbumFromRobot.ReadFaceAlbumDataFail", "");
      lastResult = RESULT_FAIL;
    } else {
      sendSucceeded = _robot.GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_FaceEnrollData,
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
    Unlock();
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
    Unlock();
    if(RESULT_OK == result) {
      // Update robot
      SaveFaceAlbumToRobot();
      // Send back confirmation
      ExternalInterface::RobotErasedEnrolledFace msg;
      msg.faceID  = faceID;
      msg.name    = "";
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
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
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotErasedAllEnrolledFaces()));
  }
  
  Result VisionComponent::RenameFace(Vision::FaceID_t faceID, const std::string& oldName, const std::string& newName)
  {
    Vision::RobotRenamedEnrolledFace renamedFace;
    Lock();
    Result result = _visionSystem->RenameFace(faceID, oldName, newName, renamedFace);
    Unlock();
    
    if(RESULT_OK == result)
    {
      SaveFaceAlbumToRobot();
      _robot.Broadcast(ExternalInterface::MessageEngineToGame( std::move(renamedFace) ));
    }
    
    return result;
  }
  
  void VisionComponent::BroadcastLoadedNamesAndIDs(const std::list<Vision::LoadedKnownFace>& loadedFaces) const
  {
    // Notify about the newly-available names and IDs, and create wave files
    // for the names if they don't already exist, so we've already got them
    // when we want to say them at some point later.
    using namespace ExternalInterface;
    _robot.Broadcast(MessageEngineToGame(RobotErasedAllEnrolledFaces()));
    for(auto & loadedFace : loadedFaces)
    {
      
      PRINT_CH_INFO("VisionComponent", "VisionComponent.BroadcastLoadedNamesAndIDs", "broadcasting loaded face id: %d",
                    loadedFace.faceID);
      
      _robot.Broadcast(MessageEngineToGame( Vision::LoadedKnownFace(loadedFace) ));
    }
  }
  
  void VisionComponent::HandleDefaultCameraParams(const DefaultCameraParams& params)
  {
    if(!_visionSystem->IsInitialized())
    {
      PRINT_NAMED_ERROR("VisionComponent.HandleDefaultCameraParams.NotInitialized", "");
      return;
    }
  
    if(kInitialExposureTime_ms < params.minExposure_ms ||
       kInitialExposureTime_ms > params.maxExposure_ms)
    {
      PRINT_NAMED_ERROR("VisionComponent.HandleDefaultCameraParams.BadInitialExposureTime",
                        "Initial exp time %ums outside range [%u,%u]",
                        kInitialExposureTime_ms,
                        params.minExposure_ms, params.maxExposure_ms);
      return;
    }
    
    SetCameraSettings(kInitialExposureTime_ms, params.gain);
      
    Lock();
    Result result = _visionSystem->SetCameraExposureParams(kInitialExposureTime_ms,
                                                           Util::numeric_cast<s32>(params.minExposure_ms),
                                                           Util::numeric_cast<s32>(params.maxExposure_ms),
                                                           params.gain,
                                                           kMinCameraGain,
                                                           params.maxGain,
                                                           params.gammaCurve);
    Unlock();
    
    if(RESULT_OK != result)
    {
      PRINT_NAMED_ERROR("VisionComponent.HandleDefaultCameraParams.SetFailed",
                        "Current:%ums Min:%ums Max:%ums",
                        kInitialExposureTime_ms,
                        params.minExposure_ms,
                        params.maxExposure_ms);
      return;
    }
    
  }
  
  void VisionComponent::SetAndDisableAutoExposure(u16 exposure_ms, f32 gain)
  {
    SetCameraSettings(exposure_ms, gain);
    EnableAutoExposure(false);
  }
  
  void VisionComponent::SetCameraSettings(const s32 exposure_ms, const f32 gain)
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
    
    SetCameraParams params(gain,
                           exposure_ms_u16,
                           false);
    
    _robot.SendMessage(RobotInterface::EngineToRobot(std::move(params)));
    _vizManager->SendCameraInfo(exposure_ms_u16, gain);
    
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(
                        ExternalInterface::CurrentCameraParams(gain, exposure_ms_u16, _enableAutoExposure) ));
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
  
  
# ifdef COZMO_V2
  void VisionComponent::CaptureAndSendImage()
  {
    const ImageResolution res = ImageResolution::QVGA;
    const int cameraRes = static_cast<const int>(res);
    const int numRows = Vision::CameraResInfo[cameraRes].height;
    const int numCols = Vision::CameraResInfo[cameraRes].width;
    
    const int bufferSize = numRows * numCols * 3;
    u8 buffer[bufferSize];
    
    // Get image buffer
    // TODO: ImageImuData can be engine-only, non-clad, struct
    std::vector<ImageImuData> imuData;
    u32 imageId = AndroidHAL::getInstance()->CameraGetFrame(buffer, res, imuData);
    
    // Add IMU data to history
    for (auto& data : imuData) {
      GetImuDataHistory().AddImuData(data.imageId , data.rateX, data.rateY, data.rateZ, data.line2Number);
    }
    
    // Create ImageRGB object from image buffer
    cv::Mat cvImg;
    cvImg = cv::Mat(numRows, numCols, CV_8UC3, (void*)buffer);
    cvtColor(cvImg, cvImg, CV_BGR2RGB);
    Vision::ImageRGB imgRGB(numRows, numCols, buffer);
    
    // Create EncodedImage with proper imageID and timestamp
    // ***** TODO: Timestamp needs to be in sync with RobotState timestamp!!! ******
    TimeStamp_t ts = AndroidHAL::getInstance()->GetTimeStamp() - BS_TIME_STEP;
    imgRGB.SetTimestamp(ts);
    EncodedImage encodedImage(imgRGB, imageId);
    
    // Set next image for VisionComponent
    Result lastResult = SetNextImage(encodedImage);
    
    // Compress to jpeg and send to game and viz
    lastResult = CompressAndSendImage(imgRGB, 50);
    DEV_ASSERT(RESULT_OK == lastResult, "CompressAndSendImage() failed");
  }
  
# endif // #ifdef COZMO_V2
  
  
  
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
  void VisionComponent::HandleMessage(const ExternalInterface::SetCameraSettings& payload)
  {
    EnableAutoExposure(payload.enableAutoExposure);
    
    // If we are not enabling auto exposure (we are disabling it) then set the exposure and gain
    if(!payload.enableAutoExposure)
    {
      SetCameraSettings(payload.exposure_ms, payload.gain);
    }
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
            PRINT_NAMED_INFO("VisionComponent.ReadCameraCalibration.Recvd",
                             "Received new %dx%d camera calibration from robot. (fx: %f, fy: %f, cx: %f cy: %f)",
                             payload.ncols, payload.nrows,
                             payload.focalLength_x, payload.focalLength_y,
                             payload.center_x, payload.center_y);
            
            // Convert calibration message into a calibration object to pass to the robot
            Vision::CameraCalibration calib(payload.nrows,
                                            payload.ncols,
                                            payload.focalLength_x,
                                            payload.focalLength_y,
                                            payload.center_x,
                                            payload.center_y,
                                            payload.skew,
                                            payload.distCoeffs);
            
            SetCameraCalibration(calib);
            
            #ifdef COZMO_V2
            {
              // Compute FOV from focal length
              f32 headCamFOV_ver = 2.f * atanf(static_cast<f32>(payload.nrows) / (2.f * payload.focalLength_y));
              f32 headCamFOV_hor = 2.f * atanf(static_cast<f32>(payload.ncols) / (2.f * payload.focalLength_x));
             
              CameraFOVInfo msg(headCamFOV_hor, headCamFOV_ver);
              if (_robot.SendMessage(RobotInterface::EngineToRobot(std::move(msg))) != RESULT_OK) {
                PRINT_NAMED_WARNING("VisionComponent.ReadCameraCalibration.SendCameraFOVFailed", "");
              }
            }
            #endif // ifdef COZMO_V2
            
            
          }
        } else {
          PRINT_NAMED_WARNING("VisionComponent.ReadCameraCalibration.Failed", "");
        }
        
        Enable(true);
      };
      
      _robot.GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_CameraCalib, readCamCalibCallback);
      
      // Request the default camera parameters
      SetCameraParams msg;
      msg.requestDefaultParams = true;
      _robot.SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
    }
  }
  
} // namespace Cozmo
} // namespace Anki
