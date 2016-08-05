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

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotPoseHistory.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/visionSystem.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/visionModesHelpers.h"

#include "anki/vision/basestation/image_impl.h"
#include "anki/vision/basestation/trackedFace.h"
#include "anki/vision/MarkerCodeDefinitions.h"
#include "anki/vision/basestation/observableObjectLibrary_impl.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/robot/config.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "util/fileUtils/fileUtils.h"
#include "anki/common/basestation/utils/helpers/boundedWhile.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/imageTypes.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"

#include "anki/cozmo/basestation/viz/vizManager.h"

#include "opencv2/highgui/highgui.hpp"

namespace Anki {
namespace Cozmo {

  CONSOLE_VAR(f32, kHeadTurnSpeedThreshBlock_degs, "WasMovingTooFast.Block.Head_deg/s",   10.f);
  CONSOLE_VAR(f32, kBodyTurnSpeedThreshBlock_degs, "WasMovingTooFast.Block.Body_deg/s",   30.f);
  CONSOLE_VAR(f32, kHeadTurnSpeedThreshFace_degs,  "WasMovingTooFast.Face.Head_deg/s",    10.f);
  CONSOLE_VAR(f32, kBodyTurnSpeedThreshFace_degs,  "WasMovingTooFast.Face.Body_deg/s",    30.f);
  CONSOLE_VAR(u8,  kNumImuDataToLookBack,          "WasMovingTooFast.Face.NumToLookBack", 5);
  
  // Whether or not to do rolling shutter correction for physical robots
  CONSOLE_VAR(bool, kRollingShutterCorrectionEnabled, "Vision.PreProcessing", true);
  
  // Amount of time we sleep when paused, waiting for next image, and after processing each image
  // (in order to provide a little breathing room for main thread)
  CONSOLE_VAR_RANGED(u8, kVision_MinSleepTime_ms, "Vision.General", 2, 1, 10);
  
  VisionComponent::VisionComponent(Robot& robot, RunMode mode, const CozmoContext* context)
  : _robot(robot)
  , _context(context)
  , _vizManager(context->GetVizManager())
  , _camera(robot.GetID())
  , _runMode(mode)
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
      helper.SubscribeGameToEngine<MessageGameToEngineTag::EraseEnrolledFaceByName>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::LoadFaceAlbumFromFile>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SaveFaceAlbumToFile>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SetFaceEnrollmentPose>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::UpdateEnrolledFaceByID>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::VisionRunMode>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::VisionWhileMoving>();

    }
    
  } // VisionSystem()

  Result VisionComponent::Init(const Json::Value& config)
  {
    _isInitialized = false;
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
      
      std::list<Vision::FaceNameAndID> namesAndIDs;
      result = _visionSystem->LoadFaceAlbum(faceAlbumName, namesAndIDs);
      BroadcastLoadedNamesAndIDs(namesAndIDs);
      
      if(RESULT_OK != result) {
        PRINT_NAMED_WARNING("VisionComponent.Init.LoadFaceAlbumFromFileFailed",
                            "AlbumFile: %s", faceAlbumName.c_str());
      }
    }
    
    _dropStats.SetChannelName("VisionComponent");
    
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
     
      if(_runMode == RunMode::Asynchronous) {
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
  
  
  void VisionComponent::SetRunMode(RunMode mode) {
    if(mode == RunMode::Synchronous && _runMode == RunMode::Asynchronous) {
      PRINT_NAMED_INFO("VisionComponent.SetRunMode.SwitchToSync", "");
      if(_running) {
        Stop();
      }
      _runMode = mode;
    }
    else if(mode == RunMode::Asynchronous && _runMode == RunMode::Synchronous) {
      PRINT_NAMED_INFO("VisionComponent.SetRunMode.SwitchToAsync", "");
      _runMode = mode;
      Start();
    }
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
                       "Thread already started, call Stop() and then restarting.");
      Stop();
    } else {
      PRINT_NAMED_INFO("VisionComponent.Start",
                       "Starting vision processing thread.");
    }
    
    _running = true;
    _paused = false;
    
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
                        "Cannot set vision marker to track before vision system is instantiated.\n");
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
      return _visionSystem->EnableMode(mode, enable);
    } else {
      PRINT_NAMED_ERROR("VisionComponent.EnableMode.NullVisionSystem", "");
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
  
  
  Result VisionComponent::SetNextImage(EncodedImage& encodedImage)
  {
    if(!_isInitialized) {
      PRINT_NAMED_WARNING("VisionComponent.SetNextImage.NotInitialized", "t=%u(%d)",
                          encodedImage.GetTimeStamp(), encodedImage.GetTimeStamp());
      return RESULT_FAIL;
    }
    
    if (!_enabled) {
      PRINT_CH_INFO("VisionComponent", "VisionComponent.SetNextImage", "Set next image but not enabled, t=%u(%d)",
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
        // so reset the lastReceived timestamp so we can set it fresh next time we get an image
        _lastReceivedImageTimeStamp_ms = 0;
        return RESULT_FAIL;
      }
      _framePeriod_ms = encodedImage.GetTimeStamp() - _lastReceivedImageTimeStamp_ms;
    }
    _lastReceivedImageTimeStamp_ms = encodedImage.GetTimeStamp();
    
    if(_isCamCalibSet) {
      ASSERT_NAMED(nullptr != _visionSystem, "VisionComponent.SetNextImage.NullVisionSystem");
      ASSERT_NAMED(_visionSystem->IsInitialized(), "VisionComponent.SetNextImage.VisionSystemNotInitialized");

      // Populate IMU data history for this image, for rolling shutter correction
      GetImuDataHistory().CalculateTimestampForImageIMU(encodedImage.GetImageID(),
                                                        encodedImage.GetTimeStamp(),
                                                        RollingShutterCorrector::timeBetweenFrames_ms,
                                                        encodedImage.GetHeight());
    
      // Fill in the pose data for the given image, by querying robot history
      RobotPoseStamp imagePoseStamp;
      TimeStamp_t imagePoseStampTimeStamp;
      
      Result lastResult = _robot.GetPoseHistory()->ComputePoseAt(encodedImage.GetTimeStamp(), imagePoseStampTimeStamp, imagePoseStamp, true);
      
      // If we are unable to compute a pose at image timestamp because we haven't recieved any state updates
      // after that timestamp then use the latest pose
      // This is due to the offset added to image timestamps allowing us to potentially get an image in the
      // future relative to our last state update. This should go away when the correct offset is calculated
      if(lastResult != RESULT_OK)
      {
        lastResult = _robot.GetPoseHistory()->GetRawPoseAt(_robot.GetPoseHistory()->GetNewestTimeStamp(), imagePoseStampTimeStamp, imagePoseStamp, false);
      }

      if(lastResult != RESULT_OK) {
        PRINT_NAMED_WARNING("VisionComponent.SetNextImage.PoseHistoryFail",
                          "Unable to get computed pose at image timestamp of %d. (rawPoses: have %zu from %d:%d) (visionPoses: have %zu from %d:%d)\n",
                          encodedImage.GetTimeStamp(),
                          _robot.GetPoseHistory()->GetNumRawPoses(),
                          _robot.GetPoseHistory()->GetOldestTimeStamp(),
                          _robot.GetPoseHistory()->GetNewestTimeStamp(),
                          _robot.GetPoseHistory()->GetNumVisionPoses(),
                          _robot.GetPoseHistory()->GetOldestVisionOnlyTimeStamp(),
                          _robot.GetPoseHistory()->GetNewestVisionOnlyTimeStamp());
        return lastResult;
      }
      
      // Get most recent pose data in history
      Anki::Cozmo::RobotPoseStamp lastPoseStamp;
      _robot.GetPoseHistory()->GetLastPoseWithFrameID(_robot.GetPoseFrameID(), lastPoseStamp);
      
      // Compare most recent pose and pose at time of image to see if robot has moved in the short time
      // time since the image was taken. If it has, this suppresses motion detection inside VisionSystem.
      // This is necessary because the image might contain motion, but according to the pose there was none.
      // Whether this is due to inaccurate timestamping of images or low-resolution pose reporting from
      // the robot, this additional info allows us to know if motion in the image was likely due to actual
      // robot motion.
      // NOTE: if the two pose stamps are not in the same coordinate frame, just assume there
      //       was motion to be conservative
      const bool inSameFrame = &lastPoseStamp.GetPose().FindOrigin() == &imagePoseStamp.GetPose().FindOrigin();
      const bool headSame = inSameFrame && NEAR(lastPoseStamp.GetHeadAngle(),
                                                imagePoseStamp.GetHeadAngle(), DEG_TO_RAD_F32(0.1));
      const bool liftSame = inSameFrame && NEAR(lastPoseStamp.GetLiftAngle(),
                                                imagePoseStamp.GetLiftAngle(), DEG_TO_RAD_F32(0.1));
      
      const bool poseSame = (inSameFrame &&
                             NEAR(lastPoseStamp.GetPose().GetTranslation().x(),
                                  imagePoseStamp.GetPose().GetTranslation().x(), .5f) &&
                             NEAR(lastPoseStamp.GetPose().GetTranslation().y(),
                                  imagePoseStamp.GetPose().GetTranslation().y(), .5f) &&
                             NEAR(lastPoseStamp.GetPose().GetRotation().GetAngleAroundZaxis().ToFloat(),
                                  imagePoseStamp.GetPose().GetRotation().GetAngleAroundZaxis().ToFloat(),
                                  DEG_TO_RAD_F32(0.1)));
      
      Lock();
      _nextPoseData.poseStamp = imagePoseStamp;
      _nextPoseData.timeStamp = imagePoseStampTimeStamp;
      _nextPoseData.isBodyMoving = !poseSame;
      _nextPoseData.isHeadMoving = !headSame;
      _nextPoseData.isLiftMoving = !liftSame;
      _nextPoseData.cameraPose = _robot.GetHistoricalCameraPose(_nextPoseData.poseStamp, _nextPoseData.timeStamp);
      _nextPoseData.groundPlaneVisible = LookupGroundPlaneHomography(_nextPoseData.poseStamp.GetHeadAngle(),
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
        if(!WasMovingTooFast(encodedImage.GetTimeStamp(), DEG_TO_RAD(0.1), DEG_TO_RAD(0.1), 3))
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
      

      switch(_runMode)
      {
        case RunMode::Synchronous:
        {
          if(!_paused) {
            UpdateVisionSystem(_nextPoseData, encodedImage);
          }
          break;
        }
        case RunMode::Asynchronous:
        {
          if(!_paused) {
            ANKI_CPU_PROFILE("VC::SetNextImage.LockedSwap");
            Lock();

            const bool isDroppingFrame = !_nextImg.IsEmpty();
            if(isDroppingFrame)
            {
              PRINT_CH_INFO("VisionComponent", "SetNextImage.DroppedFrame",
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
          break;
        }
        default:
          PRINT_NAMED_ERROR("VisionComponent.SetNextImage.InvalidRunMode", "");
      } // switch(_runMode)
      
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
    
    ASSERT_NAMED(_camera.IsCalibrated(), "VisionComponent.PopulateGroundPlaneHomographyLUT.CameraNotCalibrated");
    
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
        PRINT_CH_INFO("VisionComponent", "PopulateGroundPlaneHomographyLUT.MaxHeadAngleReached",
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
    
    ASSERT_NAMED(_visionSystem != nullptr && _visionSystem->IsInitialized(),
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
  
  
  Result VisionComponent::QueueObservedMarker(const Vision::ObservedMarker& markerOrig)
  {
    Result lastResult = RESULT_OK;
    
    // Get historical robot pose at specified timestamp to get
    // head angle and to attach as parent of the camera pose.
    TimeStamp_t t;
    RobotPoseStamp* p = nullptr;
    HistPoseKey poseKey;

    lastResult = _robot.GetPoseHistory()->ComputeAndInsertPoseAt(markerOrig.GetTimeStamp(), t, &p, &poseKey, true);

    if(lastResult != RESULT_OK) {
      PRINT_NAMED_WARNING("VisionComponent.QueueObservedMarker.HistoricalPoseNotFound",
                          "Time: %u, hist: %u to %u",
                          markerOrig.GetTimeStamp(),
                          _robot.GetPoseHistory()->GetOldestTimeStamp(),
                          _robot.GetPoseHistory()->GetNewestTimeStamp());
      return lastResult;
    }
    
    if(&p->GetPose().FindOrigin() != _robot.GetWorldOrigin()) {
      PRINT_NAMED_WARNING("VisionComponent.QueueObservedMarker.OldOrigin",
                          "Ignoring observed marker from origin %s (robot origin is %s)",
                          p->GetPose().FindOrigin().GetName().c_str(),
                          _robot.GetWorldOrigin()->GetName().c_str());
      return RESULT_OK;
    }
    
    // If we get here, ComputeAndInsertPoseIntoHistory() should have succeeded
    // and this should be true
    assert(markerOrig.GetTimeStamp() == t);
    
    // If we were moving too fast at timestamp t and we aren't doing rolling shutter correction
    // then don't queue this marker otherwise don't queue if only the head was moving too fast
    if(WasMovingTooFast(t, DEG_TO_RAD_F32(kBodyTurnSpeedThreshBlock_degs), DEG_TO_RAD_F32(kHeadTurnSpeedThreshBlock_degs)))
    {
      return RESULT_OK;
    }
    
    // Update the marker's camera to use a pose from pose history, and
    // create a new marker with the updated camera
    assert(nullptr != p);
    Vision::ObservedMarker marker(markerOrig.GetTimeStamp(), markerOrig.GetCode(),
                                  markerOrig.GetImageCorners(),
                                  _robot.GetHistoricalCamera(*p, markerOrig.GetTimeStamp()),
                                  markerOrig.GetUserHandle());
    
    // Queue the marker for processing by the blockWorld
    _robot.GetBlockWorld().QueueObservedMarker(poseKey, marker);
    
    /*
    // React to the marker if there is a callback for it
    auto reactionIter = _reactionCallbacks.find(marker.GetCode());
    if(reactionIter != _reactionCallbacks.end()) {
      // Run each reaction for this code, in order:
      for(auto & reactionCallback : reactionIter->second) {
        lastResult = reactionCallback(this, &marker);
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_WARNING("Robot.Update.ReactionCallbackFailed",
                              "Reaction callback failed for robot %d observing marker with code %d.\n",
                              robot.GetID(), marker.GetCode());
        }
      }
    }
     */
    
    // Visualize the marker in 3D
    // TODO: disable this block when not debugging / visualizing
    if(true){
      
      // Note that this incurs extra computation to compute the 3D pose of
      // each observed marker so that we can draw in the 3D world, but this is
      // purely for debug / visualization
      u32 quadID = 0;
      
      // When requesting the markers' 3D corners below, we want them
      // not to be relative to the object the marker is part of, so we
      // will request them at a "canonical" pose (no rotation/translation)
      const Pose3d canonicalPose;
      
      
      // Block Markers
      std::set<const ObservableObject*> const& blocks = _robot.GetBlockWorld().GetObjectLibrary(ObjectFamily::Block).GetObjectsWithMarker(marker);
      for(auto block : blocks) {
        std::vector<Vision::KnownMarker*> const& blockMarkers = block->GetMarkersWithCode(marker.GetCode());
        
        for(auto blockMarker : blockMarkers) {
          
          Pose3d markerPose;
          Result poseResult = marker.GetSeenBy().ComputeObjectPose(marker.GetImageCorners(),
                                                                   blockMarker->Get3dCorners(canonicalPose),
                                                                   markerPose);
          if(poseResult != RESULT_OK) {
            PRINT_NAMED_WARNING("BlockWorld.QueueObservedMarker",
                                "Could not estimate pose of block marker. Not visualizing.\n");
          } else {
            if(markerPose.GetWithRespectTo(marker.GetSeenBy().GetPose().FindOrigin(), markerPose) == true) {
              _robot.GetContext()->GetVizManager()->DrawGenericQuad(quadID++, blockMarker->Get3dCorners(markerPose), NamedColors::OBSERVED_QUAD);
            } else {
              PRINT_NAMED_WARNING("BlockWorld.QueueObservedMarker.MarkerOriginNotCameraOrigin",
                                  "Cannot visualize a Block marker whose pose origin is not the camera's origin that saw it.\n");
            }
          }
        }
      }
      
      
      // Mat Markers
      std::set<const ObservableObject*> const& mats = _robot.GetBlockWorld().GetObjectLibrary(ObjectFamily::Mat).GetObjectsWithMarker(marker);
      for(auto mat : mats) {
        std::vector<Vision::KnownMarker*> const& matMarkers = mat->GetMarkersWithCode(marker.GetCode());
        
        for(auto matMarker : matMarkers) {
          Pose3d markerPose;
          Result poseResult = marker.GetSeenBy().ComputeObjectPose(marker.GetImageCorners(),
                                                                   matMarker->Get3dCorners(canonicalPose),
                                                                   markerPose);
          if(poseResult != RESULT_OK) {
            PRINT_NAMED_WARNING("BlockWorld.QueueObservedMarker",
                                "Could not estimate pose of mat marker. Not visualizing.\n");
          } else {
            if(markerPose.GetWithRespectTo(marker.GetSeenBy().GetPose().FindOrigin(), markerPose) == true) {
              _robot.GetContext()->GetVizManager()->DrawMatMarker(quadID++, matMarker->Get3dCorners(markerPose), NamedColors::RED);
            } else {
              PRINT_NAMED_WARNING("BlockWorld.QueueObservedMarker.MarkerOriginNotCameraOrigin",
                                  "Cannot visualize a Mat marker whose pose origin is not the camera's origin that saw it.\n");
            }
          }
        }
      }
      
    } // 3D marker visualization
    
    return lastResult;
    
  } // QueueObservedMarker()
  
  Result VisionComponent::UpdateAllResults(VisionProcessingResult& procResult_out)
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
            PRINT_NAMED_ERROR("VisionComponent.UpdateAllResults", "%s Failed", EnumToString(mode));
            anyFailures = true;
          }
        };
        
        tryAndReport(&VisionComponent::UpdateVisionMarkers,       VisionMode::DetectingMarkers);
        tryAndReport(&VisionComponent::UpdateFaces,               VisionMode::DetectingFaces);
        
        // Special handling for the two types of processing that are enabled by VisionMode::Tracking
        if (result.modesProcessed.IsBitFlagSet(VisionMode::Tracking))
        {
          if (RESULT_OK != VisionComponent::UpdateTrackingQuad(result))
          {
            PRINT_NAMED_ERROR("VisionComponent.UpdateAllResults", "UpdateTrackingQuad Failed");
            anyFailures = true;
          }
          
          if (RESULT_OK != VisionComponent::UpdateDockingErrorSignal(result))
          {
            PRINT_NAMED_ERROR("VisionComponent.UpdateAllResults", "UpdateDockingErrorSignal Failed");
            anyFailures = true;
          }
        }
        
        tryAndReport(&VisionComponent::UpdateMotionCentroid,      VisionMode::DetectingMotion);
        tryAndReport(&VisionComponent::UpdateOverheadEdges,       VisionMode::DetectingOverheadEdges);
        tryAndReport(&VisionComponent::UpdateToolCode,            VisionMode::ReadingToolCode);
        tryAndReport(&VisionComponent::UpdateComputedCalibration, VisionMode::ComputingCalibration);
        
        // Display any debug images left by the vision system
        for(auto & debugGray : result.debugImages) {
          debugGray.second.Display(debugGray.first.c_str());
        }
        for(auto & debugRGB : result.debugImageRGBs) {
          debugRGB.second.Display(debugRGB.first.c_str());
        }
        
        // Store frame rate and last image processed time
        ASSERT_NAMED(result.timestamp >= _lastProcessedImageTimeStamp_ms,
                     "VisionComponent.UpdateAllResults.BadTimeStamp"); // Time should only move forward
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
        
        procResult_out = result;
        
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
    
    for(auto & visionMarker : procResult.observedMarkers)
    {
      lastResult = QueueObservedMarker(visionMarker);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_ERROR("VisionComponent.Update.FailedToQueueVisionMarker",
                          "Got VisionMarker message from vision processing thread but failed to queue it.");
        return lastResult;
      }
      
      const Quad2f& corners = visionMarker.GetImageCorners();
      const ColorRGBA& drawColor = (visionMarker.GetCode() == Vision::MARKER_UNKNOWN ?
                                    NamedColors::BLUE : NamedColors::RED);
      _vizManager->DrawCameraQuad(corners, drawColor, NamedColors::GREEN);
      
      const bool drawMarkerNames = false;
      if(drawMarkerNames)
      {
        Rectangle<f32> boundingRect(corners);
        std::string markerName(visionMarker.GetCodeName());
        _vizManager->DrawCameraText(boundingRect.GetTopLeft(),
                                    markerName.substr(strlen("MARKER_"),std::string::npos),
                                    drawColor);
      }
    }

    return lastResult;
  } // UpdateVisionMarkers()
  
  Result VisionComponent::UpdateFaces(const VisionProcessingResult& procResult)
  {
    Result lastResult = RESULT_OK;

    for(auto & updatedID : procResult.updatedFaceIDs)
    {
      _robot.GetFaceWorld().ChangeFaceID(updatedID.oldID, updatedID.newID);
    }

    for(auto faceDetection : procResult.faces)
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
      
      // Get historical robot pose at specified timestamp to get
      // head angle and to attach as parent of the camera pose.
      TimeStamp_t t;
      RobotPoseStamp* p = nullptr;
      HistPoseKey poseKey;

      _robot.GetPoseHistory()->ComputeAndInsertPoseAt(faceDetection.GetTimeStamp(), t, &p, &poseKey, true);
      
      // Check this before potentially ignoring the face detection for faceWorld's purposes below
      if(faceDetection.GetNumEnrollments() > 0) {
        PRINT_NAMED_DEBUG("VisionComponent.UpdateFaces.ReachedEnrollmentCount",
                          "Count=%d", faceDetection.GetNumEnrollments());
        ExternalInterface::RobotReachedEnrollmentCount enrollCountMsg;
        enrollCountMsg.faceID = faceDetection.GetID();
        enrollCountMsg.count  = faceDetection.GetNumEnrollments();
        
        _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(enrollCountMsg)));
      }
      
      // If we were moving too fast at the timestamp the face was detected then don't update it
      // If the detected face is being tracked than we should look farther back in imu data history
      // else we will just look at the previous and next imu data
      if(WasMovingTooFast(faceDetection.GetTimeStamp(),
                          DEG_TO_RAD_F32(kBodyTurnSpeedThreshFace_degs),
                          DEG_TO_RAD_F32(kHeadTurnSpeedThreshFace_degs),
                          (faceDetection.IsBeingTracked() ? kNumImuDataToLookBack : 0)))
      {
        return RESULT_OK;
      }
      
      // Use the faceDetection to update FaceWorld:
      lastResult = _robot.GetFaceWorld().AddOrUpdateFace(faceDetection);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_ERROR("VisionComponent.Update.FailedToUpdateFace",
                          "Got FaceDetection from vision processing but failed to update it.");
        return lastResult;
      }
      
    }
    
    return lastResult;
  } // UpdateFaces()
  
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
      Vision::Camera histCamera(_robot.GetHistoricalCamera(procResult.timestamp));
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

      Pose3d worldPoseWrtRobot = poseData.poseStamp.GetPose().GetInverse();
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
              Point3f mapPoint = poseData.poseStamp.GetPose() * Point3f(x,y,0.f);
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
        poseData.poseStamp.GetPose().ApplyTo(roi.GetGroundQuad(), lastUpdate);
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
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
    }
  
    return RESULT_OK;
  }

  bool VisionComponent::WasHeadMovingTooFast(TimeStamp_t t,
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
        PRINT_NAMED_WARNING("VisionComponent.WasHeadMovingTooFast",
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
  
  bool VisionComponent::WasBodyMovingTooFast(TimeStamp_t t,
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
        PRINT_NAMED_WARNING("VisionComponent.WasBodyMovingTooFast",
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
  
  bool VisionComponent::WasMovingTooFast(TimeStamp_t t,
                                         const f32 bodyTurnSpeedLimit_radPerSec,
                                         const f32 headTurnSpeedLimit_radPerSec,
                                         const int numImuDataToLookBack)
  {
    return (WasHeadMovingTooFast(t, headTurnSpeedLimit_radPerSec, numImuDataToLookBack) ||
            WasBodyMovingTooFast(t, bodyTurnSpeedLimit_radPerSec, numImuDataToLookBack));
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
                        "Unrecognized resolution: %dx%d.\n", captureWidth, captureHeight);
      return result;
    }
    
    static u32 imgID = 0;
    const std::vector<int> compressionParams = {
      CV_IMWRITE_JPEG_QUALITY, quality
    };
    
    cv::cvtColor(img.get_CvMat_(), img.get_CvMat_(), CV_BGR2RGB);
    
    std::vector<u8> compressedBuffer;
    //cv::imencode(".jpg",  img.get_CvMat_(), compressedBuffer, compressionParams);
    
    const u32 numTotalBytes = static_cast<u32>(compressedBuffer.size());
    
    //PRINT("Sending frame with capture time = %d at time = %d\n", captureTime, HAL::GetTimeStamp());
    
    m.frameTimeStamp = img.GetTimestamp();
    m.imageId = ++imgID;
    m.chunkId = 0;
    m.imageChunkCount = ceilf((f32)numTotalBytes / (f32)ImageConstants::IMAGE_CHUNK_SIZE);
    if(img.GetNumChannels() == 1) {
      m.imageEncoding = ImageEncoding::JPEGGray;
    } else {
      m.imageEncoding = ImageEncoding::JPEGColor;
    }
    m.data.reserve((size_t)ImageConstants::IMAGE_CHUNK_SIZE);
    
    u32 totalByteCnt = 0;
    u32 chunkByteCnt = 0;
    
    for(s32 i=0; i<numTotalBytes; ++i)
    {
      m.data.push_back(compressedBuffer[i]);
      
      ++chunkByteCnt;
      ++totalByteCnt;
      
      if (chunkByteCnt == (s32)ImageConstants::IMAGE_CHUNK_SIZE) {
        //PRINT("Sending image chunk %d\n", m.chunkId);
        _robot.GetContext()->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ImageChunk(m)));
        ++m.chunkId;
        chunkByteCnt = 0;
      } else if (totalByteCnt == numTotalBytes) {
        // This should be the last message!
        //PRINT("Sending LAST image chunk %d\n", m.chunkId);
        _robot.GetContext()->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ImageChunk(m)));
      }
    } // for each byte in the compressed buffer
    
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
  
  Result VisionComponent::WriteCalibrationPoseToRobot(size_t whichPose,
                                                      NVStorageComponent::NVStorageWriteEraseCallback callback)
  {
    Result lastResult = RESULT_OK;
    
    auto & calibPoses = _visionSystem->GetCalibrationPoses();
    if(whichPose >= calibPoses.size()) {
      PRINT_NAMED_WARNING("VisionComponent.WriteCalibrationPoseToRobot.InvalidPoseIndex",
                          "Requested %zu, only %zu available", whichPose, calibPoses.size());
      lastResult = RESULT_FAIL;
    } else {
      auto & calibImages = _visionSystem->GetCalibrationImages();
      ASSERT_NAMED_EVENT(calibImages.size() >= calibPoses.size(),
                         "VisionComponent.WriteCalibrationPoseToRobot.SizeMismatch",
                         "Expecting at least %zu calibration images, got %zu",
                         calibPoses.size(), calibImages.size());
      
      f32 poseData[6] = {0,0,0,0,0,0};
      
      if(!calibImages[whichPose].dotsFound) {
        PRINT_NAMED_INFO("VisionComponent.WriteCalibrationPoseToRobot.PoseNotComputed",
                         "Dots not found in image %zu, no pose available",
                         whichPose);
      } else {
        // Serialize the requested calibration pose
        const Pose3d& calibPose = calibPoses[whichPose];
        Radians angleX, angleY, angleZ;
        calibPose.GetRotationMatrix().GetEulerAngles(angleX, angleY, angleZ);
        poseData[0] = angleX.ToFloat();
        poseData[1] = angleY.ToFloat();
        poseData[2] = angleZ.ToFloat();
        poseData[3] = calibPose.GetTranslation().x();
        poseData[4] = calibPose.GetTranslation().y();
        poseData[5] = calibPose.GetTranslation().z();
      }
      
      // Write to robot
      const bool success = _robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_CalibPose,
                                                                (u8*)poseData, sizeof(poseData), callback);
                                                        
      lastResult = (success ? RESULT_OK : RESULT_FAIL);
    }
    
    
    return lastResult;
  }
  
  Result VisionComponent::WriteCalibrationImagesToRobot(WriteImagesToRobotCallback callback)
  {
    const auto& calibImages = _visionSystem->GetCalibrationImages();
    
    // Make sure there is no more than 5 images in the list
    if (calibImages.size() > 6 || calibImages.size() < 4) {
      PRINT_NAMED_INFO("VisionComponent.WriteCalibrationImagesToRobot.TooManyOrTooFewImages",
                       "%zu images (Need 4 or 5)", _visionSystem->GetNumStoredCalibrationImages());
      return RESULT_FAIL;
    }

    PRINT_NAMED_INFO("VisionComponent.WriteCalibrationImagesToRobot.StartingWrite",
                     "%zu images", _visionSystem->GetNumStoredCalibrationImages());
    _writeCalibImagesToRobotResults.clear();

    static const NVStorage::NVEntryTag calibImageTags[6] = {NVStorage::NVEntryTag::NVEntry_CalibImage1,
                                                            NVStorage::NVEntryTag::NVEntry_CalibImage2,
                                                            NVStorage::NVEntryTag::NVEntry_CalibImage3,
                                                            NVStorage::NVEntryTag::NVEntry_CalibImage4,
                                                            NVStorage::NVEntryTag::NVEntry_CalibImage5,
                                                            NVStorage::NVEntryTag::NVEntry_CalibImage6};
    
    // Write images to robot
    u32 imgIdx = 0;
    u8 usedMask = 0;
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
      
      // Write images to robot
      PRINT_NAMED_DEBUG("VisionComponent.WriteCalibrationImagesToFile.RequestingWrite", "Image %d", imgIdx);
      bool res = true;
      if (imgIdx < calibImages.size() - 1) {
        res = _robot.GetNVStorageComponent().Write(calibImageTags[imgIdx], &imgVec,
                                                   [this](NVStorage::NVResult res) {
                                                    _writeCalibImagesToRobotResults.push_back(res);
                                                  });
      } else {
        res = _robot.GetNVStorageComponent().Write(calibImageTags[imgIdx], &imgVec,
                                                   [this, callback](NVStorage::NVResult res) {
                                                     _writeCalibImagesToRobotResults.push_back(res);
                                               
                                                     std::string resStr = "";
                                                     for(auto r : _writeCalibImagesToRobotResults) {
                                                       resStr += EnumToString(r) + std::string(", ");
                                                     }
                                                     PRINT_NAMED_DEBUG("VisionComponent.WriteCalibrationImagesToFile.Complete", "%s", resStr.c_str());
                                               
                                                     if (callback) {
                                                       callback(_writeCalibImagesToRobotResults);
                                                     }
                                                   });
      }
      
      if (!res) {
        return RESULT_FAIL;
      }
      
      ++imgIdx;
    }
    
    // Erase remaining calib image tags
    for (u32 i = imgIdx; i < 5; ++i) {
        PRINT_NAMED_DEBUG("VisionComponent.WriteCalibrationImagesToFile.RequestingErase", "Image %d", i);
      _robot.GetNVStorageComponent().Erase(calibImageTags[i]);
    }
    
    // Write bit flag indicating in which images dots were found
    // TODO: Add callback?
    bool res = _robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_CalibImagesUsed, &usedMask, 1);


    return (res == true ? RESULT_OK : RESULT_FAIL);
    
    
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
    static const f32 kDotArea_pix = (f32)(kDotDiameter_pix*kDotDiameter_pix) * 0.25f * PI_F;
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
  
  Result VisionComponent::WriteToolCodeImagesToRobot(WriteImagesToRobotCallback callback)
  {
    const auto& images = _visionSystem->GetToolCodeImages();
    
    // Make sure there is no more than 2 images in the list
    if (images.size() != 2) {
      PRINT_NAMED_INFO("VisionComponent.WriteToolCodeImagesToRobot.TooManyOrTooFewImages",
                       "%zu images (Need exactly 2 images)", _visionSystem->GetNumStoredToolCodeImages());
      return RESULT_FAIL;
    }
    
    PRINT_NAMED_INFO("VisionComponent.WriteToolCodeImagesToRobot.StartingWrite",
                     "%zu images", _visionSystem->GetNumStoredToolCodeImages());
    _writeToolCodeImagesToRobotResults.clear();
    
    
    static const NVStorage::NVEntryTag toolCodeImageTags[2] = {NVStorage::NVEntryTag::NVEntry_ToolCodeImageLeft, NVStorage::NVEntryTag::NVEntry_ToolCodeImageRight};
    
    // Write images to robot
    u32 imgIdx = 0;
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
      
      
      // Write images to robot
      PRINT_NAMED_DEBUG("VisionComponent.WriteToolCodeImagesToFile.RequestingWrite", "Image %d", imgIdx);
      bool res = true;
      if (imgIdx < images.size() - 1) {
        res = _robot.GetNVStorageComponent().Write(toolCodeImageTags[imgIdx], &imgVec,
                                                   [this](NVStorage::NVResult res) {
                                                     _writeToolCodeImagesToRobotResults.push_back(res);
                                                   });
      } else {
        res = _robot.GetNVStorageComponent().Write(toolCodeImageTags[imgIdx], &imgVec,
                                                   [this, callback](NVStorage::NVResult res) {
                                                     _writeToolCodeImagesToRobotResults.push_back(res);
                                                     
                                                     std::string resStr = "";
                                                     for(auto r : _writeToolCodeImagesToRobotResults) {
                                                       resStr += EnumToString(r) + std::string(", ");
                                                     }
                                                     PRINT_NAMED_DEBUG("VisionComponent.WriteToolCodeImagesToFile.Complete", "%s", resStr.c_str());
                                                     
                                                     if (callback) {
                                                       callback(_writeToolCodeImagesToRobotResults);
                                                     }
                                                   });
      }
      
      if (!res) {
        return RESULT_FAIL;
      }
      
      ++imgIdx;
    }
    
    return RESULT_OK;
  }
  
  inline static size_t GetPaddedNumBytes(size_t numBytes)
  {
    // Pad to a multiple of 4 for NVStorage
    const size_t paddedNumBytes = (numBytes + 3) & ~0x03;
    ASSERT_NAMED(paddedNumBytes % 4 == 0, "EnrolledFaceEntry.Serialize.PaddedSizeNotMultipleOf4");
    
    return paddedNumBytes;
  }
  
  Result VisionComponent::SaveFaceAlbumToRobot()
  {
    Result lastResult = RESULT_OK;
    
    _albumData.clear();
    _enrollData.clear();
    
    // Get album data from vision system
    this->Lock();
    _visionSystem->GetSerializedFaceData(_albumData, _enrollData);
    this->Unlock();

    // Callback to display result when NVStorage.Write finishes
    NVStorageComponent::NVStorageWriteEraseCallback saveAlbumCallback = [this](NVStorage::NVResult result) {
      if(result == NVStorage::NVResult::NV_OKAY) {
        PRINT_NAMED_INFO("VisionComponent.SaveFaceAlbumToRobot.AlbumSuccess",
                         "Successfully completed saving album data to robot");
      } else {
        PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbumToRobot.AlbumFailure",
                            "Failed saving album data to robot: %s",
                            EnumToString(result));
        _robot.BroadcastEngineErrorCode(EngineErrorCode::WriteFacesToRobot);
      }
    };
    // Callback to display result when NVStorage.Write finishes
    NVStorageComponent::NVStorageWriteEraseCallback saveEnrollCallback = [this](NVStorage::NVResult result) {
      if(result == NVStorage::NVResult::NV_OKAY) {
        PRINT_NAMED_INFO("VisionComponent.SaveFaceAlbumToRobot.EnrollSuccess",
                         "Successfully completed saving enroll data to robot");
      } else {
        PRINT_NAMED_WARNING("VisionComponent.SaveFaceAlbumToRobot.EnrollFailure",
                            "Failed saving enroll data to robot: %s",
                            EnumToString(result));
        _robot.BroadcastEngineErrorCode(EngineErrorCode::WriteFacesToRobot);
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
    ASSERT_NAMED(!_albumData.empty() && !_enrollData.empty(),
                 "VisionComponent.SaveFaceAblumToRobot.BadAlbumOrEnrollData");
    
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
    
    return lastResult;
  } // SaveFaceAlbumToRobot()
  
  Result VisionComponent::LoadFaceAlbumFromRobot()
  {
    Result lastResult = RESULT_OK;
    
    ASSERT_NAMED(_visionSystem != nullptr,
                 "VisionComponent.LoadFaceAlbumFromRobot.VisionSystemNotReady");
    
    NVStorageComponent::NVStorageReadCallback readCallback =
    [this](u8* data, size_t size, NVStorage::NVResult result)
    {
      if(result == NVStorage::NVResult::NV_OKAY)
      {
        // Read completed: try to use the data to update the face album/enroll data
        Lock();
        std::list<Vision::FaceNameAndID> namesAndIDs;
        Result setResult = _visionSystem->SetSerializedFaceData(_albumData, _enrollData, namesAndIDs);
        Unlock();
        
        if(RESULT_OK == setResult) {
          PRINT_NAMED_INFO("VisionComponent.LoadFaceAlbumFromRobot.Success",
                           "Finished setting %zu-byte album data and %zu-byte enroll data",
                           _albumData.size(), _enrollData.size());

          BroadcastLoadedNamesAndIDs(namesAndIDs);
          
        } else {
          PRINT_NAMED_WARNING("VisionComponent.LoadFaceAlbumFromRobot.Failure",
                              "Failed setting %zu-byte album data and %zu-byte enroll data",
                              _albumData.size(), _enrollData.size());
        }
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
    std::list<Vision::FaceNameAndID> namesAndIDs;
    Result result = _visionSystem->LoadFaceAlbum(path, namesAndIDs);
    BroadcastLoadedNamesAndIDs(namesAndIDs);
    
    if(RESULT_OK != result) {
      PRINT_NAMED_WARNING("VisionComponent.LoadFaceAlbum.LoadFromFileFailed",
                          "AlbumFile: %s", path.c_str());
    } else {
      result = SaveFaceAlbumToRobot();
    }
    
    return result;
  }
  
  
  void VisionComponent::AssignNameToFace(Vision::FaceID_t faceID, const std::string& name)
  {  
    // Pair this name and ID in the vision system
    Lock();
    _visionSystem->AssignNameToFace(faceID, name);
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
  
  Result VisionComponent::EraseFace(const std::string& name)
  {
    Lock();
    Vision::FaceID_t erasedID = _visionSystem->EraseFace(name);
    Unlock();
    if(Vision::UnknownFaceID != erasedID) {
      // Update robot
      SaveFaceAlbumToRobot();
      // Send back confirmation
      ExternalInterface::RobotErasedEnrolledFace msg;
      msg.faceID  = erasedID;
      msg.name    = name;
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
    Lock();
    Result result = _visionSystem->RenameFace(faceID, oldName, newName);
    Unlock();
    
    if(RESULT_OK == result)
    {
      SaveFaceAlbumToRobot();
      ExternalInterface::RobotLoadedKnownFace msg;
      msg.faceID = faceID;
      msg.name   = newName;
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
    }
    
    return result;
  }
  
  void VisionComponent::BroadcastLoadedNamesAndIDs(const std::list<Vision::FaceNameAndID>& namesAndIDs) const
  {
    // Notify about the newly-available names and IDs, and create wave files
    // for the names if they don't already exist, so we've already got them
    // when we want to say them at some point later.
    using namespace ExternalInterface;
    _robot.Broadcast(MessageEngineToGame(RobotErasedAllEnrolledFaces()));
    for(auto & nameAndID : namesAndIDs)
    {
      RobotLoadedKnownFace msg;
      msg.name = nameAndID.name;
      msg.faceID = nameAndID.faceID;
      _robot.Broadcast(MessageEngineToGame(std::move(msg)));
      
      // TODO: Need to determine what styles need to be created
      _robot.GetTextToSpeechComponent().CreateSpeech(nameAndID.name, SayTextStyle::Normal);
    }
  }
  
#pragma mark - 
#pragma mark Message Handlers
  
  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::EnableVisionMode& payload)
  {
    EnableMode(payload.mode, payload.enable);
  }
  
  
  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::SetFaceEnrollmentPose& msg)
  {
    SetFaceEnrollmentMode(msg.pose);
  }
  
  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::VisionWhileMoving& msg)
  {
    EnableVisionWhileMovingFast(msg.enable);
  }
  
  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::VisionRunMode& msg)
  {
    SetRunMode((msg.isSync ? RunMode::Synchronous : RunMode::Asynchronous));
  }
  
  template<>
  void VisionComponent::HandleMessage(const ExternalInterface::EraseEnrolledFaceByName& msg)
  {
    EraseFace(msg.name);
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
  
} // namespace Cozmo
} // namespace Anki
