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
#include "visionSystem.h"

#include "anki/vision/basestation/image_impl.h"

#include "anki/common/basestation/utils/logging/logging.h"
#include "anki/common/basestation/utils/helpers/boundedWhile.h"

namespace Anki {
namespace Cozmo {
  
  VisionProcessingThread::VisionProcessingThread()
  : _visionSystem(nullptr)
  , _running(false)
  , _isLocked(false)
  , _wasLastImageProcessed(false)
  {
    
  } // VisionSystem()

  void VisionProcessingThread::Start(Vision::CameraCalibration& camCalib)
  {
    if(_running) {
      PRINT_NAMED_INFO("VisionProcessingThread.Start.Restarting",
                       "Thread already started, call Stop() and then restarting.\n");
      Stop();
    }
    
    _running = true;
    _camCalib = camCalib;
    
    // Note that we're giving the Processor a pointer to "this", so we
    // have to ensure this VisionSystem object outlives the thread.
    _processingThread = std::thread(&VisionProcessingThread::Processor, this);
    _processingThread.detach();
    
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
    /*
    // Now that processing is complete, we can delete any images that we're
    // holding onto
    if(_currentImg != nullptr) {
      delete _currentImg;
      _currentImg = nullptr;
    }
    
    if(_nextImg != nullptr) {
      delete _nextImg;
      _nextImg = nullptr;
    }
     */
  }


  VisionProcessingThread::~VisionProcessingThread()
  {
    Stop();
  } // ~VisionSystem()


  void VisionProcessingThread::SetNextImage(const Vision::Image &image,
                                            const Anki::Cozmo::MessageRobotState &robotState)
  {
    Lock();
    
    // TODO: Avoid all the allocation and deletion here!
    
    /*
    if(_nextImg != nullptr) {
      // There's already a next image queued up. Get rid of it and replace it
      // with this new one.
      PRINT_NAMED_INFO("VisionProcessingThread.SetNextImage.DiscardingImage",
                       "Discarding previously-set next image before processing it.\n");
      delete _nextImg;
    }
     */
    
    //_nextImg = new Vision::Image(image);
    image.CopyDataTo(_nextImg);
    _nextImg.SetTimestamp(image.GetTimestamp());
    
    //_nextImg = Vision::Image(image);
    _nextRobotState = robotState;
    _wasLastImageProcessed = false;
    
    Unlock();
    
  } // SetNextImage()
  
  
  bool VisionProcessingThread::GetCurrentImage(Vision::Image& img)
  {
    if(_running && !_currentImg.IsEmpty()) {
      Lock();
      _currentImg.CopyDataTo(img);
      Unlock();
      img.SetTimestamp(_currentImg.GetTimestamp());
      return true;
    } else {
      img = {};
      return false;
    }
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


  void VisionProcessingThread::Lock()
  {
    // Wait for unlock
    BOUNDED_WHILE(10000, _isLocked) {
      usleep(100);
    }
    
    _isLocked = true;
  }

  void VisionProcessingThread::Unlock()
  {
    _isLocked = false;
  }


  void VisionProcessingThread::Processor()
  {
    PRINT_INFO("Starting Robot VisionProcessingThread::Processor thread...\n");
    
    _visionSystem = new VisionSystem();
    _visionSystem->Init(_camCalib);
    
    // Wait for initialization to complete (i.e. Matlab to start up, if needed)
    while(!_visionSystem->IsInitialized()) {
      usleep(500);
    }
    
    while (_running) {
      
      //if(_currentImg != nullptr) {
      if(!_currentImg.IsEmpty()) {
        // There is an image to be processed:
        
        //assert(_currentImg != nullptr);
        _visionSystem->Update(_currentRobotState, _currentImg);
        _wasLastImageProcessed = true;
        
        // Clear it when done.
        /*
        delete _currentImg;
        _currentImg = nullptr;
         */
        Lock();
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
    
    PRINT_INFO("Terminated Robot VisionProcessingThread::Processor thread\n");
  } // Processor()

  
  bool VisionProcessingThread::CheckMailbox(MessageFaceDetection& msg)
  {
    AnkiConditionalErrorAndReturnValue(_visionSystem != nullptr /*&& _visionSystem->IsInitialized()*/, false,
                                       "VisionProcessingThread.CheckMailbox.NullVisionSystem",
                                       "CheckMailbox called before vision system instantiated.");// and initialized.");
    return _visionSystem->CheckMailbox(msg);
  }
  
  bool VisionProcessingThread::CheckMailbox(MessageVisionMarker& msg)
  {
    AnkiConditionalErrorAndReturnValue(_visionSystem != nullptr /*&& _visionSystem->IsInitialized()*/, false,
                                       "VisionProcessingThread.CheckMailbox.NullVisionSystem",
                                       "CheckMailbox called before vision system instantiated.");// and initialized.");
    return _visionSystem->CheckMailbox(msg);
  }
  
  bool VisionProcessingThread::CheckMailbox(MessageDockingErrorSignal& msg)
  {
    AnkiConditionalErrorAndReturnValue(_visionSystem != nullptr /*&& _visionSystem->IsInitialized()*/, false,
                                       "VisionProcessingThread.CheckMailbox.NullVisionSystem",
                                       "CheckMailbox called before vision system instantiated.");// and initialized.");
    return _visionSystem->CheckMailbox(msg);
  }
  
  bool VisionProcessingThread::CheckMailbox(MessageTrackerQuad& msg)
  {
    AnkiConditionalErrorAndReturnValue(_visionSystem != nullptr /*& _visionSystem->IsInitialized()*/, false,
                                       "VisionProcessingThread.CheckMailbox.NullVisionSystem",
                                       "CheckMailbox called before vision system instantiated.");// and initialized.");
    return _visionSystem->CheckMailbox(msg);
  }
  
  bool VisionProcessingThread::CheckMailbox(MessagePanAndTiltHead& msg)
  {
    AnkiConditionalErrorAndReturnValue(_visionSystem != nullptr /*& _visionSystem->IsInitialized()*/, false,
                                       "VisionProcessingThread.CheckMailbox.NullVisionSystem",
                                       "CheckMailbox called before vision system instantiated.");// and initialized.");
    return _visionSystem->CheckMailbox(msg);
  }
  
  
} // namespace Cozmo
} // namespace Anki
