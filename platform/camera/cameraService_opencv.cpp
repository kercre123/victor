/**
 * File: cameraService_mac.cpp
 *
 * Author: chapados
 * Created: 02/07/2018
 *
 * based on androidHAL_android.cpp
 * Author: Kevin Yoon
 * Created: 02/17/2017
 *
 * Description:
 *               Defines interface to a camera system provided by the OS/platform
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "camera/cameraService.h"

#include "coretech/common/shared/array2d_impl.h"

#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/video/tracking.hpp"

#include "util/logging/logging.h"

#include "util/container/fixedCircularBuffer.h"
#include "util/random/randomGenerator.h"

#include <deque>
#include <vector>

#define BLUR_CAPTURED_IMAGES 0

#include "opencv2/imgproc/imgproc.hpp"

namespace Anki {
  namespace Vector {

    namespace { // "Private members"

      cv::VideoCapture cap;
      
      // Lens distortion
      //const bool kUseLensDistortion = false;
      //const f32 kRadialDistCoeff1     = -0.07178328295562293f;
      //const f32 kRadialDistCoeff2     = -0.2195788148163958f;
      //const f32 kRadialDistCoeff3     = 0.13393879360293f;
      //const f32 kTangentialDistCoeff1 = 0.001433240008548796f;
      //const f32 kTangentialDistCoeff2 = 0.001523473592445885f;
      //const f32 kDistCoeffNoiseFrac   = 0.0f; // fraction of the true value to use for uniformly distributed noise (0 to disable)
      CameraCalibration empty_cal_;

      Vision::ImageRGB rgb_; 
      TimeStamp_t cameraStartTime_ms_;
      TimeStamp_t lastImageCapturedTime_ms_;

      bool _skipNextImage = false;

      std::deque<std::shared_ptr<cv::Mat>> image_buffers_;
    } // "private" namespace

    // Definition of static field
    CameraService* CameraService::_instance = nullptr;

    /**
     * Returns the single instance of the object.
     */
    CameraService* CameraService::getInstance() {
      // check if the instance has been created yet
      if (nullptr == _instance) {
        // if not, then create it
        _instance = new CameraService();
      }
      // return the single instance
      return _instance;
    }

    /**
     * Removes instance
     */
    void CameraService::removeInstance() {
      // check if the instance has been created yet
      if (nullptr != _instance) {
        delete _instance;
        _instance = nullptr;
      }
    };

    CameraService::CameraService()
    : _imageSensorCaptureHeight(DEFAULT_CAMERA_RESOLUTION_HEIGHT)
    , _imageSensorCaptureWidth(DEFAULT_CAMERA_RESOLUTION_WIDTH)
    {
          cameraStartTime_ms_ = GetTimeStamp(); // + VISION_TIME_STEP;
          lastImageCapturedTime_ms_ = 0;     
          cap.open(0); 
    }

    CameraService::~CameraService()
    {

    }

    void CameraService::RegisterOnCameraRestartCallback(std::function<void()> callback)
    {
      return;
    }
    
    TimeStamp_t CameraService::GetTimeStamp(void)
    {
      return 0;
    }

    Result CameraService::Update()
    {
      cv::Mat frame;
      cap >> frame;
      if(frame.empty()){
        std::cerr<<"frame is empty"<<std::endl;
      } else {
        cv::Mat frame_small;
        cv::resize(frame, frame_small, cv::Size(CAMERA_SENSOR_RESOLUTION_WIDTH, CAMERA_SENSOR_RESOLUTION_HEIGHT));
        std::shared_ptr<cv::Mat> small_rgb(new cv::Mat);        
        cv::cvtColor(frame_small, *small_rgb, CV_BGR2RGB);
        rgb_ = Vision::ImageRGB(CAMERA_SENSOR_RESOLUTION_HEIGHT, CAMERA_SENSOR_RESOLUTION_WIDTH, small_rgb->data);
        image_buffers_.push_back(small_rgb);
        if (image_buffers_.size() > 10) {
          image_buffers_.pop_front();
        }
      }
      return RESULT_OK;
    }
    
    const CameraCalibration* CameraService::GetHeadCamInfo(void)
    {
      return &empty_cal_;
    }

    void CameraService::CameraSetParameters(u16 exposure_ms, f32 gain)
    {
      // Can't control simulated camera's exposure.

      // TODO: Simulate this somehow?

      return;

    } // HAL::CameraSetParameters()

    void CameraService::CameraSetWhiteBalanceParameters(f32 r_gain, f32 g_gain, f32 b_gain)
    {
      return;
    }

    void CameraService::CameraSetCaptureFormat(Vision::ImageEncoding format)
    {
      return;
    }

    void CameraService::CameraSetCaptureSnapshot(bool start)
    {
      return;
    }

    Result CameraService::InitCamera()
    {
      return RESULT_OK;
    }

    Result CameraService::DeleteCamera()
    {
      return RESULT_OK;
    }

    void CameraService::UnpauseForCameraSetting()
    {
      return;
    }
    
    void CameraService::PauseCamera(bool pause)
    {
      // Technically only need to skip the next image when unpausing but since
      // you can't get images while paused it does not matter that this is being set
      // when pausing
      _skipNextImage = true;
    }

    // Starts camera frame synchronization.
    // Returns true and popuates buffer if we have an available image from at or before atTimestamp_ms.
    bool CameraService::CameraGetFrame(u32 atTimestamp_ms, Vision::ImageBuffer& buffer)
    {
      if (_skipNextImage) {
        _skipNextImage = false;
        return false;
      }

      if (image_buffers_.size() == 0) {
        printf("No images yet\n");
        return false;
      }

      //const TimeStamp_t currentTime_ms = GetTimeStamp();
      buffer = Vision::ImageBuffer(const_cast<u8*>(reinterpret_cast<const u8*>(rgb_.GetDataPointer())),
                                   CAMERA_SENSOR_RESOLUTION_HEIGHT,
                                   CAMERA_SENSOR_RESOLUTION_WIDTH,
                                   Vision::ImageEncoding::RawRGB,
                                   0,
                                   _imageFrameID);
      _imageFrameID++;
      return true;

    } // CameraGetFrame()

    bool CameraService::CameraReleaseFrame(u32 imageID)
    {
      // no-op
      return true;
    }
  } // namespace Vector
} // namespace Anki
