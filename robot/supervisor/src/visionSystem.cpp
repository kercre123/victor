/**
* File: visionSystem.cpp
*
* Author: Andrew Stein
* Date:   (various)
*
* Description: High-level module that controls the vision system and switches
*              between fiducial detection and tracking and feeds results to
*              main execution thread via message mailboxes.
*
* Copyright: Anki, Inc. 2014
**/

#include "anki/common/robot/config.h"
// Coretech Vision Includes
#include "anki/vision/MarkerCodeDefinitions.h"
#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/fiducialMarkers.h"
#include "anki/vision/robot/imageProcessing.h"
#include "anki/vision/robot/perspectivePoseEstimation.h"
#include "anki/vision/robot/classifier.h"
#include "anki/vision/robot/lbpcascade_frontalface.h"
#include "anki/vision/robot/cameraImagingPipeline.h"

// CoreTech Common Includes
#include "anki/common/shared/radians.h"
#include "anki/common/robot/benchmarking.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/utilities.h"

// Cozmo-Specific Library Includes
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/robot/hal.h"

// Local Cozmo Includes
#include "headController.h"
#include "imuFilter.h"
#include "matlabVisualization.h"
#include "messages.h"
#include "localization.h"
#include "visionSystem.h"
#include "visionDebugStream.h"

#if USE_MATLAB_TRACKER || USE_MATLAB_DETECTOR
#include "matlabVisionProcessor.h"
#endif

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE && !USE_APPROXIMATE_DOCKING_ERROR_SIGNAL
#error Affine tracker requires that USE_APPROXIMATE_DOCKING_ERROR_SIGNAL = 1.
#endif

static bool isInitialized_ = false;



namespace Anki {
  namespace Cozmo {
    
    namespace VisionSystem {
      using namespace Embedded;

#if USE_COMPRESSION_FOR_SENDING_IMAGES
      void CompressAndSendImage(const Array<u8> &img, const TimeStamp_t captureTime)
      {
        static u32 imgID = 0;
        const cv::vector<int> compressionParams = {
          CV_IMWRITE_JPEG_QUALITY, IMAGE_SEND_JPEG_COMPRESSION_QUALITY
        };
        
        cv::Mat cvImg;
        cvImg = cv::Mat(img.get_size(0), img.get_size(1)/3, CV_8UC3, const_cast<void*>(img.get_buffer()));
        cvtColor(cvImg, cvImg, CV_BGR2RGB);

        cv::vector<u8> compressedBuffer;
        cv::imencode(".jpg",  cvImg, compressedBuffer, compressionParams);
        
        const u32 numTotalBytes = compressedBuffer.size();
        
        //PRINT("Sending frame with capture time = %d at time = %d\n", captureTime, HAL::GetTimeStamp());
        Messages::ImageChunk m;
        m.frameTimeStamp = captureTime;
        m.resolution = captureResolution_;
        m.imageId = ++imgID;
        m.chunkId = 0;
        m.chunkSize = IMAGE_CHUNK_SIZE;
        m.imageChunkCount = ceilf((f32)numTotalBytes / IMAGE_CHUNK_SIZE);
        m.imageEncoding = Vision::IE_JPEG_COLOR;
        
        u32 totalByteCnt = 0;
        u32 chunkByteCnt = 0;
        
        for(s32 i=0; i<numTotalBytes; ++i)
        {
          m.data[chunkByteCnt] = compressedBuffer[i];
          
          ++chunkByteCnt;
          ++totalByteCnt;
          
          if (chunkByteCnt == IMAGE_CHUNK_SIZE) {
            //PRINT("Sending image chunk %d\n", m.chunkId);
            HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::ImageChunk), &m);
            ++m.chunkId;
            chunkByteCnt = 0;
          } else if (totalByteCnt == numTotalBytes) {
            // This should be the last message!
            //PRINT("Sending LAST image chunk %d\n", m.chunkId);
            m.chunkSize = chunkByteCnt;
            HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::ImageChunk), &m);
          }
        } // for each byte in the compressed buffer
        
      } // CompressAndSendImage()
#endif // USE_COMPRESSION_FOR_SENDING_IMAGES
      

      f32 GetVerticalFOV() {
        return headCamFOV_ver_;
      }

      f32 GetHorizontalFOV() {
        return headCamFOV_hor_;
      }

      const FaceDetectionParameters& GetFaceDetectionParams() {
        return faceDetectionParameters_;
      }
  
      template<typename PRECISION>
      static Result GetCamPoseWrtRobot(Array<PRECISION>& RcamWrtRobot,
        Point3<PRECISION>& TcamWrtRobot)
      {
        AnkiConditionalErrorAndReturnValue(RcamWrtRobot.get_size(0)==3 &&
          RcamWrtRobot.get_size(1)==3,
          RESULT_FAIL_INVALID_SIZE,
          "VisionSystem::GetCamPoseWrtRobot",
          "Rotation matrix must already be 3x3.");

        const f32 headAngle = HeadController::GetAngleRad();
        const f32 cosH = cosf(headAngle);
        const f32 sinH = sinf(headAngle);

        RcamWrtRobot[0][0] = 0;  RcamWrtRobot[0][1] = sinH;  RcamWrtRobot[0][2] = cosH;
        RcamWrtRobot[1][0] = -1; RcamWrtRobot[1][1] = 0;     RcamWrtRobot[1][2] = 0;
        RcamWrtRobot[2][0] = 0;  RcamWrtRobot[2][1] = -cosH; RcamWrtRobot[2][2] = sinH;

        TcamWrtRobot.x = HEAD_CAM_POSITION[0]*cosH - HEAD_CAM_POSITION[2]*sinH + NECK_JOINT_POSITION[0];
        TcamWrtRobot.y = 0;
        TcamWrtRobot.z = HEAD_CAM_POSITION[2]*cosH + HEAD_CAM_POSITION[0]*sinH + NECK_JOINT_POSITION[2];

        return RESULT_OK;
      }

      Result GetWithRespectToRobot(const Embedded::Point3<f32>& pointWrtCamera,
        Embedded::Point3<f32>&       pointWrtRobot)
      {
        Point3<f32> TcamWrtRobot;

        Result lastResult;
        if((lastResult = GetCamPoseWrtRobot(RcamWrtRobot_, TcamWrtRobot)) != RESULT_OK) {
          return lastResult;
        }

        pointWrtRobot = RcamWrtRobot_*pointWrtCamera + TcamWrtRobot;

        return RESULT_OK;
      }

      Result GetWithRespectToRobot(const Embedded::Array<f32>&  rotationWrtCamera,
        const Embedded::Point3<f32>& translationWrtCamera,
        Embedded::Array<f32>&        rotationWrtRobot,
        Embedded::Point3<f32>&       translationWrtRobot)
      {
        Point3<f32> TcamWrtRobot;

        Result lastResult;
        if((lastResult = GetCamPoseWrtRobot(RcamWrtRobot_, TcamWrtRobot)) != RESULT_OK) {
          return lastResult;
        }

        if((lastResult = Matrix::Multiply(RcamWrtRobot_, rotationWrtCamera, rotationWrtRobot)) != RESULT_OK) {
          return lastResult;
        }

        translationWrtRobot = RcamWrtRobot_*translationWrtCamera + TcamWrtRobot;

        return RESULT_OK;
      }

      
      // TODO: Have this taken care of inside HAL::CameraGetFrame()
      static TimeStamp_t GetFrameCaptureTime()
      {
#       ifdef SIMULATOR
        // Round down to the most recent frame capture time, according to the VISION_TIME_STEP
        const TimeStamp_t frameCaptureTime = std::floor((HAL::GetTimeStamp()-HAL::GetCameraStartTime())/VISION_TIME_STEP)*VISION_TIME_STEP + HAL::GetCameraStartTime();
#       else
        const TimeStamp_t frameCaptureTime = HAL::GetTimeStamp();
#       endif
        
        return frameCaptureTime;
      }

      // This is the regular Update() call
      Result Update(const Messages::RobotState robotState)
      {
        Result lastResult = RESULT_OK;

        // This should be called from elsewhere first, but calling it again won't hurt
        Init();

        frameNumber++;

        // no-op on real hardware
        if(!Simulator::IsFrameReady()) {
          return RESULT_OK;
        }
        
        // Make sure that we send the robot state message associated with the
        // image we are about to process.
        if(mode_ != VISION_MODE_SEND_IMAGES_TO_BASESTATION) {
          Messages::SendRobotStateMsg(&robotState);
        }
        
        UpdateRobotState(robotState);
        
        // If SetMarkerToTrack() was called by main() during previous Update(),
        // actually swap in the new marker now.
        lastResult = UpdateMarkerToTrack();
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "VisionSystem::Update()", "UpdateMarkerToTrack failed.\n");

        // Use the timestamp of passed-in robot state as our frame capture's
        // timestamp.  This is not totally correct, since the image will be
        // grabbed some (trivial?) number of cycles later, once we get to the
        // CameraGetFrame() calls below.  But this enforces, for now, that we
        // always send a RobotState message off to basestation with a matching
        // timestamp to every VisionMarker message.
        //const TimeStamp_t imageTimeStamp = HAL::GetTimeStamp();
        const TimeStamp_t imageTimeStamp = robotState.timestamp;

        // TODO: Do I even need the #ifdef SIMULATOR anymore? is this code even compiled into the real robot?
#       ifdef SIMULATOR
        
        Simulator::SetSendWifiImageReadyTime();
        
        TimeStamp_t currentTime = HAL::GetTimeStamp();
        
        // This computation is based on Cyberbotics support's explaination for how to compute
        // the actual capture time of the current available image from the simulated
        // camera, *except* I seem to need the extra "- VISION_TIME_STEP" for some reason.
        // (The available frame is still one frame behind? I.e. we are just *about* to capture
        //  the next one?)
        TimeStamp_t currentImageTime = std::floor((currentTime-HAL::GetCameraStartTime())/VISION_TIME_STEP) * VISION_TIME_STEP + HAL::GetCameraStartTime() - VISION_TIME_STEP;
        
        // Keep up with the capture time of the last image we sent
        static TimeStamp_t lastImageSentTime = 0;
        
        // Have we already sent the currently-available image?
        if(lastImageSentTime != currentImageTime)
        {
          // Nope, so get the (new) available frame from the camera:
          VisionMemory::ResetBuffers();
          const s32 captureHeight = Vision::CameraResInfo[captureResolution_].height;
          const s32 captureWidth  = Vision::CameraResInfo[captureResolution_].width * 3; // The "*3" is a hack to get enough room for color
          
          Array<u8> image(captureHeight, captureWidth,
                          VisionMemory::offchipScratch_, Flags::Buffer(false,false,false));
          
          HAL::CameraGetFrame(reinterpret_cast<u8*>(image.get_buffer()),
                              captureResolution_, false);
          
          //
          // BEGIN camera/image adjustment stuff (exposure/vignetting...)
          //
          BeginBenchmark("VisionSystem_CameraImagingPipeline");
          
          if(vignettingCorrection == VignettingCorrection_Software) {
            BeginBenchmark("VisionSystem_CameraImagingPipeline_Vignetting");
            
            MemoryStack onchipScratch_local = VisionMemory::onchipScratch_;
            FixedLengthList<f32> polynomialParameters(5, onchipScratch_local, Flags::Buffer(false, false, true));
            
            for(s32 i=0; i<5; i++)
              polynomialParameters[i] = vignettingCorrectionParameters[i];
            
            CorrectVignetting(image, polynomialParameters);
            
            EndBenchmark("VisionSystem_CameraImagingPipeline_Vignetting");
          } // if(vignettingCorrection == VignettingCorrection_Software)
          
          if(autoExposure_enabled && (frameNumber % autoExposure_adjustEveryNFrames) == 0) {
            BeginBenchmark("VisionSystem_CameraImagingPipeline_AutoExposure");
            
            ComputeBestCameraParameters(
                                        image,
                                        Rectangle<s32>(0, image.get_size(1)-1, 0, image.get_size(0)-1),
                                        autoExposure_integerCountsIncrement,
                                        autoExposure_highValue,
                                        autoExposure_percentileToMakeHigh,
                                        autoExposure_minExposureTime, autoExposure_maxExposureTime,
                                        autoExposure_tooHighPercentMultiplier,
                                        exposureTime_,
                                        VisionMemory::ccmScratch_);
            
            EndBenchmark("VisionSystem_CameraImagingPipeline_AutoExposure");
          }
          
          HAL::CameraSetParameters(exposureTime_, vignettingCorrection == VignettingCorrection_CameraHardware);
          
          EndBenchmark("VisionSystem_CameraImagingPipeline");
          
          //
          // END camera/image adjustment stuff
          //
          
          // Send the image, with its actual capture time (not the current system time)
#             if USE_COMPRESSION_FOR_SENDING_IMAGES
          CompressAndSendImage(image, currentImageTime);
#             else
          DownsampleAndSendImage(image, currentImageTime);
#             endif
          // Turn off image sending if sending single image only.
          if (imageSendMode_ == ISM_SINGLE_SHOT) {
            imageSendMode_ = ISM_OFF;
          }
          
          //PRINT("Sending state message from time = %d to correspond to image at time = %d\n",
          //      robotState.timestamp, currentImageTime);
          
          // Mark that we've already sent the image for the current time
          lastImageSentTime = currentImageTime;
        } // if(lastImageSentTime != currentImageTime)
        
#       endif // ifdef SIMULATOR
        
      } // Update()
    
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki
