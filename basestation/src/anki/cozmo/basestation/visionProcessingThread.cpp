/**
 * File: visionProcessingThread.cpp
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

#include "anki/cozmo/basestation/visionProcessingThread.h"
#include "anki/cozmo/basestation/visionSystem.h"

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
  
  VisionProcessingThread::VisionProcessingThread(Util::Data::DataPlatform* dataPlatform)
  {
    std::string dataPath("");
    if(dataPlatform != nullptr) {
      dataPath = dataPlatform->pathToResource(Util::Data::Scope::Resources,
                                              "/config/basestation/vision");
    } else {
      PRINT_NAMED_WARNING("VisionProcessingThread.Constructor.NullDataPlatform",
                          "Insantiating VisionSystem with a empty DataPath.");
    }
    
    _visionSystem = new VisionSystem(dataPath);
    
  } // VisionSystem()

  void VisionProcessingThread::SetCameraCalibration(const Vision::CameraCalibration& camCalib)
  {
    _camCalib = camCalib;
    _isCamCalibSet = true;
  }
  
  void VisionProcessingThread::Start()
  {
    if(!_isCamCalibSet) {
      PRINT_NAMED_ERROR("VisionProcessingThread.Start",
                        "Camera calibration must be set to start VisionProcessingThread.");
      return;
    }
    
    if(_running) {
      PRINT_NAMED_INFO("VisionProcessingThread.Start.Restarting",
                       "Thread already started, call Stop() and then restarting.");
      Stop();
    } else {
      PRINT_NAMED_INFO("VisionProcessingThread.Start",
                       "Starting vision processing thread.");
    }
    
    _running = true;
    _paused = false;
    
    // Note that we're giving the Processor a pointer to "this", so we
    // have to ensure this VisionSystem object outlives the thread.
    _processingThread = std::thread(&VisionProcessingThread::Processor, this);
    //_processingThread.detach();
    
  }
  
  void VisionProcessingThread::Start(const Vision::CameraCalibration& camCalib)
  {
    SetCameraCalibration(camCalib);
    Start();
  }

  void VisionProcessingThread::Stop()
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


  VisionProcessingThread::~VisionProcessingThread()
  {
    Stop();
    
    if(_visionSystem != nullptr) {
      delete _visionSystem;
      _visionSystem = nullptr;
    }
  } // ~VisionSystem()


  void VisionProcessingThread::SetNextImage(const Vision::Image &image,
                                            const RobotState &robotState)
  {
    Lock();
    
    // TODO: Avoid the copying here (shared memory?)
    image.CopyDataTo(_nextImg);
    _nextImg.SetTimestamp(image.GetTimestamp());
    
    //_nextImg = Vision::Image(image);
    _nextRobotState = robotState;
    
    Unlock();
    
  } // SetNextImage()
  
  
  void VisionProcessingThread::SetMarkerToTrack(const Vision::Marker::Code&  markerToTrack,
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
      PRINT_NAMED_ERROR("VisionProcessingThread.SetMarkerToTrack.NullVisionSystem", "Cannot set vision marker to track before vision system is instantiated.\n");
    }
  }
  
  
  bool VisionProcessingThread::GetCurrentImage(Vision::Image& img, TimeStamp_t newerThanTimestamp)
  {
    bool retVal = false;
    
    Lock();
    if(_running && !_currentImg.IsEmpty() && _currentImg.GetTimestamp() > newerThanTimestamp) {
      _currentImg.CopyDataTo(img);
      img.SetTimestamp(_currentImg.GetTimestamp());
      retVal = true;
    } else {
      img = {};
      retVal = false;
    }
    Unlock();
    
    return retVal;
  }
  
  bool VisionProcessingThread::GetLastProcessedImage(Vision::Image& img,
                                                     TimeStamp_t newerThanTimestamp)
  {
    bool retVal = false;
    
    Lock();
    if(!_lastImg.IsEmpty() && _lastImg.GetTimestamp() > newerThanTimestamp) {
      _lastImg.CopyDataTo(img);
      img.SetTimestamp(_lastImg.GetTimestamp());
      retVal = true;
    }
    Unlock();
    
    return retVal;
  }

  TimeStamp_t VisionProcessingThread::GetLastProcessedImageTimeStamp()
  {
    
    Lock();
    const TimeStamp_t t = (_lastImg.IsEmpty() ? 0 : _lastImg.GetTimestamp());
    Unlock();

    return t;
  }
  
  void VisionProcessingThread::EnableMarkerDetection(bool enable)
  {
    if(enable) {
      _visionSystem->StartMarkerDetection();
    } else {
      _visionSystem->StopMarkerDetection();
    }
  }

  void VisionProcessingThread::EnableFaceDetection(bool enable)
  {
    if(enable) {
      _visionSystem->StartDetectingFaces();
    } else {
      _visionSystem->StopDetectingFaces();
    }
  }
  
  void VisionProcessingThread::StopMarkerTracking()
  {
    _visionSystem->StopTracking();
  }

  void VisionProcessingThread::Lock()
  {
    _lock.lock();
  }

  void VisionProcessingThread::Unlock()
  {
    _lock.unlock();
  }

  void VisionProcessingThread::Update(const Vision::Image& image,
                                      const RobotState& robotState)
  {
    if(_isCamCalibSet) {
      if(!_visionSystem->IsInitialized()) {
        _visionSystem->Init(_camCalib);
        
        // Wait for initialization to complete (i.e. Matlab to start up, if needed)
        while(!_visionSystem->IsInitialized()) {
          usleep(500);
        }
      }
      
      if(!_paused) {
        _visionSystem->Update(robotState, image);
        _lastImg = image;
        
        VizManager::getInstance()->SetText(VizManager::VISION_MODE, NamedColors::CYAN,
                                           "Vision: %s", _visionSystem->GetCurrentModeName().c_str());
      }
      
    } else {
      PRINT_NAMED_ERROR("VisionProcessingThread.Update.NoCamCalib",
                        "Camera calibration must be set before calling Update().\n");
    }
    
  } // Update()

  void VisionProcessingThread::Processor()
  {
    PRINT_NAMED_INFO("VisionProcessingThread.Processor",
                     "Starting Robot VisionProcessingThread::Processor thread...");
    
    _visionSystem->Init(_camCalib);
    
    // Wait for initialization to complete (i.e. Matlab to start up, if needed)
    while(!_visionSystem->IsInitialized()) {
      usleep(500);
    }
    
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
    
    PRINT_NAMED_INFO("VisionProcessingThread.Processor",
                     "Terminated Robot VisionProcessingThread::Processor thread");
  } // Processor()

  /*
  bool VisionProcessingThread::CheckMailbox(MessageFaceDetection& msg)
  {
    AnkiConditionalErrorAndReturnValue(_visionSystem != nullptr, // && _visionSystem->IsInitialized(), false,
                                       "VisionProcessingThread.CheckMailbox.NullVisionSystem",
                                       "CheckMailbox called before vision system instantiated.");// and initialized.");
    return _visionSystem->CheckMailbox(msg);
  }
  */
  
  bool VisionProcessingThread::CheckMailbox(Vision::ObservedMarker& msg)
  {
    AnkiConditionalErrorAndReturnValue(_visionSystem != nullptr /*&& _visionSystem->IsInitialized()*/, false,
                                       "VisionProcessingThread.CheckMailbox.NullVisionSystem",
                                       "CheckMailbox called before vision system instantiated.");// and initialized.");
    return _visionSystem->CheckMailbox(msg);
  }
  
  bool VisionProcessingThread::CheckMailbox(std::pair<Pose3d, TimeStamp_t>& markerPoseWrtCamera)
  {
    AnkiConditionalErrorAndReturnValue(_visionSystem != nullptr /*&& _visionSystem->IsInitialized()*/, false,
                                       "VisionProcessingThread.CheckMailbox.NullVisionSystem",
                                       "CheckMailbox called before vision system instantiated.");// and initialized.");
    return _visionSystem->CheckMailbox(markerPoseWrtCamera);
  }
  
  bool VisionProcessingThread::CheckMailbox(VizInterface::TrackerQuad& msg)
  {
    AnkiConditionalErrorAndReturnValue(_visionSystem != nullptr /*& _visionSystem->IsInitialized()*/, false,
                                       "VisionProcessingThread.CheckMailbox.NullVisionSystem",
                                       "CheckMailbox called before vision system instantiated.");// and initialized.");
    return _visionSystem->CheckMailbox(msg);
  }
  
  bool VisionProcessingThread::CheckMailbox(RobotInterface::PanAndTilt& msg)
  {
    AnkiConditionalErrorAndReturnValue(_visionSystem != nullptr /*& _visionSystem->IsInitialized()*/, false,
                                       "VisionProcessingThread.CheckMailbox.NullVisionSystem",
                                       "CheckMailbox called before vision system instantiated.");// and initialized.");
    return _visionSystem->CheckMailbox(msg);
  }
  
  bool VisionProcessingThread::CheckMailbox(Vision::TrackedFace& msg)
  {
    AnkiConditionalErrorAndReturnValue(_visionSystem != nullptr /*& _visionSystem->IsInitialized()*/, false,
                                       "VisionProcessingThread.CheckMailbox.NullVisionSystem",
                                       "CheckMailbox called before vision system instantiated.");// and initialized.");
    return _visionSystem->CheckMailbox(msg);
  }
  
  
} // namespace Cozmo
} // namespace Anki
