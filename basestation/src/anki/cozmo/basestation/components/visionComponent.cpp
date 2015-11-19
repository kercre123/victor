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
#include "util/helpers/templateHelpers.h"
#include "anki/common/basestation/utils/helpers/boundedWhile.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include "anki/cozmo/basestation/viz/vizManager.h"

namespace Anki {
namespace Cozmo {
  
  VisionComponent::VisionComponent(RobotID_t robotID, RunMode mode, Util::Data::DataPlatform* dataPlatform)
  : _camera(robotID)
  , _runMode(mode)
  , _groundPlaneMask(_groundPlaneROI.widthFar, _groundPlaneROI.length)
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
    
    assert(false == _groundPlaneMask.IsEmpty());
    
    const s32 w = std::round(0.5f*(_groundPlaneROI.widthFar - _groundPlaneROI.widthClose));
    _groundPlaneMask.FillWith(0);
    cv::fillConvexPoly(_groundPlaneMask.get_CvMat_(), std::vector<cv::Point>{
      cv::Point(0, w),
      cv::Point(_groundPlaneROI.length-1,0),
      cv::Point(_groundPlaneROI.length-1,_groundPlaneROI.widthFar-1),
      cv::Point(0, w + _groundPlaneROI.widthClose)
    }, 255);
    
  } // VisionSystem()

  void VisionComponent::SetCameraCalibration(const Robot& robot, const Vision::CameraCalibration& camCalib)
  {
    if(_camCalib != camCalib)
    {
      _camCalib = camCalib;
      _camera.SetSharedCalibration(&_camCalib);
      _isCamCalibSet = true;
      
      _visionSystem->UnInit();
      
      // Got a new calibration: rebuild the LUT for ground plane homographies
      PopulateGroundPlaneHomographyLUT(robot);
    }
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
    
    Util::SafeDelete(_visionSystem);
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
      PRINT_NAMED_ERROR("VisionComponent.SetMarkerToTrack.NullVisionSystem",
                        "Cannot set vision marker to track before vision system is instantiated.\n");
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
  
  TimeStamp_t VisionComponent::GetProcessingPeriod()
  {
    Lock();
    const TimeStamp_t t = _processingPeriod;
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
  
  bool VisionComponent::IsModeEnabled(VisionMode mode) const
  {
    if(nullptr != _visionSystem) {
      return _visionSystem->IsModeEnabled(mode);
    } else {
      return false;
    }
  }
  
  u32 VisionComponent::GetEnabledModes() const
  {
    if(nullptr != _visionSystem) {
      return _visionSystem->GetEnabledModes();
    } else {
      return 0;
    }
  }
  
  Result VisionComponent::SetModes(u32 modes)
  {
    if(nullptr != _visionSystem) {
      _visionSystem->SetModes(modes);
      return RESULT_OK;
    } else {
      return RESULT_FAIL;
    }
  }
  
  Result VisionComponent::SetNextImage(const Vision::ImageRGB& image,
                                       const Robot& robot)
  {
    if(_isCamCalibSet) {
      ASSERT_NAMED(nullptr != _visionSystem, "VisionSystem should not be NULL.");
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
      
      // Fill in the pose data for the given image, by querying robot history
      Lock();
      Result lastResult = robot.GetPoseHistory()->ComputePoseAt(image.GetTimestamp(), _nextPoseData.timeStamp, _nextPoseData.poseStamp, true);
      Unlock();
      
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_ERROR("VisionComponent.SetNextImage.PoseHistoryFail",
                          "Unable to get computed pose at image timestamp of %d.\n", image.GetTimestamp());
        return lastResult;
      }
      
      Lock();
      _nextPoseData.cameraPose = robot.GetHistoricalCameraPose(_nextPoseData.poseStamp, _nextPoseData.timeStamp);
      _nextPoseData.groundPlaneVisible = LookupGroundPlaneHomography(_nextPoseData.poseStamp.GetHeadAngle(),
                                                                     _nextPoseData.groundPlaneHomography);
      Unlock();
      
      UpdateOverheadMap(image, _nextPoseData);
      
      switch(_runMode)
      {
        case RunMode::Synchronous:
        {
          if(!_paused) {
            _visionSystem->Update(_nextPoseData, image);
            _lastImg = image;
            
            VizManager::getInstance()->SetText(VizManager::VISION_MODE, NamedColors::CYAN,
                                               "Vision: %s", _visionSystem->GetCurrentModeName().c_str());
          }
          break;
        }
        case RunMode::Asynchronous:
        {
          Lock();
          
          if(!_nextImg.IsEmpty()) {
            PRINT_NAMED_INFO("VisionComponent.SetNextImage.DropedFrame",
                             "Setting next image with t=%d, but existing next image from t=%d not yet processed.",
                             image.GetTimestamp(), _nextImg.GetTimestamp());
          }
          
          // TODO: Avoid the copying here (shared memory?)
          image.CopyTo(_nextImg);
          
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
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
    
  } // SetNextImage()
  
  void VisionComponent::PopulateGroundPlaneHomographyLUT(const Robot& robot, f32 angleResolution_rad)
  {
    const Pose3d& robotPose = robot.GetPose();
    
    ASSERT_NAMED(_camera.IsCalibrated(), "Camera must be calibrated to populate homography LUT.");
    
    const Matrix_3x3f K = _camera.GetCalibration().GetCalibrationMatrix();
    
    // Loop over all possible head angles at the specified resolution and store
    // the ground plane homography for each.
    for(f32 headAngle_rad = MIN_HEAD_ANGLE; headAngle_rad <= MAX_HEAD_ANGLE;
        headAngle_rad += angleResolution_rad)
    {
      // Get the robot origin w.r.t. the camera position with the camera at
      // the current head angle
      Pose3d robotPoseWrtCamera;
      bool result = robotPose.GetWithRespectTo(robot.GetCameraPose(headAngle_rad), robotPoseWrtCamera);
      assert(result == true); // this really shouldn't fail! camera has to be in the robot's pose tree
      
      const RotationMatrix3d& R = robotPoseWrtCamera.GetRotationMatrix();
      const Vec3f&            T = robotPoseWrtCamera.GetTranslation();
      
      // Construct the homography mapping points on the ground plane into the
      // image plane
      const Matrix_3x3f H = K*Matrix_3x3f{R.GetColumn(0),R.GetColumn(1),T};
      
      Point3f temp = H * Point3f(_groundPlaneROI.dist + _groundPlaneROI.length, 0.f, 1.f);
      if(temp.z() > 0.f && temp.y()/temp.z() < _camera.GetCalibration().GetNrows()) {
        // Only store this homography if the ROI still projects into the image
        _groundPlaneHomographyLUT[headAngle_rad] = H;
      } else {
        PRINT_NAMED_INFO("VisionComponent.PopulateGroundPlaneHomographyLUT.MaxHeadAngleReached",
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

  void VisionComponent::Processor()
  {
    PRINT_NAMED_INFO("VisionComponent.Processor",
                     "Starting Robot VisionComponent::Processor thread...");
    
    ASSERT_NAMED(_visionSystem != nullptr && _visionSystem->IsInitialized(),
                 "VisionSystem should already be initialized.");
    
    while (_running) {
      
      if(_paused) {
        usleep(100);
        continue;
      }
      
      //if(_currentImg != nullptr) {
      if(!_currentImg.IsEmpty()) {
        // There is an image to be processed:
        
        //assert(_currentImg != nullptr);
        _visionSystem->Update(_currentPoseData, _currentImg);
        
        VizManager::getInstance()->SetText(VizManager::VISION_MODE, NamedColors::CYAN,
                                           "Vision: %s", _visionSystem->GetCurrentModeName().c_str());
        
        Lock();
        // Store frame rate
        _processingPeriod = _currentImg.GetTimestamp() - _lastImg.GetTimestamp();
        
        // Save the image we just processed
        _lastImg = _currentImg;
        ASSERT_NAMED(_lastImg.GetTimestamp() == _currentImg.GetTimestamp(),
                     "Last image should now have current image's timestamp.");
        
        // Clear it when done.
        _currentImg = {};
        _nextImg = {};
        
        Unlock();
        
      } else if(!_nextImg.IsEmpty()) {
        Lock();
        _currentImg        = _nextImg;
        _currentPoseData   = _nextPoseData;
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
          PRINT_NAMED_ERROR("VisionComponent.Update.FailedToQueueVisionMarker",
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
         PRINT_NAMED_INFO("VisionComponent.Update",
         "Robot %d reported seeing a face at (x,y,w,h)=(%.1f,%.1f,%.1f,%.1f), "
         "at t=%d (lastImg t=%d).",
         GetID(), faceDetection.GetRect().GetX(), faceDetection.GetRect().GetY(),
         faceDetection.GetRect().GetWidth(), faceDetection.GetRect().GetHeight(),
         faceDetection.GetTimeStamp(), GetLastImageTimeStamp());
         */
        
        // Use the faceDetection to update FaceWorld:
        lastResult = robot.GetFaceWorld().AddOrUpdateFace(faceDetection);
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_ERROR("VisionComponent.Update.FailedToUpdateFace",
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
         PRINT_NAMED_ERROR("VisionComponent.Update.PoseOriginFail",
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
      if (true == _visionSystem->CheckMailbox(motionCentroid))
      {
        using namespace ExternalInterface;
        motionCentroid -= _camera.GetCalibration().GetCenter(); // make relative to image center
       
        robot.Broadcast(MessageEngineToGame(RobotObservedMotion(motionCentroid.x(), motionCentroid.y())));
      }
    } // if(_visionSystem != nullptr)
    return RESULT_OK;
  } // UpdateMotionCentroid()
  
  
  Result VisionComponent::UpdateOverheadMap(const Vision::ImageRGB& image,
                                            const VisionSystem::PoseData& poseData)
  {
    // Define ROI quad on ground plane, in robot-centric coordinates (origin is *)
    // The region is "d" mm long and starts "d0" mm from the robot origin.
    // It is "w_close" mm wide at the end close to the robot and "w_far" mm
    // wide at the opposite end
    //                              _____
    //  +---------+    _______------     |
    //  | Robot   |   |                  |
    //  |       * |   | w_close          | w_far
    //  |         |   |_______           |
    //  +---------+           ------_____|
    //
    //          |<--->|<---------------->|
    //           dist         length
    //
    
    if(poseData.groundPlaneVisible)
    {
      const Matrix_3x3f& H = poseData.groundPlaneHomography;
      
      // Note that the z coordinate is actually 0, but in the mapping to the
      // image plane below, we are actually doing K[R t]* [Px Py Pz 1]',
      // and Pz == 0 and we thus drop out the third column, making it
      // K[R t] * [Px Py 0 1]' or H * [Px Py 1]', so for convenience, we just
      // go ahead and fill in that 1 here:
      const GroundPlaneROI& roi = _groundPlaneROI;
      const Quad3f groundQuad({roi.dist + roi.length,  0.5f*roi.widthFar,   1.f},
                              {roi.dist             ,  0.5f*roi.widthClose, 1.f},
                              {roi.dist + roi.length, -0.5f*roi.widthFar,   1.f},
                              {roi.dist             , -0.5f*roi.widthClose, 1.f});

      // Project ground quad in camera image
      // (This could be done by Camera::ProjectPoints, but that would duplicate
      //  the computation of H we did above, which here we need to use below)
      Quad2f imgGroundQuad;
      for(Quad::CornerName iCorner = Quad::CornerName::FirstCorner;
          iCorner != Quad::CornerName::NumCorners; ++iCorner)
      {
        Point3f temp = H * groundQuad[iCorner];
        ASSERT_NAMED(temp.z() > 0.f, "Projected ground quad points should have z > 0.");
        const f32 divisor = 1.f / temp.z();
        imgGroundQuad[iCorner].x() = temp.x() * divisor;
        imgGroundQuad[iCorner].y() = temp.y() * divisor;
      }
      
      static Vision::ImageRGB overheadMap(1000.f, 1000.f);
      
      // Need to apply a shift after the homography to put things in image
      // coordinates with (0,0) at the upper left (since groundQuad's origin
      // is not upper left). Also mirror Y coordinates since we are looking
      // from above, not below
      Matrix_3x3f InvShift{
        1.f, 0.f, roi.dist, // Negated b/c we're using inv(Shift)
        0.f,-1.f, roi.widthFar*0.5f,
        0.f, 0.f, 1.f};

      static Vision::Image roiMask(roi.widthFar, roi.length);
      static bool maskFilled = false;
      if(!maskFilled) {
        const f32 w = 0.5f*(roi.widthFar - roi.widthClose);
        const f32 invTanAngle = roi.length / w;
        for(s32 i=0; i<roi.widthFar; ++i) {
          u8* row_i = roiMask.GetRow(i);
          
          s32 xmax = 0;
          if(i < w) {
            xmax = std::round(roi.length - static_cast<f32>(i) * invTanAngle);
          } else if(i > w + roi.widthClose) {
            xmax = std::round(roi.length - (roi.widthFar - static_cast<f32>(i)) * invTanAngle);
          }
          
          for(s32 j=0; j<xmax; ++j) {
            row_i[j] = 0;
          }
          for(s32 j=xmax; j<roi.length; ++j) {
            row_i[j] = 255;
          }
        }
        maskFilled = true;
        
        roiMask.Display("RoiMask");
      }
      
      Pose3d worldPoseWrtRobot = poseData.poseStamp.GetPose().GetInverse();
      for(s32 i=0; i<roi.widthFar; ++i) {
        const u8* mask_i = roiMask.GetRow(i);
        const f32 y = static_cast<f32>(i) - 0.5f*roi.widthFar;
        for(s32 j=0; j<roi.length; ++j) {
          if(mask_i[j] > 0) {
            // Project ground plane point in robot frame to image
            const f32 x = static_cast<f32>(j) + roi.dist;
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
      
      Vision::ImageRGB overheadImg;
      
      // Note that we're applying the inverse homography, so we're doing
      //  inv(Shift * inv(H)), which is the same as  (H * inv(Shift))
      cv::warpPerspective(image.get_CvMat_(), overheadImg.get_CvMat_(), (H*InvShift).get_CvMatx_(),
                          cv::Size(roi.length, roi.widthFar), cv::INTER_LINEAR | cv::WARP_INVERSE_MAP);
      
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
        poseData.poseStamp.GetPose().ApplyTo(groundQuad, lastUpdate);
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

} // namespace Cozmo
} // namespace Anki
