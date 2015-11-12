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
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/visionSystem.h"
#include "anki/cozmo/basestation/cozmoActions.h"

#include "anki/vision/basestation/image_impl.h"
#include "anki/vision/basestation/trackedFace.h"
#include "anki/vision/MarkerCodeDefinitions.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "util/logging/logging.h"
#include "anki/common/basestation/utils/helpers/boundedWhile.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"

#include "anki/cozmo/basestation/viz/vizManager.h"

namespace Anki {
namespace Cozmo {
  
  VisionComponent::VisionComponent(RobotID_t robotID, RunMode mode, Util::Data::DataPlatform* dataPlatform)
  : _camera(robotID)
  , _runMode(mode)
  {
    std::string dataPath("");
    if(dataPlatform != nullptr) {
      dataPath = dataPlatform->pathToResource(Util::Data::Scope::Resources,
                                              "/config/basestation/vision");
    } else {
      PRINT_NAMED_WARNING("VisionComponent.Constructor.NullDataPlatform",
                          "Insantiating VisionSystem with a empty DataPath.");
    }
    
    _visionSystem = new VisionSystem(dataPath);
    
  } // VisionSystem()

  void VisionComponent::SetCameraCalibration(const Vision::CameraCalibration& camCalib)
  {
    _camCalib = camCalib;
    _camera.SetSharedCalibration(&_camCalib);
    _isCamCalibSet = true;
  }
  
  
  void VisionComponent::SetRunMode(RunMode mode) {
    if(mode == RunMode::Synchronous && _runMode == RunMode::Asynchronous) {
      PRINT_NAMED_INFO("VisionComponent.SetRunMode.SwitchToAsync", "");
      if(_running) {
        Stop();
      }
      _runMode = mode;
    }
    else if(mode == RunMode::Asynchronous && _runMode == RunMode::Synchronous) {
      PRINT_NAMED_INFO("VisionComponent.SetRunMode.SwitchToSync", "");
      _runMode = mode;
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
  
  /*
  void VisionComponent::Start(const Vision::CameraCalibration& camCalib)
  {
    SetCameraCalibration(camCalib);
    Start();
  }
   */

  void VisionComponent::Stop()
  {
    _running = false;
    
    
    // Wait for processing thread to die before destructing since we gave it
    // a reference to *this
    if(_processingThread.joinable()) {
      _processingThread.join();
    }
    
    _currentImg = {};
    _nextImg    = {};
    _lastImg    = {};

  }


  VisionComponent::~VisionComponent()
  {
    Stop();
    
    if(_visionSystem != nullptr) {
      delete _visionSystem;
      _visionSystem = nullptr;
    }
  } // ~VisionSystem()
 
  
  void VisionComponent::SetMarkerToTrack(const Vision::Marker::Code&  markerToTrack,
                        const f32                    markerWidth_mm,
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
      _visionSystem->SetMarkerToTrack(markerType, markerWidth_mm,
                                      pt, radius, checkAngleX,
                                      postOffsetX_mm,
                                      postOffsetY_mm,
                                      postOffsetAngle_rad);
    } else {
      PRINT_NAMED_ERROR("VisionComponent.SetMarkerToTrack.NullVisionSystem", "Cannot set vision marker to track before vision system is instantiated.\n");
    }
  }
  
  
  bool VisionComponent::GetCurrentImage(Vision::ImageRGB& img, TimeStamp_t newerThanTimestamp)
  {
    bool retVal = false;
    
    Lock();
    if(_running && !_currentImg.IsEmpty() && _currentImg.GetTimestamp() > newerThanTimestamp) {
      _currentImg.CopyTo(img);
      img.SetTimestamp(_currentImg.GetTimestamp());
      retVal = true;
    } else {
      img = {};
      retVal = false;
    }
    Unlock();
    
    return retVal;
  }
  
  bool VisionComponent::GetLastProcessedImage(Vision::ImageRGB& img,
                                                     TimeStamp_t newerThanTimestamp)
  {
    bool retVal = false;
    
    Lock();
    if(!_lastImg.IsEmpty() && _lastImg.GetTimestamp() > newerThanTimestamp) {
      _lastImg.CopyTo(img);
      img.SetTimestamp(_lastImg.GetTimestamp());
      retVal = true;
    }
    Unlock();
    
    return retVal;
  }

  TimeStamp_t VisionComponent::GetLastProcessedImageTimeStamp()
  {
    
    Lock();
    const TimeStamp_t t = (_lastImg.IsEmpty() ? 0 : _lastImg.GetTimestamp());
    Unlock();

    return t;
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
  
  void VisionComponent::SetNextImage(const Vision::ImageRGB& image,
                                     const RobotState& robotState)
  {
    if(_isCamCalibSet) {
      if(!_visionSystem->IsInitialized()) {
        _visionSystem->Init(_camCalib);
        
        // Wait for initialization to complete (i.e. Matlab to start up, if needed)
        while(!_visionSystem->IsInitialized()) {
          usleep(500);
        }
        
        if(_runMode == RunMode::Asynchronous) {
          Start();
        }
      }
      
      switch(_runMode)
      {
        case RunMode::Synchronous:
        {
          if(!_paused) {
            _visionSystem->Update(robotState, image);
            _lastImg = image;
            
            VizManager::getInstance()->SetText(VizManager::VISION_MODE, NamedColors::CYAN,
                                               "Vision: %s", _visionSystem->GetCurrentModeName().c_str());
          }
          break;
        }
        case RunMode::Asynchronous:
        {
          Lock();
          
          // TODO: Avoid the copying here (shared memory?)
          image.CopyTo(_nextImg);
          
          _nextRobotState = robotState;
          
          Unlock();
          break;
        }
        default:
          PRINT_NAMED_ERROR("VisionComponent.SetNextImage.InvalidRunMode", "");
      } // switch(_runMode)
      
      // Display any debug images left by the vision system
      std::pair<const char*, Vision::Image>    debugGray;
      std::pair<const char*, Vision::ImageRGB> debugRGB;
      while(_visionSystem->CheckDebugMailbox(debugGray)) {
        debugGray.second.Display(debugGray.first);
      }
      while(_visionSystem->CheckDebugMailbox(debugRGB)) {
        debugRGB.second.Display(debugRGB.first);
      }
      
    } else {
      PRINT_NAMED_ERROR("VisionComponent.Update.NoCamCalib",
                        "Camera calibration must be set before calling Update().\n");
    }
    
  } // SetNextImage()

  void VisionComponent::Processor()
  {
    PRINT_NAMED_INFO("VisionComponent.Processor",
                     "Starting Robot VisionComponent::Processor thread...");
    
    ASSERT_NAMED(_visionSystem != nullptr && _visionSystem->IsInitialized(),
                 "VisionSystem should already be initialized.");
    /*
    _visionSystem->Init(_camCalib);
    
    // Wait for initialization to complete (i.e. Matlab to start up, if needed)
    while(!_visionSystem->IsInitialized()) {
      usleep(500);
    }
     */
    
    while (_running) {
      
      if(_paused) {
        usleep(100);
        continue;
      }
      
      //if(_currentImg != nullptr) {
      if(!_currentImg.IsEmpty()) {
        // There is an image to be processed:
        
        //assert(_currentImg != nullptr);
        _visionSystem->Update(_currentRobotState, _currentImg);
        
        VizManager::getInstance()->SetText(VizManager::VISION_MODE, NamedColors::CYAN,
                                           "Vision: %s", _visionSystem->GetCurrentModeName().c_str());
        
        Lock();
        // Save the image we just processed
        _lastImg = _currentImg;
        ASSERT_NAMED(_lastImg.GetTimestamp() == _currentImg.GetTimestamp(),
                     "Last image should now have current image's timestamp.");
        
        // Clear it when done.
        _currentImg = {};
        Unlock();
        
      } else if(!_nextImg.IsEmpty()) {
        Lock();
        _currentImg        = _nextImg;
        _currentRobotState = _nextRobotState;
        _nextImg = {};
        Unlock();
      } else {
        usleep(100);
      }
      
    } // while(_running)
    
    if(_visionSystem != nullptr) {
      delete _visionSystem;
      _visionSystem = nullptr;
    }
    
    PRINT_NAMED_INFO("VisionComponent.Processor",
                     "Terminated Robot VisionComponent::Processor thread");
  } // Processor()
  
  Result VisionComponent::UpdateVisionMarkers(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    if(_visionSystem != nullptr)
    {
      Vision::ObservedMarker visionMarker;
      while(true == _visionSystem->CheckMailbox(visionMarker)) {
        
        lastResult = robot.QueueObservedMarker(visionMarker);
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_ERROR("Robot.Update.FailedToQueueVisionMarker",
                            "Got VisionMarker message from vision processing thread but failed to queue it.");
          return lastResult;
        }
        
        const Quad2f& corners = visionMarker.GetImageCorners();
        VizManager::getInstance()->SendVisionMarker(corners[Quad::TopLeft].x(),  corners[Quad::TopLeft].y(),
                                                    corners[Quad::TopRight].x(),  corners[Quad::TopRight].y(),
                                                    corners[Quad::BottomRight].x(),  corners[Quad::BottomRight].y(),
                                                    corners[Quad::BottomLeft].x(),  corners[Quad::BottomLeft].y(),
                                                    visionMarker.GetCode() != Vision::MARKER_UNKNOWN);
      }
    }
    return lastResult;
  } // UpdateVisionMarkers()
  
  Result VisionComponent::UpdateFaces(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    if(_visionSystem != nullptr)
    {
      Vision::TrackedFace faceDetection;
      while(true == _visionSystem->CheckMailbox(faceDetection)) {
        /*
         PRINT_NAMED_INFO("Robot.Update",
         "Robot %d reported seeing a face at (x,y,w,h)=(%.1f,%.1f,%.1f,%.1f), "
         "at t=%d (lastImg t=%d).",
         GetID(), faceDetection.GetRect().GetX(), faceDetection.GetRect().GetY(),
         faceDetection.GetRect().GetWidth(), faceDetection.GetRect().GetHeight(),
         faceDetection.GetTimeStamp(), GetLastImageTimeStamp());
         */
        
        // Get historical robot pose at specified timestamp to make sure we've
        // got a robot/camera in pose history
        TimeStamp_t t;
        RobotPoseStamp* p = nullptr;
        HistPoseKey poseKey;
        lastResult = robot.GetPoseHistory()->ComputeAndInsertPoseAt(faceDetection.GetTimeStamp(),
                                                                    t, &p, &poseKey, true);
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_WARNING("Robot.Update.HistoricalPoseNotFound",
                              "For face seen at time: %d, hist: %d to %d\n",
                              faceDetection.GetTimeStamp(),
                              robot.GetPoseHistory()->GetOldestTimeStamp(),
                              robot.GetPoseHistory()->GetNewestTimeStamp());
          return lastResult;
        }
        
        // Use a camera from the robot's pose history to estimate the head's
        // 3D translation, w.r.t. that camera. Also puts the face's pose in
        // the camera's pose chain.
        faceDetection.UpdateTranslation(robot.GetHistoricalCamera(p, t));
        
        // Now use the faceDetection to update FaceWorld:
        lastResult = robot.GetFaceWorld().AddOrUpdateFace(faceDetection);
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_ERROR("Robot.Update.FailedToUpdateFace",
                            "Got FaceDetection from vision processing but failed to update it.");
          return lastResult;
        }
        
        VizManager::getInstance()->DrawCameraFace(faceDetection,
                                                  faceDetection.IsBeingTracked() ? NamedColors::GREEN : NamedColors::RED);
      }
    } // if(_visionSystem != nullptr)
    return lastResult;
  } // UpdateFaces()
  
  Result VisionComponent::UpdateTrackingQuad(Robot& robot)
  {
    if(_visionSystem != nullptr)
    {
      VizInterface::TrackerQuad trackerQuad;
      if(true == _visionSystem->CheckMailbox(trackerQuad)) {
        // Send tracker quad info to viz
        VizManager::getInstance()->SendTrackerQuad(trackerQuad.topLeft_x, trackerQuad.topLeft_y,
                                                   trackerQuad.topRight_x, trackerQuad.topRight_y,
                                                   trackerQuad.bottomRight_x, trackerQuad.bottomRight_y,
                                                   trackerQuad.bottomLeft_x, trackerQuad.bottomLeft_y);
      }
    }
    return RESULT_OK;
  } // UpdateTrackingQuad()
  
  Result VisionComponent::UpdateDockingErrorSignal(Robot& robot)
  {
    if(_visionSystem != nullptr)
    {
      std::pair<Pose3d, TimeStamp_t> markerPoseWrtCamera;
      if(true == _visionSystem->CheckMailbox(markerPoseWrtCamera)) {
        
        // Hook the pose coming out of the vision system up to the historical
        // camera at that timestamp
        Vision::Camera histCamera(robot.GetHistoricalCamera(markerPoseWrtCamera.second));
        markerPoseWrtCamera.first.SetParent(&histCamera.GetPose());
        /*
         // Get the pose w.r.t. the (historical) robot pose instead of the camera pose
         Pose3d markerPoseWrtRobot;
         if(false == markerPoseWrtCamera.first.GetWithRespectTo(p.GetPose(), markerPoseWrtRobot)) {
         PRINT_NAMED_ERROR("Robot.Update.PoseOriginFail",
         "Could not get marker pose w.r.t. robot.");
         return RESULT_FAIL;
         }
         */
        //Pose3d poseWrtRobot = poseWrtCam;
        //poseWrtRobot.PreComposeWith(camWrtRobotPose);
        Pose3d markerPoseWrtRobot(markerPoseWrtCamera.first);
        markerPoseWrtRobot.PreComposeWith(histCamera.GetPose());
        
        DockingErrorSignal dockErrMsg;
        dockErrMsg.timestamp = markerPoseWrtCamera.second;
        dockErrMsg.x_distErr = markerPoseWrtRobot.GetTranslation().x();
        dockErrMsg.y_horErr  = markerPoseWrtRobot.GetTranslation().y();
        dockErrMsg.z_height  = markerPoseWrtRobot.GetTranslation().z();
        dockErrMsg.angleErr  = markerPoseWrtRobot.GetRotation().GetAngleAroundZaxis().ToFloat() + M_PI_2;
        
        // Visualize docking error signal
        VizManager::getInstance()->SetDockingError(dockErrMsg.x_distErr,
                                                   dockErrMsg.y_horErr,
                                                   dockErrMsg.angleErr);
        
        // Try to use this for closed-loop control by sending it on to the robot
        robot.SendRobotMessage<DockingErrorSignal>(std::move(dockErrMsg));
      }
    } // if(_visionSystem != nullptr)
    return RESULT_OK;
  } // UpdateDockingErrorSignal()
  
  Result VisionComponent::UpdateMotionCentroid(Robot& robot)
  {
    if(_visionSystem != nullptr)
    {
      Point2f motionCentroid;
      if (true == _visionSystem->CheckMailbox(motionCentroid)) {
        
        motionCentroid -= _camera.GetCalibration().GetCenter(); // make relative to image center
        
        if(NEAR(motionCentroid.x(), 0.f, _camera.GetCalibration().GetNcols()/10) &&
           NEAR(motionCentroid.y(), 0.f, _camera.GetCalibration().GetNrows()/10))
        {
          // Move towards the motion since it's centered
          const f32 dist_mm = 20;
          Pose3d newPose(robot.GetPose());
          const f32 angleRad = newPose.GetRotation().GetAngleAroundZaxis().ToFloat();
          newPose.SetTranslation({newPose.GetTranslation().x() + dist_mm*std::cos(angleRad),
            newPose.GetTranslation().y() + dist_mm*std::sin(angleRad),
            newPose.GetTranslation().z()});
          robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, new DriveToPoseAction(newPose, false, false, 50, DEG_TO_RAD(45)));
          
        } else {
          // Turn to face the motion:
          // Convert image positions to desired angles (in absolute coordinates)
          RobotInterface::PanAndTilt msg;
          const f32 relHeadAngle_rad = std::atan(-motionCentroid.y() / _camera.GetCalibration().GetFocalLength_y());
          msg.headTiltAngle_rad = robot.GetHeadAngle() + relHeadAngle_rad;
          const f32 relBodyPanAngle_rad = std::atan(-motionCentroid.x() / _camera.GetCalibration().GetFocalLength_x());
          msg.bodyPanAngle_rad = (robot.GetPose().GetRotation().GetAngleAroundZaxis() + relBodyPanAngle_rad).ToFloat();
          
          robot.SendRobotMessage<RobotInterface::PanAndTilt>(std::move(msg));
        }
      }
    } // if(_visionSystem != nullptr)
    return RESULT_OK;
  } // UpdateMotionCentroid()
  
} // namespace Cozmo
} // namespace Anki
