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

#ifndef WEBOTS

#include "camera/cameraService.h"

#include "coretech/common/shared/array2d.h"

#include "util/logging/logging.h"

#include "util/container/fixedCircularBuffer.h"
#include "util/random/randomGenerator.h"

#include <vector>

#define BLUR_CAPTURED_IMAGES 0

#include "opencv2/imgproc/imgproc.hpp"

#ifndef MACOSX
#error MACOSX should be defined by any target using cameraService_mac.cpp
#endif

namespace Anki {
  namespace Vector {

#pragma mark --- Simulated Hardware Method Implementations ---

    // Apply lens distortion to the RGB image in frame, using the information from headCamInfo
    void ApplyLensDistortion(u8* frame, const CameraCalibration& headCamInfo);

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
    {
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
      return RESULT_OK;
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
    }

    // Starts camera frame synchronization.
    // Returns true and popuates buffer if we have an available image from at or before atTimestamp_ms.
    bool CameraService::CameraGetFrame(u32 atTimestamp_ms, Vision::ImageBuffer& buffer)
    {
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

#endif // ndef WEBOTS
