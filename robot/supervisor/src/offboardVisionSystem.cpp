/**
* File: offboardVisionSystem.cpp
*
* Author: Andrew Stein
* Date:   (various)
*
* Description: High-level module that controls the vision system and switches
*              between fiducial detection and tracking and feeds results to
*              main execution thread via message mailboxes.
*
* This version of the interface relays data collected via messages from an
* offboard vision processor.
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
    // Hack for hardware version 2
    namespace HAL {
      extern int UARTGetFreeSpace();
      extern int UARTGetWriteBufferSize();
    }
  
    namespace VisionSystem {
      using namespace Embedded;

      typedef enum {
        VISION_MODE_IDLE,
        VISION_MODE_SEND_IMAGES_TO_BASESTATION,
        VISION_MODE_LOOKING_FOR_MARKERS,
        VISION_MODE_TRACKING,
        VISION_MODE_DETECTING_FACES,
        
        VISION_MODE_DEFAULT = VISION_MODE_SEND_IMAGES_TO_BASESTATION
      } VisionSystemMode;

#if 0
#pragma mark --- VisionMemory ---
#endif

      namespace VisionMemory
      {
        static const s32 MAX_MARKERS = 100; // TODO: this should probably be in visionParameters
        static const s32 CCM_BUFFER_SIZE     = 5000; // Make this somehow relate to MAX_MARKERS plus whatever FixedLengthList overhead
        static CCM char ccmBuffer[CCM_BUFFER_SIZE];
        static MemoryStack ccmScratch_;

        // Markers is the one things that can move between functions, so it is always allocated in memory
        static FixedLengthList<VisionMarker> markers_;

        // WARNING: ResetBuffers should be used with caution
        static Result ResetBuffers()
        {
          ccmScratch_     = MemoryStack(ccmBuffer, CCM_BUFFER_SIZE);

          if(!ccmScratch_.IsValid()) {
            PRINT("Error: InitializeScratchBuffers\n");
            return RESULT_FAIL;
          }

          markers_ = FixedLengthList<VisionMarker>(VisionMemory::MAX_MARKERS, ccmScratch_);

          return RESULT_OK;
        }

        static Result Initialize()
        {
          return ResetBuffers();
        }
      } // namespace VisionMemory

      // This private namespace stores all the "member" or "state" variables
      // with scope restricted to this file. There should be no globals
      // defined outside this namespace.
      // TODO: I don't think we really need _both_ a private namespace and static
      namespace {
        enum VignettingCorrection
        {
          VignettingCorrection_Off,
          VignettingCorrection_CameraHardware,
          VignettingCorrection_Software
        };

        // The tracker can fail to converge this many times before we give up
        // and reset the docker
        // TODO: Move this to visionParameters
        static const s32 MAX_TRACKING_FAILURES = 1;

        static const Anki::Cozmo::HAL::CameraInfo* headCamInfo_;
        static f32 headCamFOV_ver_;
        static f32 headCamFOV_hor_;
        static Array<f32> RcamWrtRobot_;

        static VisionSystemMode mode_;

        // Camera parameters
        // TODO: Should these be moved to (their own struct in) visionParameters.h/cpp?
        static f32 exposureTime_;

#ifdef SIMULATOR
        // Simulator doesn't need vignetting correction on by default
        static VignettingCorrection vignettingCorrection = VignettingCorrection_Off;
#else
        static VignettingCorrection vignettingCorrection = VignettingCorrection_Off;
#endif
        // For OV7725 (cozmo 2.0)
        //static const f32 vignettingCorrectionParameters[5] = {1.56852140958887f, -0.00619880766167132f, -0.00364222219719291f, 2.75640497906470e-05f, 1.75476361058157e-05f}; //< for vignettingCorrection == VignettingCorrection_Software, computed by fit2dCurve.m
        
        // For OV7739 (cozmo 2.1)
        // TODO: figure these out
        static const f32 vignettingCorrectionParameters[5] = {0,0,0,0,0};

        static s32 frameNumber;
        static bool autoExposure_enabled = true;
        
        // TEMP: Un-const-ing these so that we can adjust them from basestation for dev purposes.
        /*
        static const s32 autoExposure_integerCountsIncrement = 3;
        static const f32 autoExposure_minExposureTime = 0.02f;
        static const f32 autoExposure_maxExposureTime = 0.98f;
        static const u8 autoExposure_highValue = 250;
        static const f32 autoExposure_percentileToMakeHigh = 0.97f;
        static const s32 autoExposure_adjustEveryNFrames = 1;
         */
        static s32 autoExposure_integerCountsIncrement = 3;
        static f32 autoExposure_minExposureTime = 0.02f;
        static f32 autoExposure_maxExposureTime = 0.50f;
        static u8 autoExposure_highValue = 250;
        static f32 autoExposure_percentileToMakeHigh = 0.95f;
        static f32 autoExposure_tooHighPercentMultiplier = 0.7f;
        static s32 autoExposure_adjustEveryNFrames = 2;
        
        static bool wifiCamera_limitFramerate = true;

        // Tracking marker related members
        struct MarkerToTrack {
          Anki::Vision::MarkerType  type;
          f32                       width_mm;
          Point2f                   imageCenter;
          f32                       imageSearchRadius;
          bool                      checkAngleX;

          MarkerToTrack();
          bool IsSpecified() const;
          void Clear();
          bool Matches(const VisionMarker& marker) const;
        };

        static MarkerToTrack markerToTrack_;
        static MarkerToTrack newMarkerToTrack_;
        static bool          newMarkerToTrackWasProvided_ = false;

        static Quadrilateral<f32>          trackingQuad_;
        static s32                         numTrackFailures_ ;
        
        static Point3<P3P_PRECISION>       canonicalMarker3d_[4];

        // Snapshots of robot state
        static bool wasCalledOnce_, havePreviousRobotState_;
        static Messages::RobotState robotState_, prevRobotState_;

        // Parameters defined in visionParameters.h
        static DetectFiducialMarkersParameters detectionParameters_;
        static TrackerParameters               trackerParameters_;
        static FaceDetectionParameters         faceDetectionParameters_;
        static Vision::CameraResolution        captureResolution_;

        // For sending images to basestation
        static ImageSendMode_t                 imageSendMode_ = ISM_OFF;
        static Vision::CameraResolution        nextSendImageResolution_ = Vision::CAMERA_RES_NONE;

        // For taking snapshots
        static bool  isWaitingOnSnapshot_;
        static bool* isSnapshotReady_;
        Embedded::Rectangle<s32> snapshotROI_;
        s32 snapshotSubsample_;
        Embedded::Array<u8>* snapshot_;
        
        /* Only using static members of SimulatorParameters now
        #ifdef SIMULATOR
        static SimulatorParameters             simulatorParameters_;
        #endif
        */

        //
        // Implementation of MarkerToTrack methods:
        //

        MarkerToTrack::MarkerToTrack()
        {
          Clear();
        }

        inline bool MarkerToTrack::IsSpecified() const {
          return type != Anki::Vision::MARKER_UNKNOWN;
        }

        void MarkerToTrack::Clear() {
          type        = Anki::Vision::MARKER_UNKNOWN;
          width_mm    = 0;
          imageCenter = Point2f(-1.f, -1.f);
          imageSearchRadius = -1.f;
          checkAngleX = true;
        }

        bool MarkerToTrack::Matches(const VisionMarker& marker) const
        {
          bool doesMatch = false;

          if(marker.markerType == this->type) {
            if(this->imageCenter.x >= 0.f && this->imageCenter.y >= 0.f &&
              this->imageSearchRadius > 0.f)
            {
              // There is an image position specified, check to see if the
              // marker's centroid is close enough to it
              Point2f centroid = marker.corners.ComputeCenter<f32>();
              if( (centroid - this->imageCenter).Length() < this->imageSearchRadius ) {
                doesMatch = true;
              }
            } else {
              // No image position specified, just return true since the
              // types match
              doesMatch = true;
            }
          }

          return doesMatch;
        } // MarkerToTrack::Matches()
      } // private namespace for VisionSystem state

#if 0
#pragma mark --- Simulator-Related Definitions ---
#endif
      // This little namespace is just for simulated processing time for
      // tracking and detection (since those run far faster in simulation on
      // a PC than they do on embedded hardware. Basically, this is used by
      // Update() below to wait until a frame is ready before proceeding.
      namespace Simulator {
#ifdef SIMULATOR
        static u32 frameReadyTime_;

        static Result Initialize() {
          frameReadyTime_ = 0;
          return RESULT_OK;
        }

        // Returns true if we are past the last set time for simulated processing
        static bool IsFrameReady() {
          return (HAL::GetMicroCounter() >= frameReadyTime_);
        }

        static void SetDetectionReadyTime() {
          frameReadyTime_ = HAL::GetMicroCounter() + SimulatorParameters::FIDUCIAL_DETECTION_PERIOD_US;
        }
        static void SetTrackingReadyTime() {
          frameReadyTime_ = HAL::GetMicroCounter() + SimulatorParameters::TRACK_BLOCK_PERIOD_US;
        }
        static void SetFaceDetectionReadyTime() {
          frameReadyTime_ = HAL::GetMicroCounter() + SimulatorParameters::FACE_DETECTION_PERIOD_US;
        }
        static void SetSendWifiImageReadyTime() {
          frameReadyTime_ = HAL::GetMicroCounter() + SimulatorParameters::SEND_WIFI_IMAGE_PERIOD_US;
        }
#else
        static Result Initialize() { return RESULT_OK; }
        static bool IsFrameReady() { return true; }
        static void SetDetectionReadyTime() { }
        static void SetTrackingReadyTime() { }
        static void SetFaceDetectionReadyTime() {}
        static void SetSendWifiImageReadyTime() {}
#endif
      } // namespace Simulator

#if 0
#pragma mark --- Private (Static) Helper Function Implementations ---
#endif

      static Quadrilateral<f32> GetTrackerQuad(MemoryStack scratch)
      {
#if USE_MATLAB_TRACKER
        return MatlabVisionProcessor::GetTrackerQuad();
#else
        assert(false); while(true); // XXX: Implement me for offboard version!        
        //return tracker_.get_transformation().get_transformedCorners(scratch);
#endif
      } // GetTrackerQuad()

      static Result UpdateRobotState(const Messages::RobotState newRobotState)
      {
        prevRobotState_ = robotState_;
        robotState_     = newRobotState;

        if(wasCalledOnce_) {
          havePreviousRobotState_ = true;
        } else {
          wasCalledOnce_ = true;
        }

        return RESULT_OK;
      } // UpdateRobotState()

      static void GetPoseChange(f32& xChange, f32& yChange, Radians& angleChange)
      {
        AnkiAssert(havePreviousRobotState_);

        angleChange = Radians(robotState_.pose_angle) - Radians(prevRobotState_.pose_angle);

        // Position change in world (mat) coordinates
        const f32 dx = robotState_.pose_x - prevRobotState_.pose_x;
        const f32 dy = robotState_.pose_y - prevRobotState_.pose_y;

        // Get change in robot coordinates
        const f32 cosAngle = cosf(-prevRobotState_.pose_angle);
        const f32 sinAngle = sinf(-prevRobotState_.pose_angle);
        xChange = dx*cosAngle - dy*sinAngle;
        yChange = dx*sinAngle + dy*cosAngle;
      } // GetPoseChange()

      
      // This function actually swaps in the new marker to track, and should
      // not be made available as part of the public API since it could get
      // interrupted by main and we want all this stuff updated at once.
      static Result UpdateMarkerToTrack()
      {
        if(newMarkerToTrackWasProvided_) {
          
          mode_                  = VISION_MODE_DEFAULT;
          numTrackFailures_      = 0;
          
          markerToTrack_ = newMarkerToTrack_;
          
          if(markerToTrack_.IsSpecified()) {
            
            AnkiConditionalErrorAndReturnValue(markerToTrack_.width_mm > 0.f,
                                               RESULT_FAIL_INVALID_PARAMETER,
                                               "VisionSystem::UpdateMarkerToTrack()",
                                               "Invalid marker width specified.");
            
            // Set canonical 3D marker's corner coordinates
            const P3P_PRECISION markerHalfWidth = markerToTrack_.width_mm * P3P_PRECISION(0.5);
            canonicalMarker3d_[0] = Point3<P3P_PRECISION>(-markerHalfWidth, -markerHalfWidth, 0);
            canonicalMarker3d_[1] = Point3<P3P_PRECISION>(-markerHalfWidth,  markerHalfWidth, 0);
            canonicalMarker3d_[2] = Point3<P3P_PRECISION>( markerHalfWidth, -markerHalfWidth, 0);
            canonicalMarker3d_[3] = Point3<P3P_PRECISION>( markerHalfWidth,  markerHalfWidth, 0);
          } // if markerToTrack is valid
          
          newMarkerToTrack_.Clear();
          newMarkerToTrackWasProvided_ = false;
        } // if newMarker provided
        
        return RESULT_OK;
        
      } // UpdateMarkerToTrack()
      
      
      static Radians GetCurrentHeadAngle()
      {
        return robotState_.headAngle;
      }

      static Radians GetPreviousHeadAngle()
      {
        return prevRobotState_.headAngle;
      }

      void SetImageSendMode(ImageSendMode_t mode, Vision::CameraResolution res)
      {
        if (res == Vision::CAMERA_RES_QVGA ||
          res == Vision::CAMERA_RES_QQVGA ||
          res == Vision::CAMERA_RES_QQQVGA ||
          res == Vision::CAMERA_RES_QQQQVGA) {
            imageSendMode_ = mode;
            nextSendImageResolution_ = res;
        }
      }

#if USE_COMPRESSION_FOR_SENDING_IMAGES
      void CompressAndSendImage(const Array<u8> &img)
      {
        static u32 imgID = 0;
        const cv::vector<int> compressionParams = {
          CV_IMWRITE_JPEG_QUALITY, IMAGE_SEND_JPEG_COMPRESSION_QUALITY
        };
        
        cv::Mat cvImg;
        Result lastResult = ArrayToCvMat(img, &cvImg);
        AnkiConditionalErrorAndReturn(lastResult == RESULT_OK,
                                      "CompressAndSendImage.ArrayToCvMat failure.",
                                      "Failed to convert input array to cv::Mat for compression.\n");
        
        cv::vector<u8> compressedBuffer;
        cv::imencode(".jpg",  cvImg, compressedBuffer, compressionParams);
        
        const u32 numTotalBytes = compressedBuffer.size();
        
        Messages::ImageChunk m;
        // TODO: pass this in so it corresponds to actual frame capture time instead of send time
        m.frameTimeStamp = HAL::GetTimeStamp();
        m.resolution = captureResolution_;
        m.imageId = ++imgID;
        m.chunkId = 0;
        m.chunkSize = IMAGE_CHUNK_SIZE;
        m.imageChunkCount = ceilf((f32)numTotalBytes / IMAGE_CHUNK_SIZE);
        m.imageEncoding = IE_JPEG;
        
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
      
      void DownsampleAndSendImage(const Array<u8> &img)
      {
        // If limiting framerate and the buffer is not empty, then silently fail and return
        if(wifiCamera_limitFramerate) {
          const int freeSpace = HAL::UARTGetFreeSpace();
          const int bufferSize = HAL::UARTGetWriteBufferSize();
          
          if(freeSpace < (bufferSize-5))  // Hack, case of off-by-some errors
            return;
        }
      
        // Only downsample if normal capture res is QVGA
        if (imageSendMode_ != ISM_OFF && captureResolution_ == Vision::CAMERA_RES_QVGA) {
          
          // Time to send frame?
          static u8 streamFrameCnt = 0;
          if (imageSendMode_ == ISM_STREAM && streamFrameCnt++ != IMG_STREAM_SKIP_FRAMES) {
            return;
          }
          streamFrameCnt = 0;
          
          
          static u32 imgID = 0;

          // Downsample and split into image chunk message
          const u32 xRes = Vision::CameraResInfo[nextSendImageResolution_].width;
          const u32 yRes = Vision::CameraResInfo[nextSendImageResolution_].height;

          const u32 xSkip = 320 / xRes;
          const u32 ySkip = 240 / yRes;

          const u32 numTotalBytes = xRes*yRes;

          Messages::ImageChunk m;
          // TODO: pass this in so it corresponds to actual frame capture time instead of send time
          m.frameTimeStamp = HAL::GetTimeStamp();
          m.resolution = nextSendImageResolution_;
          m.imageId = ++imgID;
          m.chunkId = 0;
          m.chunkSize = IMAGE_CHUNK_SIZE;
          m.imageChunkCount = ceilf((f32)numTotalBytes / IMAGE_CHUNK_SIZE);
          m.imageEncoding = 0;

          u32 totalByteCnt = 0;
          u32 chunkByteCnt = 0;

          //PRINT("Downsample: from %d x %d  to  %d x %d\n", img.get_size(1), img.get_size(0), xRes, yRes);

          u32 dataY = 0;
          for (u32 y = 0; y < 240; y += ySkip, dataY++)
          {
            const u8* restrict rowPtr = img.Pointer(y, 0);

            u32 dataX = 0;
            for (u32 x = 0; x < 320; x += xSkip, dataX++)
            {
              m.data[chunkByteCnt] = rowPtr[x];
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
            }
          }

          // Turn off image sending if sending single image only.
          if (imageSendMode_ == ISM_SINGLE_SHOT) {
            imageSendMode_ = ISM_OFF;
          }
        }
      }

      static Result LookForMarkers(
        const Array<u8> &grayscaleImage,
        const DetectFiducialMarkersParameters &parameters,
        FixedLengthList<VisionMarker> &markers,
        MemoryStack ccmScratch,
        MemoryStack onchipScratch,
        MemoryStack offchipScratch)
      {
        BeginBenchmark("VisionSystem_LookForMarkers");

        AnkiAssert(parameters.isInitialized);

        const s32 maxMarkers = markers.get_maximumSize();

        FixedLengthList<Array<f32> > homographies(maxMarkers, ccmScratch);

        markers.set_size(maxMarkers);
        homographies.set_size(maxMarkers);

        for(s32 i=0; i<maxMarkers; i++) {
          Array<f32> newArray(3, 3, ccmScratch);
          homographies[i] = newArray;
        }

        // XXX: MatlabVisualization::ResetFiducialDetection(grayscaleImage);

#if USE_MATLAB_DETECTOR
        const Result result = MatlabVisionProcessor::DetectMarkers(grayscaleImage, markers, homographies, ccmScratch);
#else
        const Result result = DetectFiducialMarkers(
          grayscaleImage,
          markers,
          homographies,
          parameters.scaleImage_numPyramidLevels, parameters.scaleImage_thresholdMultiplier,
          parameters.component1d_minComponentWidth, parameters.component1d_maxSkipDistance,
          parameters.component_minimumNumPixels, parameters.component_maximumNumPixels,
          parameters.component_sparseMultiplyThreshold, parameters.component_solidMultiplyThreshold,
          parameters.component_minHollowRatio,
          parameters.quads_minQuadArea, parameters.quads_quadSymmetryThreshold, parameters.quads_minDistanceFromImageEdge,
          parameters.decode_minContrastRatio,
          parameters.maxConnectedComponentSegments,
          parameters.maxExtractedQuads,
          parameters.quadRefinementIterations,
          parameters.numRefinementSamples,
          parameters.quadRefinementMaxCornerChange,
          parameters.quadRefinementMinCornerChange,
          false,
          ccmScratch, onchipScratch, offchipScratch);
#endif
        
        if(result != RESULT_OK) {
          return result;
        }

        EndBenchmark("VisionSystem_LookForMarkers");

        DebugStream::SendFiducialDetection(grayscaleImage, markers, ccmScratch, onchipScratch, offchipScratch);

        for(s32 i_marker = 0; i_marker < markers.get_size(); ++i_marker) {
          const VisionMarker crntMarker = markers[i_marker];

          // XXX: MatlabVisualization::SendFiducialDetection(crntMarker.corners, crntMarker.markerType);
        }

        // XXX: MatlabVisualization::SendDrawNow();

        return RESULT_OK;
      } // LookForMarkers()

      // Divide image by mean of whatever is inside the trackingQuad
      static Result BrightnessNormalizeImage(Array<u8>& image, const Quadrilateral<f32>& quad)
      {
        //Debug: image.Show("OriginalImage", false);
        
#define USE_VARIANCE 0
        
        // Compute mean of data inside the bounding box of the tracking quad
        const Rectangle<s32> bbox = quad.ComputeBoundingRectangle<s32>();
        
        ConstArraySlice<u8> imageROI = image(bbox.top, bbox.bottom, bbox.left, bbox.right);
        
#if USE_VARIANCE
        // Playing with normalizing using std. deviation as well
        s32 mean, var;
        Matrix::MeanAndVar<u8, s32>(imageROI, mean, var);
        const f32 stddev = sqrt(static_cast<f32>(var));
        const f32 oneTwentyEightOverStdDev = 128.f / stddev;
        //PRINT("Initial mean/std = %d / %.2f\n", mean, sqrt(static_cast<f32>(var)));
#else
        const u8 mean = Matrix::Mean<u8, u32>(imageROI);
        //PRINT("Initial mean = %d\n", mean);
#endif
        
        //PRINT("quad mean = %d\n", mean);
        //const f32 oneOverMean = 1.f / static_cast<f32>(mean);
        
        // Remove mean (and variance) from image
        for(s32 i=0; i<image.get_size(0); ++i)
        {
          u8 * restrict img_i = image.Pointer(i, 0);
          
          for(s32 j=0; j<image.get_size(1); ++j)
          {
            f32 value = static_cast<f32>(img_i[j]);
            value -= static_cast<f32>(mean);
#if USE_VARIANCE
            value *= oneTwentyEightOverStdDev;
#endif
            value += 128.f;
            img_i[j] = saturate_cast<u8>(value) ;
          }
        }
        
        // Debug:
        /*
#if USE_VARIANCE
        Matrix::MeanAndVar<u8, s32>(imageROI, mean, var);
        PRINT("Final mean/std = %d / %.2f\n", mean, sqrt(static_cast<f32>(var)));
#else 
        PRINT("Final mean = %d\n", Matrix::Mean<u8,u32>(imageROI));
#endif
         */
        
        //Debug: image.Show("NormalizedImage", true);
        
#undef USE_VARIANCE
        return RESULT_OK;
        
      } // BrightnessNormalizeImage()
      
      
      static Result BrightnessNormalizeImage(Array<u8>& image, const Quadrilateral<f32>& quad,
        const f32 filterWidthFraction,
        MemoryStack scratch)
      {
        if(filterWidthFraction > 0.f) {
          //Debug:
          image.Show("OriginalImage", false);
          
          // TODO: Add the ability to only normalize within the vicinity of the quad
          // Note that this requires templateQuad to be sorted!
          const s32 filterWidth = static_cast<s32>(filterWidthFraction*((quad[3] - quad[0]).Length()));
          AnkiAssert(filterWidth > 0.f);

          Array<u8> imageNormalized(image.get_size(0), image.get_size(1), scratch);

          AnkiConditionalErrorAndReturnValue(imageNormalized.IsValid(),
            RESULT_FAIL_OUT_OF_MEMORY,
            "VisionSystem::BrightnessNormalizeImage",
            "Out of memory allocating imageNormalized.\n");

          BeginBenchmark("BoxFilterNormalize");

          ImageProcessing::BoxFilterNormalize(image, filterWidth, static_cast<u8>(128),
            imageNormalized, scratch);

          EndBenchmark("BoxFilterNormalize");

          { // DEBUG
            /*
            static Matlab matlab(false);
            matlab.PutArray(grayscaleImage, "grayscaleImage");
            matlab.PutArray(grayscaleImageNormalized, "grayscaleImageNormalized");
            matlab.EvalString("subplot(121), imagesc(grayscaleImage), axis image, colorbar, "
            "subplot(122), imagesc(grayscaleImageNormalized), colorbar, axis image, "
            "colormap(gray)");
            */

            //image.Show("GrayscaleImage", false);
            //imageNormalized.Show("GrayscaleImageNormalized", false);
          }

          image.Set(imageNormalized);
          
          //Debug:
          image.Show("NormalizedImage", true);
          
        } // if(filterWidthFraction > 0)

        return RESULT_OK;
      } // BrightnessNormalizeImage()


      //
      // Tracker Prediction
      //
      // Adjust the tracker transformation by approximately how much we
      // think we've moved since the last tracking call.
      //
      static Result TrackerPredictionUpdate(const Array<u8>& grayscaleImage, MemoryStack scratch)
      {
        Result result = RESULT_OK;
        
        assert(false); while(true); // XXX: Implement me for offboard version!
/*
        const Quadrilateral<f32> currentQuad = GetTrackerQuad(scratch);

        // XXX: MatlabVisualization::SendTrackerPrediction_Before(grayscaleImage, currentQuad);

        // Ask VisionState how much we've moved since last call (in robot coordinates)
        Radians theta_robot;
        f32 T_fwd_robot, T_hor_robot;

        GetPoseChange(T_fwd_robot, T_hor_robot, theta_robot);

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF

#if USE_MATLAB_TRACKER
        MatlabVisionProcessor::UpdateTracker(T_fwd_robot, T_hor_robot,
          theta_robot, theta_head);
#else
        Radians theta_head2 = GetCurrentHeadAngle();
        Radians theta_head1 = GetPreviousHeadAngle();

        const f32 cH1 = cosf(theta_head1.ToFloat());
        const f32 sH1 = sinf(theta_head1.ToFloat());

        const f32 cH2 = cosf(theta_head2.ToFloat());
        const f32 sH2 = sinf(theta_head2.ToFloat());

        const f32 cR = cosf(theta_robot.ToFloat());
        const f32 sR = sinf(theta_robot.ToFloat());

        // NOTE: these "geometry" entries were computed symbolically with Sage
        // In the derivation, it was assumed the head and neck positions' Y
        // components are zero.
        //
        // From Sage:
        // [cos(thetaR)                 sin(thetaH1)*sin(thetaR)       cos(thetaH1)*sin(thetaR)]
        // [-sin(thetaH2)*sin(thetaR)   cos(thetaR)*sin(thetaH1)*sin(thetaH2) + cos(thetaH1)*cH2  cos(thetaH1)*cos(thetaR)*sin(thetaH2) - cos(thetaH2)*sin(thetaH1)]
        // [-cos(thetaH2)*sin(thetaR)   cos(thetaH2)*cos(thetaR)*sin(thetaH1) - cos(thetaH1)*sin(thetaH2) cos(thetaH1)*cos(thetaH2)*cos(thetaR) + sin(thetaH1)*sin(thetaH2)]
        //
        // T_blockRelHead_new =
        // [T_hor*cos(thetaR) + (Hx*cos(thetaH1) - Hz*sin(thetaH1) + Nx)*sin(thetaR) - T_fwd*sin(thetaR)]
        // [(Hx*cos(thetaH1) - Hz*sin(thetaH1) + Nx)*cos(thetaR)*sin(thetaH2) - (Hz*cos(thetaH1) + Hx*sin(thetaH1) + Nz)*cos(thetaH2) + (Hz*cos(thetaH2) + Hx*sin(thetaH2) + Nz)*cos(thetaH2) - (Hx*cos(thetaH2) - Hz*sin(thetaH2) + Nx)*sin(thetaH2) - (T_fwd*cos(thetaR) + T_hor*sin(thetaR))*sin(thetaH2)]
        // [(Hx*cos(thetaH1) - Hz*sin(thetaH1) + Nx)*cos(thetaH2)*cos(thetaR) - (Hx*cos(thetaH2) - Hz*sin(thetaH2) + Nx)*cos(thetaH2) - (T_fwd*cos(thetaR) + T_hor*sin(thetaR))*cos(thetaH2) + (Hz*cos(thetaH1) + Hx*sin(thetaH1) + Nz)*sin(thetaH2) - (Hz*cos(thetaH2) + Hx*sin(thetaH2) + Nz)*sin(thetaH2)]

        AnkiAssert(HEAD_CAM_POSITION[1] == 0.f && NECK_JOINT_POSITION[1] == 0.f);
        Array<f32> R_geometry = Array<f32>(3,3,scratch);
        R_geometry[0][0] = cR;     R_geometry[0][1] = sH1*sR;             R_geometry[0][2] = cH1*sR;
        R_geometry[1][0] = -sH2*sR; R_geometry[1][1] = cR*sH1*sH2 + cH1*cH2;  R_geometry[1][2] = cH1*cR*sH2 - cH2*sH1;
        R_geometry[2][0] = -cH2*sR; R_geometry[2][1] = cH2*cR*sH1 - cH1*sH2;  R_geometry[2][2] = cH1*cH2*cR + sH1*sH2;

        const f32 term1 = (HEAD_CAM_POSITION[0]*cH1 - HEAD_CAM_POSITION[2]*sH1 + NECK_JOINT_POSITION[0]);
        const f32 term2 = (HEAD_CAM_POSITION[2]*cH1 + HEAD_CAM_POSITION[0]*sH1 + NECK_JOINT_POSITION[2]);
        const f32 term3 = (HEAD_CAM_POSITION[2]*cH2 + HEAD_CAM_POSITION[0]*sH2 + NECK_JOINT_POSITION[2]);
        const f32 term4 = (HEAD_CAM_POSITION[0]*cH2 - HEAD_CAM_POSITION[2]*sH2 + NECK_JOINT_POSITION[0]);
        const f32 term5 = (T_fwd_robot*cR + T_hor_robot*sR);

        Point3<f32> T_geometry(T_hor_robot*cR + term1*sR - T_fwd_robot*sR,
          term1*cR*sH2 - term2*cH2 + term3*cH2 - term4*sH2 - term5*sH2,
          term1*cH2*cR - term4*cH2 - term5*cH2 + term2*sH2 - term3*sH2);

        Array<f32> R_blockRelHead = Array<f32>(3,3,scratch);
        tracker_.GetRotationMatrix(R_blockRelHead);
        const Point3<f32>& T_blockRelHead = tracker_.GetTranslation();

        Array<f32> R_blockRelHead_new = Array<f32>(3,3,scratch);
        Matrix::Multiply(R_geometry, R_blockRelHead, R_blockRelHead_new);

        Point3<f32> T_blockRelHead_new = R_geometry*T_blockRelHead + T_geometry;

        if(tracker_.UpdateRotationAndTranslation(R_blockRelHead_new,
          T_blockRelHead_new,
          scratch) == RESULT_OK)
        {
          result = RESULT_OK;
        }

#endif // #if USE_MATLAB_TRACKER

#else
        const Quadrilateral<f32> sortedQuad  = currentQuad.ComputeClockwiseCorners<f32>();

        f32 dx = sortedQuad[3].x - sortedQuad[0].x;
        f32 dy = sortedQuad[3].y - sortedQuad[0].y;
        const f32 observedVerticalSize_pix = sqrtf( dx*dx + dy*dy );

        // Compare observed vertical size to actual block marker size (projected
        // to be orthogonal to optical axis, using head angle) to approximate the
        // distance to the marker along the camera's optical axis
        Radians theta_head = GetCurrentHeadAngle();
        const f32 cosHeadAngle = cosf(theta_head.ToFloat());
        const f32 sinHeadAngle = sinf(theta_head.ToFloat());
        const f32 d = (trackingMarkerWidth_mm* cosHeadAngle *
          headCamInfo_->focalLength_y /
          observedVerticalSize_pix);

        // Convert to how much we've moved along (and orthogonal to) the camera's optical axis
        const f32 T_fwd_cam =  T_fwd_robot*cosHeadAngle;
        const f32 T_ver_cam = -T_fwd_robot*sinHeadAngle;

        // Predict approximate horizontal shift from two things:
        // 1. The rotation of the robot
        //    Compute pixel-per-degree of the camera and multiply by degrees rotated
        // 2. Convert horizontal shift of the robot to pixel shift, using
        //    focal length
        f32 horizontalShift_pix = (static_cast<f32>(headCamInfo_->ncols/2) * theta_robot.ToFloat() /
          headCamFOV_hor_) + (T_hor_robot*headCamInfo_->focalLength_x/d);

        // Predict approximate scale change by comparing the distance to the
        // object before and after forward motion
        const f32 scaleChange = d / (d - T_fwd_cam);

        // Predict approximate vertical shift in the camera plane by comparing
        // vertical motion (orthogonal to camera's optical axis) to the focal
        // length
        const f32 verticalShift_pix = T_ver_cam * headCamInfo_->focalLength_y/d;

        PRINT("Adjusting transformation: %.3fpix H shift for %.3fdeg rotation, "
          "%.3f scaling and %.3f V shift for %.3f translation forward (%.3f cam)\n",
          horizontalShift_pix, theta_robot.getDegrees(), scaleChange,
          verticalShift_pix, T_fwd_robot, T_fwd_cam);

        // Adjust the Transformation
        // Note: UpdateTransformation is doing *inverse* composition (thus using the negatives)
        if(tracker_.get_transformation().get_transformType() == Transformations::TRANSFORM_TRANSLATION) {
          Array<f32> update(1,2,scratch);
          update[0][0] = -horizontalShift_pix;
          update[0][1] = -verticalShift_pix;

#if USE_MATLAB_TRACKER
          MatlabVisionProcessor::UpdateTracker(update);
#else
          tracker_.UpdateTransformation(update, 1.f, scratch,
            Transformations::TRANSFORM_TRANSLATION);
#endif
        }
        else {
          // Inverse update we are composing is:
          //
          //                  [s 0 0]^(-1)     [0 0 h_shift]^(-1)
          //   updateMatrix = [0 s 0]       *  [0 0 v_shift]
          //                  [0 0 1]          [0 0    1   ]
          //
          //      [1/s  0  -h_shift/s]   [ update_0  update_1  update_2 ]
          //   =  [ 0  1/2 -v_shift/s] = [ update_3  update_4  update_5 ]
          //      [ 0   0      1     ]   [    0         0         1     ]
          //
          // Note: UpdateTransformation adds 1.0 to the diagonal scale terms
          Array<f32> update(1,6,scratch);
          update.Set(0.f);
          update[0][0] = 1.f/scaleChange - 1.f;               // first row, first col
          update[0][2] = -horizontalShift_pix/scaleChange;    // first row, last col
          update[0][4] = 1.f/scaleChange - 1.f;               // second row, second col
          update[0][5] = -verticalShift_pix/scaleChange;      // second row, last col

#if USE_MATLAB_TRACKER
          MatlabVisionProcessor::UpdateTracker(update);
#else
          tracker_.UpdateTransformation(update, 1.f, scratch,
            Transformations::TRANSFORM_AFFINE);
#endif
        } // if(tracker transformation type == TRANSLATION...)

#endif // if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF

        // XXX: MatlabVisualization::SendTrackerPrediction_After(GetTrackerQuad(scratch));
*/
        return result;
      } // TrackerPredictionUpdate()

      static void FillDockErrMsg(const Quadrilateral<f32>& currentQuad,
        Messages::DockingErrorSignal& dockErrMsg,
        MemoryStack scratch)
      {
        dockErrMsg.isApproximate = false;

#if USE_APPROXIMATE_DOCKING_ERROR_SIGNAL
        dockErrMsg.isApproximate = true;

        const bool useTopBar = false; // TODO: pass in? make a docker parameter?
        const f32 focalLength_x = headCamInfo_->focalLength_x;
        const f32 imageResolutionWidth_pix = detectionParameters_.detectionWidth;

        Quadrilateral<f32> sortedQuad = currentQuad.ComputeClockwiseCorners<f32>();
        const Point<f32>& lineLeft  = (useTopBar ? sortedQuad[0] : sortedQuad[3]); // topLeft  or bottomLeft
        const Point<f32>& lineRight = (useTopBar ? sortedQuad[1] : sortedQuad[2]); // topRight or bottomRight

        AnkiAssert(lineRight.x > lineLeft.x);

        //L = sqrt(sum( (upperRight-upperLeft).^2) );
        const f32 lineDx = lineRight.x - lineLeft.x;
        const f32 lineDy = lineRight.y - lineLeft.y;
        const f32 lineLength = sqrtf(lineDx*lineDx + lineDy*lineDy);

        // Get the angle from vertical of the top or bottom bar of the marker
        //we're tracking

        //angleError = -asin( (upperRight(2)-upperLeft(2)) / L);
        //const f32 angleError = -asinf( (upperRight.y-upperLeft.y) / lineLength);
        const f32 angleError = -asinf( (lineRight.y-lineLeft.y) / lineLength) * 4;  // Multiply by scalar which makes angleError a little more accurate.  TODO: Something smarter than this.

        //currentDistance = BlockMarker3D.ReferenceWidth * this.calibration.fc(1) / L;
        const f32 distanceError = trackingMarkerWidth_mm * focalLength_x / lineLength;

        //ANS: now returning error in terms of camera. mainExecution converts to robot coords
        // //distError = currentDistance - CozmoDocker.LIFT_DISTANCE;
        // const f32 distanceError = currentDistance - cozmoLiftDistanceInMM;

        // TODO: should I be comparing to ncols/2 or calibration center?

        //midPointErr = -( (upperRight(1)+upperLeft(1))/2 - this.trackingResolution(1)/2 );
        f32 midpointError = ( (lineRight.x+lineLeft.x)/2 - imageResolutionWidth_pix/2 );

        //midPointErr = midPointErr * currentDistance / this.calibration.fc(1);
        midpointError *= distanceError / focalLength_x;

        // Go ahead and put the errors in the robot centric coordinates (other
        // than taking head angle into account)
        dockErrMsg.x_distErr = distanceError;
        dockErrMsg.y_horErr  = -midpointError;
        dockErrMsg.angleErr  = angleError;
        dockErrMsg.z_height  = -1.f; // unknown for approximate error signal

#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF

#if USE_MATLAB_TRACKER
        MatlabVisionProcessor::ComputeProjectiveDockingSignal(currentQuad,
          dockErrMsg.x_distErr,
          dockErrMsg.y_horErr,
          dockErrMsg.z_height,
          dockErrMsg.angleErr);
#else
        // Despite the names, fill the elements of the message with camera-centric coordinates
        // XXX:  Tracker is no longer local - get these another way
        /*
        dockErrMsg.x_distErr = tracker_.GetTranslation().x;
        dockErrMsg.y_horErr  = tracker_.GetTranslation().y;
        dockErrMsg.z_height  = tracker_.GetTranslation().z;

        dockErrMsg.angleErr  = tracker_.get_angleY();
        */
        
#endif // if USE_MATLAB_TRACKER

#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE || DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER

#if USE_MATLAB_TRACKER
        MatlabVisionProcessor::ComputeProjectiveDockingSignal(currentQuad,
          dockErrMsg.x_distErr,
          dockErrMsg.y_horErr,
          dockErrMsg.z_height,
          dockErrMsg.angleErr);
#else

        // Compute the current pose of the block relative to the camera:
        Array<P3P_PRECISION> R = Array<P3P_PRECISION>(3,3, scratch);
        Point3<P3P_PRECISION> T;
        Quadrilateral<P3P_PRECISION> currentQuad_atPrecision(Point<P3P_PRECISION>(currentQuad[0].x, currentQuad[0].y),
          Point<P3P_PRECISION>(currentQuad[1].x, currentQuad[1].y),
          Point<P3P_PRECISION>(currentQuad[2].x, currentQuad[2].y),
          Point<P3P_PRECISION>(currentQuad[3].x, currentQuad[3].y));

#warning broken
        /*P3P::computePose(currentQuad_atPrecision,
        canonicalMarker3d_[0], canonicalMarker3d_[1],
        canonicalMarker3d_[2], canonicalMarker3d_[3],
        headCamInfo_->focalLength_x, headCamInfo_->focalLength_y,
        headCamInfo_->center_x, headCamInfo_->center_y,
        R, T, scratch);*/

        // Extract what we need for the docking error signal from the block's pose:
        dockErrMsg.x_distErr = T.x;
        dockErrMsg.y_horErr  = T.y;
        dockErrMsg.z_height  = T.z;
        dockErrMsg.angleErr  = asinf(R[2][0]);

#endif // if USE_MATLAB_TRACKER

#endif // if USE_APPROXIMATE_DOCKING_ERROR_SIGNAL
      } // FillDockErrMsg()

#if 0
#pragma mark --- Public VisionSystem API Implementations ---
#endif

      u32 DownsampleHelper(const Array<u8>& in,
        Array<u8>& out,
        MemoryStack scratch)
      {
        const s32 inWidth  = in.get_size(1);
        //const s32 inHeight = in.get_size(0);

        const s32 outWidth  = out.get_size(1);
        //const s32 outHeight = out.get_size(0);

        const u32 downsampleFactor = inWidth / outWidth;

        const u32 downsamplePower = Log2u32(downsampleFactor);

        if(downsamplePower > 0) {
          //PRINT("Downsampling [%d x %d] frame by %d.\n", inWidth, inHeight, (1 << downsamplePower));

          ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(in,
            downsamplePower,
            out,
            scratch);
        } else {
          // No need to downsample, just copy the buffer
          out.Set(in);
        }

        return downsampleFactor;
      }

      const HAL::CameraInfo* GetCameraCalibration() {
        // TODO: is just returning the pointer to HAL's camera info struct kosher?
        return headCamInfo_;
      }

      f32 GetTrackingMarkerWidth() {
        return markerToTrack_.width_mm;
      }

      f32 GetVerticalFOV() {
        return headCamFOV_ver_;
      }

      f32 GetHorizontalFOV() {
        return headCamFOV_hor_;
      }

      const FaceDetectionParameters& GetFaceDetectionParams() {
        return faceDetectionParameters_;
      }
      
      Result Init()
      {
        Result result = RESULT_OK;

        if(!isInitialized_) {
          captureResolution_ = Vision::CAMERA_RES_QVGA;

          // WARNING: the order of these initializations matter!

          //
          // Initialize the VisionSystem's state (i.e. its "private member variables")
          //

          mode_                      = VISION_MODE_DEFAULT;
          markerToTrack_.Clear();
          numTrackFailures_          = 0;

          wasCalledOnce_             = false;
          havePreviousRobotState_    = false;

          headCamInfo_ = HAL::GetHeadCamInfo();
          if(headCamInfo_ == NULL) {
            PRINT("Initialize() - HeadCam Info pointer is NULL!\n");
            return RESULT_FAIL;
          }

          // Compute FOV from focal length (currently used for tracker prediciton)
          headCamFOV_ver_ = 2.f * atanf(static_cast<f32>(headCamInfo_->nrows) /
            (2.f * headCamInfo_->focalLength_y));
          headCamFOV_hor_ = 2.f * atanf(static_cast<f32>(headCamInfo_->ncols) /
            (2.f * headCamInfo_->focalLength_x));

          exposureTime_ = 0.2f; // TODO: pick a reasonable start value
          frameNumber = 0;

          detectionParameters_.Initialize();
          trackerParameters_.Initialize();
          faceDetectionParameters_.Initialize();

          Simulator::Initialize();

#ifdef RUN_SIMPLE_TRACKING_TEST
          Anki::Cozmo::VisionSystem::SetMarkerToTrack(Vision::MARKER_FIRE, DEFAULT_BLOCK_MARKER_WIDTH_MM);
#endif

          result = VisionMemory::Initialize();
          if(result != RESULT_OK) { return result; }

          result = DebugStream::Initialize();
          if(result != RESULT_OK) { return result; }

          // XXX: result = MatlabVisualization::Initialize();
          // XXX: if(result != RESULT_OK) { return result; }

#if USE_MATLAB_TRACKER || USE_MATLAB_DETECTOR
          result = MatlabVisionProcessor::Initialize();
          if(result != RESULT_OK) { return result; }
#endif

          RcamWrtRobot_ = Array<f32>(3,3,VisionMemory::ccmScratch_);
          
          markerToTrack_.Clear();
          newMarkerToTrack_.Clear();
          newMarkerToTrackWasProvided_ = false;

          isWaitingOnSnapshot_ = false;
          isSnapshotReady_ = NULL;
          snapshotROI_ = Rectangle<s32>(-1, -1, -1, -1);
          snapshot_ = NULL;
          
          isInitialized_ = true;
        }

        return result;
      }

      Result SetMarkerToTrack(const Vision::MarkerType& markerTypeToTrack,
                              const f32 markerWidth_mm,
                              const bool checkAngleX)
      {
        const Point2f imageCenter(-1.f, -1.f);
        const f32     searchRadius = -1.f;
        return SetMarkerToTrack(markerTypeToTrack, markerWidth_mm,
          imageCenter, searchRadius, checkAngleX);
      }

      Result SetMarkerToTrack(const Vision::MarkerType& markerTypeToTrack,
                              const f32 markerWidth_mm,
                              const Point2f& atImageCenter,
                              const f32 imageSearchRadius,
                              const bool checkAngleX)
      {
        newMarkerToTrack_.type              = markerTypeToTrack;
        newMarkerToTrack_.width_mm          = markerWidth_mm;
        newMarkerToTrack_.imageCenter       = atImageCenter;
        newMarkerToTrack_.imageSearchRadius = imageSearchRadius;
        newMarkerToTrack_.checkAngleX       = checkAngleX;
        
        // Next call to Update(), we will call UpdateMarkerToTrack() and
        // actually replace the current markerToTrack_ with the one set here.
        newMarkerToTrackWasProvided_ = true;
        
        // Send a message up to basestation vision to start tracking:
        //HAL::RadioSendMessage(<#const Messages::ID msgID#>, <#const void *buffer#>)
        
        return RESULT_OK;
      }
      
      void StopTracking()
      {
        SetMarkerToTrack(Vision::MARKER_UNKNOWN, 0.f, true);
      }
      
      Result StartDetectingFaces()
      {
        mode_ = VISION_MODE_DETECTING_FACES;
        return RESULT_OK;
      }

      const Embedded::FixedLengthList<Embedded::VisionMarker>& GetObservedMarkerList()
      {
        return VisionMemory::markers_;
      } // GetObservedMarkerList()

      Result GetVisionMarkerPoseNearestTo(const Embedded::Point3<f32>&  atPosition,
        const Vision::MarkerType&     withType,
        const f32                     maxDistance_mm,
        Embedded::Array<f32>&         rotationWrtRobot,
        Embedded::Point3<f32>&        translationWrtRobot,
        bool&                         markerFound)
      {
        using namespace Embedded;

        Result lastResult = RESULT_OK;
        markerFound = false;

        if(VisionMemory::markers_.get_size() > 0)
        {
          FixedLengthList<VisionMarker*> markersWithType(VisionMemory::markers_.get_size(),
            VisionMemory::ccmScratch_);

          AnkiConditionalErrorAndReturnValue(markersWithType.IsValid(),
            RESULT_FAIL_MEMORY,
            "GetVisionMarkerPoseNearestTo",
            "Failed to allocate markersWithType FixedLengthList.");

          // Find all markers with specified type
          s32 numFound = 0;
          VisionMarker  * restrict pMarker = VisionMemory::markers_.Pointer(0);
          VisionMarker* * restrict pMarkerWithType = markersWithType.Pointer(0);

          for(s32 i=0; i<VisionMemory::markers_.get_size(); ++i)
          {
            if(pMarker[i].markerType == withType) {
              pMarkerWithType[numFound++] = pMarker + i;
            }
          }
          markersWithType.set_size(numFound);

          // If any were found, find the one that is closest to the specified
          // 3D point and within the specified max distance
          if(numFound > 0) {
            // Create a little MemoryStack for allocating temporary
            // rotation matrix
            const s32 SCRATCH_BUFFER_SIZE = 128;
            char scratchBuffer[SCRATCH_BUFFER_SIZE];
            MemoryStack scratch(scratchBuffer, SCRATCH_BUFFER_SIZE);

            // Create temporary pose storage (wrt camera)
            Point3<f32> translationWrtCamera;
            Array<f32> rotationWrtCamera(3,3,scratch);
            AnkiConditionalErrorAndReturnValue(rotationWrtCamera.IsValid(),
              RESULT_FAIL_MEMORY,
              "GetVisionMarkerPoseNearestTo",
              "Failed to allocate rotationWrtCamera Array.");

            VisionMarker* const* restrict pMarkerWithType = markersWithType.Pointer(0);

            f32 closestDistance = maxDistance_mm;

            for(s32 i=0; i<numFound; ++i) {
              // Compute this marker's pose WRT camera
              if((lastResult = GetVisionMarkerPose(*(pMarkerWithType[i]), true,
                rotationWrtCamera, translationWrtCamera)) != RESULT_OK) {
                  return lastResult;
              }

              // Convert it to pose WRT robot
              if((lastResult = GetWithRespectToRobot(rotationWrtCamera, translationWrtCamera,
                rotationWrtRobot, translationWrtRobot)) != RESULT_OK) {
                  return lastResult;
              }

              // See how far it is from the specified position
              const f32 currentDistance = (translationWrtRobot - atPosition).Length();
              if(currentDistance < closestDistance) {
                closestDistance = currentDistance;
                markerFound = true;
              }
            } // for each marker with type
          } // if numFound > 0
        } // if(VisionMemory::markers_.get_size() > 0)

        return RESULT_OK;
      } // GetVisionMarkerPoseNearestTo()

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

      Result GetVisionMarkerPose(const Embedded::VisionMarker& marker,
        const bool ignoreOrientation,
        Embedded::Array<f32>&  rotation,
        Embedded::Point3<f32>& translation)
      {
        Quadrilateral<f32> sortedQuad;
        if(ignoreOrientation) {
          sortedQuad = marker.corners.ComputeClockwiseCorners<f32>();
        } else {
          sortedQuad = marker.corners;
        }

        return P3P::computePose(sortedQuad,
          canonicalMarker3d_[0], canonicalMarker3d_[1],
          canonicalMarker3d_[2], canonicalMarker3d_[3],
          headCamInfo_->focalLength_x, headCamInfo_->focalLength_y,
          headCamInfo_->center_x, headCamInfo_->center_y,
          rotation, translation);
      } // GetVisionMarkerPose()
      
      
      Result TakeSnapshot(const Embedded::Rectangle<s32> roi, const s32 subsample,
                          Embedded::Array<u8>& snapshot, bool& readyFlag)
      {
        if(!isWaitingOnSnapshot_)
        {
          snapshotROI_ = roi;
          
          snapshotSubsample_ = subsample;
          AnkiConditionalErrorAndReturnValue(snapshotSubsample_ >= 1,
                                             RESULT_FAIL_INVALID_PARAMETER,
                                             "VisionSystem::TakeSnapshot()",
                                             "Subsample must be >= 1. %d was specified!\n", snapshotSubsample_);

          snapshot_ = &snapshot;
          
          AnkiConditionalErrorAndReturnValue(snapshot_ != NULL, RESULT_FAIL_INVALID_OBJECT,
                                             "VisionSystem::TakeSnapshot()", "NULL snapshot pointer!\n");
          
          AnkiConditionalErrorAndReturnValue(snapshot_->IsValid(),
                                             RESULT_FAIL_INVALID_OBJECT,
                                             "VisionSystem::TakeSnapshot()", "Invalid snapshot array!\n");
          
          const s32 nrowsSnap = snapshot_->get_size(0);
          const s32 ncolsSnap = snapshot_->get_size(1);
          
          AnkiConditionalErrorAndReturnValue(nrowsSnap*snapshotSubsample_ == snapshotROI_.get_height() &&
                                             ncolsSnap*snapshotSubsample_ == snapshotROI_.get_width(),
                                             RESULT_FAIL_INVALID_SIZE,
                                             "VisionSystem::TakeSnapshot()",
                                             "Snapshot ROI size (%dx%d) subsampled by %d doesn't match snapshot array size (%dx%d)!\n",
                                             snapshotROI_.get_height(), snapshotROI_.get_width(), snapshotSubsample_, nrowsSnap, ncolsSnap);
          
          isSnapshotReady_ = &readyFlag;
          
          AnkiConditionalErrorAndReturnValue(isSnapshotReady_ != NULL,
                                             RESULT_FAIL_INVALID_OBJECT,
                                             "VisionSystem::TakeSnapshot()",
                                             "NULL isSnapshotReady pointer!\n");
        
          isWaitingOnSnapshot_ = true;
          
        } // if !isWaitingOnSnapshot_
        
        return RESULT_OK;
      } // TakeSnapshot()
      
      
      static Result TakeSnapshotHelper(const Embedded::Array<u8>& grayscaleImage)
      {
        if(isWaitingOnSnapshot_) {
          
          const s32 nrowsFull = grayscaleImage.get_size(0);
          const s32 ncolsFull = grayscaleImage.get_size(1);
          
          if(snapshotROI_.top    < 0 || snapshotROI_.top    >= nrowsFull-1 ||
             snapshotROI_.bottom < 0 || snapshotROI_.bottom >= nrowsFull-1 ||
             snapshotROI_.left   < 0 || snapshotROI_.left   >= ncolsFull-1 ||
             snapshotROI_.right  < 0 || snapshotROI_.right  >= ncolsFull-1)
          {
            PRINT("VisionSystem::TakeSnapshotHelper(): Snapshot ROI out of bounds!\n");
            return RESULT_FAIL_INVALID_SIZE;
          }
          
          const s32 nrowsSnap = snapshot_->get_size(0);
          const s32 ncolsSnap = snapshot_->get_size(1);
          
          for(s32 iFull=snapshotROI_.top, iSnap=0;
              iFull<snapshotROI_.bottom && iSnap<nrowsSnap;
              iFull+= snapshotSubsample_, ++iSnap)
          {
            const u8 * restrict pImageRow = grayscaleImage.Pointer(iFull,0);
            u8 * restrict pSnapRow = snapshot_->Pointer(iSnap, 0);
            
            for(s32 jFull=snapshotROI_.left, jSnap=0;
                jFull<snapshotROI_.right && jSnap<ncolsSnap;
                jFull+= snapshotSubsample_, ++jSnap)
            {
              pSnapRow[jSnap] = pImageRow[jFull];
            }
          }
            
          isWaitingOnSnapshot_ = false;
          *isSnapshotReady_ = true;
          
        } // if isWaitingOnSnapshot_
        
        return RESULT_OK;
        
      } // TakeSnapshotHelper()



#if defined(SEND_IMAGE_ONLY)
      // In SEND_IMAGE_ONLY mode, just create a special version of update

      Result Update(const Messages::RobotState robotState)
      {
        // This should be called from elsewhere first, but calling it again won't hurt
        Init();

        VisionMemory::ResetBuffers();

        frameNumber++;

        const s32 captureHeight = Vision::CameraResInfo[captureResolution_].height;
        const s32 captureWidth  = Vision::CameraResInfo[captureResolution_].width;

        Array<u8> grayscaleImage(captureHeight, captureWidth,
          VisionMemory::onchipScratch_, Flags::Buffer(false,false,false));

        HAL::CameraGetFrame(reinterpret_cast<u8*>(grayscaleImage.get_buffer()),
          captureResolution_, false);

        BeginBenchmark("VisionSystem_CameraImagingPipeline");

        if(vignettingCorrection == VignettingCorrection_Software) {
          BeginBenchmark("VisionSystem_CameraImagingPipeline_Vignetting");

          MemoryStack onchipScratch_local = VisionMemory::onchipScratch_;
          FixedLengthList<f32> polynomialParameters(5, onchipScratch_local, Flags::Buffer(false, false, true));

          for(s32 i=0; i<5; i++)
            polynomialParameters[i] = vignettingCorrectionParameters[i];

          CorrectVignetting(grayscaleImage, polynomialParameters);

          EndBenchmark("VisionSystem_CameraImagingPipeline_Vignetting");
        } // if(vignettingCorrection == VignettingCorrection_Software)

        if(autoExposure_enabled && (frameNumber % autoExposure_adjustEveryNFrames) == 0) {
          BeginBenchmark("VisionSystem_CameraImagingPipeline_AutoExposure");

          ComputeBestCameraParameters(
            grayscaleImage,
            Rectangle<s32>(0, grayscaleImage.get_size(1)-1, 0, grayscaleImage.get_size(0)-1),
            autoExposure_integerCountsIncrement,
            autoExposure_highValue,
            autoExposure_percentileToMakeHigh,
            autoExposure_minExposureTime, autoExposure_maxExposureTime,
            autoExposure_tooHighPercentMultiplier,
            exposureTime,
            VisionMemory::ccmScratch_);

          EndBenchmark("VisionSystem_CameraImagingPipeline_AutoExposure");
        }

        //if(frameNumber % 10 == 0) {
        //if(vignettingCorrection == VignettingCorrection_Off)
        //vignettingCorrection = VignettingCorrection_CameraHardware;
        //else if(vignettingCorrection == VignettingCorrection_CameraHardware)
        //vignettingCorrection = VignettingCorrection_Software;
        //else if(vignettingCorrection == VignettingCorrection_Software)
        //vignettingCorrection = VignettingCorrection_Off;
        //}

        //if(vignettingCorrection == VignettingCorrection_Software) {
        //  for(s32 y=0; y<3; y++) {
        //    for(s32 x=0; x<3; x++) {
        //      grayscaleImage[y][x] = 0;
        //    }
        //  }
        //}

        //if(frameNumber % 25 == 0) {
        //  if(vignettingCorrection == VignettingCorrection_Off)
        //    vignettingCorrection = VignettingCorrection_Software;
        //  else if(vignettingCorrection == VignettingCorrection_Software)
        //    vignettingCorrection = VignettingCorrection_Off;
        //}

        HAL::CameraSetParameters(exposureTime, vignettingCorrection == VignettingCorrection_CameraHardware);

        EndBenchmark("VisionSystem_CameraImagingPipeline");

#ifdef SEND_BINARY_IMAGE_ONLY
        DebugStream::SendBinaryImage(grayscaleImage, "Binary Robot Image", tracker_, trackerParameters_, VisionMemory::ccmScratch_, VisionMemory::onchipScratch_, VisionMemory::offchipScratch_);
        HAL::MicroWait(250000);
#else
        {
          const bool sendSmall = false;
          
          if(sendSmall) {
            MemoryStack ccmScratch_local = MemoryStack(VisionMemory::ccmScratch_);
            
            Array<u8> grayscaleImageSmall(30, 40, ccmScratch_local);
            u32 downsampleFactor = DownsampleHelper(grayscaleImage, grayscaleImageSmall, ccmScratch_local);
            DebugStream::SendImage(grayscaleImageSmall, exposureTime, "Robot Image", VisionMemory::offchipScratch_);
          } else {
            DebugStream::SendImage(grayscaleImage, exposureTime, "Robot Image", VisionMemory::offchipScratch_);
            HAL::MicroWait(166666); // 6fps
            //HAL::MicroWait(140000); //7fps
            //HAL::MicroWait(125000); //8fps
          }
        }
#endif

        return RESULT_OK;
      } // Update() [SEND_IMAGE_ONLY]
#elif defined(RUN_GROUND_TRUTHING_CAPTURE) // #if defined(SEND_IMAGE_ONLY)
      Result Update(const Messages::RobotState robotState)
      {
        // This should be called from elsewhere first, but calling it again won't hurt
        Init();

        VisionMemory::ResetBuffers();

        frameNumber++;

        const s32 captureHeight = Vision::CameraResInfo[captureResolution_].height;
        const s32 captureWidth  = Vision::CameraResInfo[captureResolution_].width;

        Array<u8> grayscaleImage(captureHeight, captureWidth, VisionMemory::onchipScratch_, Flags::Buffer(false,false,false));

        HAL::CameraGetFrame(reinterpret_cast<u8*>(grayscaleImage.get_buffer()), captureResolution_, false);

        if(vignettingCorrection == VignettingCorrection_Software) {
          MemoryStack onchipScratch_local = VisionMemory::onchipScratch_;
          FixedLengthList<f32> polynomialParameters(5, onchipScratch_local, Flags::Buffer(false, false, true));

          for(s32 i=0; i<5; i++)
            polynomialParameters[i] = vignettingCorrectionParameters[i];

          CorrectVignetting(grayscaleImage, polynomialParameters);
        } // if(vignettingCorrection == VignettingCorrection_Software)
        
        DebugStream::SendImage(grayscaleImage, exposureTime, "Robot Image", VisionMemory::offchipScratch_);
        
        HAL::CameraSetParameters(exposureTime, vignettingCorrection == VignettingCorrection_CameraHardware);
        
        exposureTime += .1f;
        
        if(exposureTime > 1.01f) {
          exposureTime = 0.0f;
        }
        
        HAL::MicroWait(333333);
        //HAL::MicroWait(1500000);

        return RESULT_OK;
      } // Update() [RUN_GROUND_TRUTHING_CAPTURE]

#else

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
        Messages::SendRobotStateMsg(&robotState);
        
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

        if(mode_ == VISION_MODE_IDLE) {
          // Nothing to do, unless a snapshot was requested
          
        }
        else if(mode_ == VISION_MODE_LOOKING_FOR_MARKERS) {
          Simulator::SetDetectionReadyTime(); // no-op on real hardware

          VisionMemory::ResetBuffers();

          assert(false); while(true); // XXX: Implement me for offboard version!

        } else if(mode_ == VISION_MODE_TRACKING) {
          Simulator::SetTrackingReadyTime(); // no-op on real hardware

          //
          // Capture image for tracking
          //

          assert(false); while(true); // XXX: Implement me for offboard version!

        } else if(mode_ == VISION_MODE_DETECTING_FACES) {
          Simulator::SetFaceDetectionReadyTime();
          
          VisionMemory::ResetBuffers();
          
          AnkiConditionalErrorAndReturnValue(faceDetectionParameters_.isInitialized, RESULT_FAIL,
                                             "VisionSystem::Update::FaceDetectionParametersNotInitialized",
                                             "Face detection parameters not initialized before Update() in DETECTING_FACES mode.\n");
          
          assert(false); while(true); // XXX: Implement me for offboard version!

        } else if(mode_ == VISION_MODE_SEND_IMAGES_TO_BASESTATION) {

          Simulator::SetSendWifiImageReadyTime();

          VisionMemory::ResetBuffers();
                    
          assert(false); while(true); // XXX: Implement me for offboard version!
          
        } else {
          PRINT("VisionSystem::Update(): unknown mode = %d.", mode_);
          return RESULT_FAIL;
        } // if(converged)

        return lastResult;
      } // Update() [Real]

#endif // #ifdef SEND_IMAGE_ONLY
      
      
      void SetParams(const bool autoExposureOn,
                     const f32 exposureTime,
                     const s32 integerCountsIncrement,
                     const f32 minExposureTime,
                     const f32 maxExposureTime,
                     const u8 highValue,
                     const f32 percentileToMakeHigh,
                     const bool limitFramerate)
      {
        autoExposure_enabled = autoExposureOn;
        exposureTime_ = exposureTime;
        autoExposure_integerCountsIncrement = integerCountsIncrement;
        autoExposure_minExposureTime = minExposureTime;
        autoExposure_maxExposureTime = maxExposureTime;
        autoExposure_highValue = highValue;
        autoExposure_percentileToMakeHigh = percentileToMakeHigh;
        wifiCamera_limitFramerate = limitFramerate;
        
        PRINT("Changed VisionSystem params: autoExposureOn d exposureTime %f integerCountsInc %d, minExpTime %f, maxExpTime %f, highVal %d, percToMakeHigh %f\n",
              autoExposure_enabled,
              exposureTime_,
              autoExposure_integerCountsIncrement,
              autoExposure_minExposureTime,
              autoExposure_maxExposureTime,
              autoExposure_highValue,
              autoExposure_percentileToMakeHigh);
      }
      
      void SetFaceDetectParams(const f32 scaleFactor,
                               const s32 minNeighbors,
                               const s32 minObjectHeight,
                               const s32 minObjectWidth,
                               const s32 maxObjectHeight,
                               const s32 maxObjectWidth)
      {
        PRINT("Updated VisionSystem FaceDetect params\n");
        faceDetectionParameters_.scaleFactor = scaleFactor;
        faceDetectionParameters_.minNeighbors = minNeighbors;
        faceDetectionParameters_.minHeight = minObjectHeight;
        faceDetectionParameters_.minWidth = minObjectWidth;
        faceDetectionParameters_.maxHeight = maxObjectHeight;
        faceDetectionParameters_.maxWidth = maxObjectWidth;
      }
      
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki
