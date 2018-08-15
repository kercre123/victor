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
#include "util/logging/logging.h"

#include "util/random/randomGenerator.h"

#include <vector>

#include <webots/Supervisor.hpp>
#include <webots/Camera.hpp>

#define BLUR_CAPTURED_IMAGES 1

#if BLUR_CAPTURED_IMAGES
#include "opencv2/imgproc/imgproc.hpp"
#endif

#ifndef SIMULATOR
#error SIMULATOR should be defined by any target using cameraService_mac.cpp
#endif

namespace Anki {
  namespace Vector {

    namespace { // "Private members"

      // Has SetSupervisor() been called yet?
      bool _engineSupervisorSet = false;

      // Current supervisor (if any)
      webots::Supervisor* _engineSupervisor = nullptr;

      // Const parameters / settings
      const u32 VISION_TIME_STEP = 65; // This should be a multiple of the world's basic time step!

      // Cameras / Vision Processing
      webots::Camera* headCam_;

      // Lens distortion
      const bool kUseLensDistortion = false;
      const f32 kRadialDistCoeff1     = -0.07178328295562293f;
      const f32 kRadialDistCoeff2     = -0.2195788148163958f;
      const f32 kRadialDistCoeff3     = 0.13393879360293f;
      const f32 kTangentialDistCoeff1 = 0.001433240008548796f;
      const f32 kTangentialDistCoeff2 = 0.001523473592445885f;
      const f32 kDistCoeffNoiseFrac   = 0.0f; // fraction of the true value to use for uniformly distributed noise (0 to disable)

      std::vector<u8> imageBuffer_;
      TimeStamp_t cameraStartTime_ms_;
      TimeStamp_t lastImageCapturedTime_ms_;

    } // "private" namespace


#pragma mark --- Simulated Hardware Method Implementations ---

    // Declarations
    void FillCameraInfo(const webots::Camera *camera, CameraCalibration &info);


    // Definition of static field
    CameraService* CameraService::_instance = nullptr;

    /**
     * Returns the single instance of the object.
     */
    CameraService* CameraService::getInstance() {
      // Did you remember to call SetSupervisor()?
      DEV_ASSERT(_engineSupervisorSet, "cameraService_mac.NoSupervisorSet");
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

    void CameraService::SetSupervisor(webots::Supervisor *sup)
    {
      _engineSupervisor = sup;
      _engineSupervisorSet = true;
    }

    CameraService::CameraService()
    {
      if (nullptr != _engineSupervisor) {

        // Is the step time defined in the world file >= than the robot time? It should be!
        DEV_ASSERT(ROBOT_TIME_STEP_MS >= _engineSupervisor->getBasicTimeStep(), "cameraService_mac.UnexpectedTimeStep");

        if (VISION_TIME_STEP % static_cast<u32>(_engineSupervisor->getBasicTimeStep()) != 0) {
          PRINT_NAMED_WARNING("cameraService_mac.InvalidVisionTimeStep",
                              "VISION_TIME_STEP (%d) must be a multiple of the world's basic timestep (%.0f).",
                              VISION_TIME_STEP, _engineSupervisor->getBasicTimeStep());
          return;
        }

        // Head Camera
        headCam_ = _engineSupervisor->getCamera("HeadCamera");
        if (nullptr != headCam_) {
          headCam_->enable(VISION_TIME_STEP);
          FillCameraInfo(headCam_, headCamInfo_);

          // HACK: Figure out when first camera image will actually be taken (next
          // timestep from now), so we can reference to it when computing frame
          // capture time from now on.
          // TODO: Not sure from Cyberbotics support message whether this should include "+ VISION_TIME_STEP" or not...
          cameraStartTime_ms_ = GetTimeStamp(); // + VISION_TIME_STEP;
          lastImageCapturedTime_ms_ = 0;
        }
      }
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
      if (nullptr != _engineSupervisor) {
        return static_cast<TimeStamp_t>(_engineSupervisor->getTime() * 1000.0);
      }
      return 0;
    }

    Result CameraService::Update()
    {
      if (nullptr != _engineSupervisor) {
        if (_engineSupervisor->step(Vector::ROBOT_TIME_STEP_MS) == -1) {
          return RESULT_FAIL;
        }
        // AudioUpdate();
      }
      return RESULT_OK;
    }


    // Helper function to create a CameraInfo struct from Webots camera properties:
    void FillCameraInfo(const webots::Camera *camera, CameraCalibration &info)
    {

      const u16 nrows  = static_cast<u16>(camera->getHeight());
      const u16 ncols  = static_cast<u16>(camera->getWidth());
      const f32 width  = static_cast<f32>(ncols);
      const f32 height = static_cast<f32>(nrows);
      //f32 aspect = width/height;

      const f32 fov_hor = camera->getFov();

      // Compute focal length from simulated camera's reported FOV:
      const f32 f = width / (2.f * std::tan(0.5f*fov_hor));

      // There should only be ONE focal length, because simulated pixels are
      // square, so no need to compute/define a separate fy
      //f32 fy = height / (2.f * std::tan(0.5f*fov_ver));

      info.focalLength_x = f;
      info.focalLength_y = f;
      info.center_x      = 0.5f*(width-1);
      info.center_y      = 0.5f*(height-1);
      info.skew          = 0.f;
      info.nrows         = nrows;
      info.ncols         = ncols;
      info.distCoeffs.fill(0.f);

      if(kUseLensDistortion)
      {
        info.distCoeffs[0] = kRadialDistCoeff1;
        info.distCoeffs[1] = kRadialDistCoeff2;
        info.distCoeffs[2] = kTangentialDistCoeff1;
        info.distCoeffs[3] = kTangentialDistCoeff2;
        info.distCoeffs[4] = kRadialDistCoeff3;

        if(Util::IsFltGTZero(kDistCoeffNoiseFrac))
        {
          // Simulate not having perfectly calibrated distortion coefficients
          static Util::RandomGenerator rng(0);
          for(s32 i=0; i<5; ++i) {
            info.distCoeffs[i] *= rng.RandDblInRange(1.f-kDistCoeffNoiseFrac, 1.f+kDistCoeffNoiseFrac);
          }
        }
      }
    } // FillCameraInfo

    const CameraCalibration* CameraService::GetHeadCamInfo(void)
    {
      return &headCamInfo_;
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

    void CameraService::CameraSetCaptureFormat(ImageEncoding format)
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

    void CameraService::DeleteCamera() {
    }

    // Starts camera frame synchronization
    bool CameraService::CameraGetFrame(u8*& frame, u32& imageID, TimeStamp_t& imageCaptureSystemTimestamp_ms, ImageEncoding& format)
    {
      if (nullptr == headCam_) {
        return false;
      }

      const TimeStamp_t currentTime_ms = GetTimeStamp();

      // This computation is based on Cyberbotics support's explanation for how to compute
      // the actual capture time of the current available image from the simulated camera
      // *except* I seem to need the extra "- VISION_TIME_STEP" for some reason.
      // (The available frame is still one frame behind? I.e. we are just *about* to capture
      //  the next one?)
      const TimeStamp_t currentImageTime_ms = (std::floor((currentTime_ms-cameraStartTime_ms_)/VISION_TIME_STEP) * VISION_TIME_STEP
                                               + cameraStartTime_ms_ - VISION_TIME_STEP);

      // Have we already sent the currently-available image?
      if(lastImageCapturedTime_ms_ == currentImageTime_ms)
      {
        return false;
      }

      imageBuffer_.resize(headCamInfo_.nrows * headCamInfo_.ncols * 3);
      frame = imageBuffer_.data();

      const u8* image = headCam_->getImage();

      DEV_ASSERT(image != NULL, "cameraService_mac.CameraGetFrame.NullImagePointer");
      DEV_ASSERT_MSG(headCam_->getWidth() == headCamInfo_.ncols,
                     "cameraService_mac.CameraGetFrame.MismatchedImageWidths",
                     "HeadCamInfo:%d HeadCamWidth:%d", headCamInfo_.ncols, headCam_->getWidth());

      u8* pixel = frame;
      for (s32 y=0; y < headCamInfo_.nrows; y++) {
        for (s32 x=0; x < headCamInfo_.ncols; x++) {
          pixel[0] = webots::Camera::imageGetRed(image,   headCamInfo_.ncols, x, y);
          pixel[1] = webots::Camera::imageGetGreen(image, headCamInfo_.ncols, x, y);
          pixel[2] = webots::Camera::imageGetBlue(image,  headCamInfo_.ncols, x, y);
          pixel+=3;
        }
      }

      if (kUseLensDistortion)
      {
        // Apply radial/lens distortion. Note that cv::remap uses in inverse lookup to find where the pixels in
        // the output (distorted) image came from in the source. So we have to compute the inverse distortion here.
        // We do that using cv::undistortPoints to create the necessary x/y maps for remap:
        static cv::Mat_<f32> x_undistorted, y_undistorted;
        if(x_undistorted.empty())
        {
          // Compute distortion maps on first use
          std::vector<cv::Point2f> points;
          points.reserve(headCamInfo_.nrows * headCamInfo_.ncols);

          for (s32 i=0; i < headCamInfo_.nrows; i++) {
            for (s32 j=0; j < headCamInfo_.ncols; j++) {
              points.emplace_back(j,i);
            }
          }

          const std::vector<f32> distCoeffs{
            kRadialDistCoeff1, kRadialDistCoeff2, kTangentialDistCoeff1, kTangentialDistCoeff2, kRadialDistCoeff3
          };
          const cv::Matx<f32,3,3> cameraMatrix(headCamInfo_.focalLength_x, 0.f, headCamInfo_.center_x,
                                               0.f, headCamInfo_.focalLength_y, headCamInfo_.center_y,
                                               0.f, 0.f, 1.f);

          cv::undistortPoints(points, points, cameraMatrix, distCoeffs, cv::noArray(), cameraMatrix);

          x_undistorted.create(headCamInfo_.nrows, headCamInfo_.ncols);
          y_undistorted.create(headCamInfo_.nrows, headCamInfo_.ncols);
          std::vector<cv::Point2f>::const_iterator pointIter = points.begin();
          for (s32 i=0; i < headCamInfo_.nrows; i++)
          {
            f32* x_i = x_undistorted.ptr<f32>(i);
            f32* y_i = y_undistorted.ptr<f32>(i);

            for (s32 j=0; j < headCamInfo_.ncols; j++)
            {
              x_i[j] = pointIter->x;
              y_i[j] = pointIter->y;
              ++pointIter;
            }
          }
        }
        cv::Mat  cvFrame(headCamInfo_.nrows, headCamInfo_.ncols, CV_8UC3, frame);
        cv::remap(cvFrame, cvFrame, x_undistorted, y_undistorted, CV_INTER_LINEAR);
      }

      if (BLUR_CAPTURED_IMAGES)
      {
        // Add some blur to simulated images
        cv::Mat cvImg(headCamInfo_.nrows, headCamInfo_.ncols, CV_8UC3, frame);
        cv::GaussianBlur(cvImg, cvImg, cv::Size(0,0), 0.75f);
      }

      imageCaptureSystemTimestamp_ms = currentImageTime_ms;

      imageID = _imageFrameID;
      _imageFrameID++;

      // Mark that we've already sent the image for the current time
      lastImageCapturedTime_ms_ = currentImageTime_ms;

      format = ImageEncoding::RawRGB;
      
      return true;

    } // CameraGetFrame()

    bool CameraService::CameraReleaseFrame(u32 imageID)
    {
      // no-op
      return true;
    }
  } // namespace Vector
} // namespace Anki
