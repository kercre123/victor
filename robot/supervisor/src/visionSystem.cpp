
#include "anki/common/robot/config.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/serialize.h"

#include "anki/vision/robot/docking_vision.h"
#include "anki/vision/robot/imageProcessing.h"
#include "anki/vision/robot/lucasKanade.h"
#include "anki/vision/robot/miscVisionKernels.h"

#include "anki/common/shared/radians.h"

#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/messages.h"
#include "anki/cozmo/robot/visionSystem.h"

#include "headController.h"

// Set to 1 to use LucasKanadeTrackerFast, 0 for LucasKanadeTracker_f32
#define USE_FAST_LK 1

#if defined(SIMULATOR) && ANKICORETECH_EMBEDDED_USE_MATLAB && (!defined(USE_OFFBOARD_VISION) || !USE_OFFBOARD_VISION)
#define USE_MATLAB_VISUALIZATION 1
#else
#define USE_MATLAB_VISUALIZATION 0
#endif

#if USE_MATLAB_VISUALIZATION
#include "anki/common/robot/matlabInterface.h"
#endif

namespace Anki {
  namespace Cozmo {

    typedef enum {
      IDLE,
      LOOKING_FOR_BLOCKS,
      DOCKING,
      MAT_LOCALIZATION,
      VISUAL_ODOMETRY,
      CAPTURE_IMAGES // Do no robot control, just capture a buffer of images and send it to the PC over USB
    } Mode;

    // Private "Members" of the VisionSystem:
    namespace {

      // Constants / parameters:
      const s32 NUM_TRACKING_PYRAMID_LEVELS = 2;
      const f32 TRACKING_RIDGE_WEIGHT = 0.f;

      const s32 TRACKING_MAX_ITERATIONS = 25;
      const f32 TRACKING_CONVERGENCE_TOLERANCE = .05f;
      const bool TRACKING_USE_WEIGHTS = true;

      bool isInitialized_ = false;

      //
      // Memory
      //

      // CMX buffer
      const u32 CMX_BUFFER_SIZE = 600000;

      // DDR buffer (for captured frames at the moment)
      const u32 FRAMEBUFFER_WIDTH  = 640;
      const u32 FRAMEBUFFER_HEIGHT = 480;
      const u32 DDR_BUFFER_SIZE   = FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT;

#ifdef USE_CAPTURE_IMAGES
      const u32 CAPTURE_IMAGES_BUFFER_SIZE = 50000000;
      __attribute__((section(".ddr.bss"))) u8 captureImagesBufferRaw_[CAPTURE_IMAGES_BUFFER_SIZE] ALIGNVARIABLE;
      Embedded::SerializedBuffer captureImagesBuffer_;
      s32 numCapturedImages = -1;
      s32 currentWarmupImage = 0;
      const s32 NUM_WARMUP_IMAGES = 60;
      const s32 MAX_IMAGES_TO_CAPTURE = 5;
#ifdef USE_QVGA_CAMERA
      const HAL::CameraMode cameraMode = HAL::CAMERA_MODE_QVGA;
#else
      const HAL::CameraMode cameraMode = HAL::CAMERA_MODE_VGA;
#endif
#endif

#ifdef SIMULATOR
      char cmxBuffer_[CMX_BUFFER_SIZE];
      u8   ddrBuffer_[DDR_BUFFER_SIZE] ALIGNVARIABLE;

      u32 frameRdyTimeUS_ = 0;
      const u32 LOOK_FOR_BLOCK_PERIOD_US = 200000;
      const u32 TRACK_BLOCK_PERIOD_US = 100000;
#else
      __attribute__((section(".cmx.bss"))) char cmxBuffer_[CMX_BUFFER_SIZE];
      __attribute__((section(".ddr.bss"))) u8   ddrBuffer_[DDR_BUFFER_SIZE] ALIGNVARIABLE;
#endif

      const u32 TRACKER_SCRATCH_SIZE   = 600000;
      const u32 DETECTOR_SCRATCH1_SIZE = 300000;
      const u32 DETECTOR_SCRATCH2_SIZE = 300000;

      Embedded::MemoryStack trackerScratch_;
      Embedded::MemoryStack detectorScratch1_, detectorScratch2_;
      bool detectorScratchInitialized_ = false;

      Mode mode_ = IDLE;

      const HAL::CameraInfo* headCamInfo_ = NULL;
      const HAL::CameraInfo* matCamInfo_  = NULL;

      // Whether or not we're in the process of waiting for an image to be acquired
      // TODO: need one of these for both mat and head cameras?
      //bool continuousCaptureStarted_ = false;

      u16  dockingBlock_ = 0;
      bool isDockingBlockFound_ = false;

      bool isTemplateInitialized_ = false;

      // The tracker can fail to converge this many times before we give up
      // and reset the docker
      const u8 MAX_TRACKING_FAILURES = 5;
      u8 numTrackFailures_ = 0;

      f32 matCamPixPerMM_ = 1.f;

#if USE_FAST_LK
      Embedded::TemplateTracker::LucasKanadeTrackerFast tracker_;
#else
      Embedded::TemplateTracker::LucasKanadeTracker_f32 tracker_;
#endif

      Embedded::Quadrilateral<f32> trackingQuad_;

#if USE_MATLAB_VISUALIZATION
      Embedded::Matlab matlabViz_(false);
#endif
    } // private namespace

    namespace VisionSystem {
#pragma mark --- VisionSystem "Private Member Variables" ---

      //
      // Forward declarations:
      //

      // Capture an entire frame using HAL commands and put it in the given
      // frame buffer
      typedef struct {
        u8* data;
        HAL::CameraMode resolution;
        TimeStamp  timestamp;
      } FrameBuffer;

      ReturnCode CaptureHeadFrame(FrameBuffer &frame);
      ReturnCode CaptureMatFrame(FrameBuffer &frame);

      ReturnCode LookForBlocks(const FrameBuffer &frame);

      ReturnCode LocalizeWithMat(const FrameBuffer &frame);

      ReturnCode InitTemplate(const FrameBuffer &frame,
                              Embedded::Quadrilateral<f32>& templateRegion);

      ReturnCode TrackTemplate(const FrameBuffer &frame);

      ReturnCode GetRelativeOdometry(const FrameBuffer &frame);


#pragma mark --- VisionSystem Method Implementations ---


      ReturnCode Init(void)
      {
        isInitialized_ = false;

        headCamInfo_ = HAL::GetHeadCamInfo();
        if(headCamInfo_ == NULL) {
          PRINT("VisionSystem::Init() - HeadCam Info pointer is NULL!\n");
          return EXIT_FAILURE;
        }

        matCamInfo_  = HAL::GetMatCamInfo();
        if(matCamInfo_ == NULL) {
          PRINT("VisionSystem::Init() - MatCam Info pointer is NULL!\n");
          return EXIT_FAILURE;
        }

        // Compute the resolution of the mat camera from its FOV and height
        // off the mat:
        f32 matCamHeightInPix = ((static_cast<f32>(matCamInfo_->nrows)*.5f) /
                                 tanf(matCamInfo_->fov_ver * .5f));
        matCamPixPerMM_ = matCamHeightInPix / MAT_CAM_HEIGHT_FROM_GROUND_MM;

#if USE_OFFBOARD_VISION
        PRINT("VisionSystem::Init(): Registering message IDs for offboard processing.\n");

        // Register all the message IDs we need with Matlab:
        HAL::SendMessageID("CozmoMsg_BlockMarkerObserved",
                           GET_MESSAGE_ID(Messages::BlockMarkerObserved));

        HAL::SendMessageID("CozmoMsg_TemplateInitialized",
                           GET_MESSAGE_ID(Messages::TemplateInitialized));

        HAL::SendMessageID("CozmoMsg_TotalBlocksDetected",
                           GET_MESSAGE_ID(Messages::TotalBlocksDetected));

        HAL::SendMessageID("CozmoMsg_DockingErrorSignal",
                           GET_MESSAGE_ID(Messages::DockingErrorSignal));

        //HAL::SendMessageID("CozmoMsg_HeadCameraCalibration",
        //                   GET_MESSAGE_ID(Messages::HeadCameraCalibration));

        // TODO: Update this to send mat and head cam calibration separately
        PRINT("VisionSystem::Init(): Sending head camera calibration to "
              "offoard vision processor.\n");

        // Create a camera calibration message and send it to the offboard
        // vision processor
        Messages::HeadCameraCalibration headCalibMsg = {
          headCamInfo_->focalLength_x,
          headCamInfo_->focalLength_y,
          headCamInfo_->fov_ver,
          headCamInfo_->center_x,
          headCamInfo_->center_y,
          headCamInfo_->skew,
          headCamInfo_->nrows,
          headCamInfo_->ncols
        };

        //HAL::USBSendMessage(&msg, GET_MESSAGE_ID(HeadCameraCalibration));
        HAL::USBSendPacket(HAL::USB_VISION_COMMAND_HEAD_CALIBRATION,
                           &headCalibMsg, sizeof(Messages::HeadCameraCalibration));

        /* Don't need entire calibration, just pixPerMM

        Messages::MatCameraCalibration matCalibMsg = {
          matCamInfo_->focalLength_x,
          matCamInfo_->focalLength_y,
          matCamInfo_->fov_ver,
          matCamInfo_->center_x,
          matCamInfo_->center_y,
          matCamInfo_->skew,
          matCamInfo_->nrows,
          matCamInfo_->ncols
        };

        HAL::USBSendPacket(HAL::USB_VISION_COMMAND_MAT_CALIBRATION,
                           &matCalibMsg, sizeof(Messages::MatCameraCalibration));
         */
        HAL::USBSendPacket(HAL::USB_VISION_COMMAND_MAT_CALIBRATION,
                           &matCamPixPerMM_, sizeof(matCamPixPerMM_));

#endif // USE_OFFBOARD_VISION

#if USE_MATLAB_VISUALIZATION
        matlabViz_.EvalStringEcho("h_fig  = figure('Name', 'VisionSystem'); "
                                  "h_axes = axes('Pos', [.1 .1 .8 .8], 'Parent', h_fig); "
                                  "h_img  = imagesc(0, 'Parent', h_axes); "
                                  "axis(h_axes, 'image', 'off'); "
                                  "hold(h_axes, 'on'); "
                                  "colormap(h_fig, gray); "
                                  "h_trackedQuad = plot(nan, nan, 'b', 'LineWidth', 2, "
                                  "                     'Parent', h_axes); "
                                  "imageCtr = 0;");
#endif

#ifdef USE_CAPTURE_IMAGES
      captureImagesBuffer_ = Embedded::SerializedBuffer(&captureImagesBufferRaw_[0], CAPTURE_IMAGES_BUFFER_SIZE);
      numCapturedImages = 0;

      {
        FrameBuffer frame = {
          ddrBuffer_,
          cameraMode
        };

        const HAL::CameraUpdateMode updateMode = HAL::CAMERA_UPDATE_CONTINUOUS;

        CameraStartFrame(HAL::CAMERA_FRONT, frame.data, frame.resolution,
                         updateMode, 0, false);

        CameraSetIsEndOfFrame(HAL::CAMERA_FRONT, false);
      }

#endif

        isInitialized_ = true;
        return EXIT_SUCCESS;
      }


      bool IsInitialized()
      {
        return isInitialized_;
      }

      void Destroy()
      {

      }

      void StopTracking()
      {
        mode_ = IDLE;
      }

      ReturnCode SetDockingBlock(const u16 blockTypeToDockWith)
      {
        dockingBlock_          = blockTypeToDockWith;
        isDockingBlockFound_   = false;
        isTemplateInitialized_ = false;
        numTrackFailures_      = 0;

        mode_ = LOOKING_FOR_BLOCKS;

#if USE_OFFBOARD_VISION
        // Let the offboard vision processor know that it should be looking for
        // this block type as well, so it can do template initialization on the
        // same frame where it sees the block, instead of needing a second
        // USBSendFrame call.
        HAL::USBSendPacket(HAL::USB_VISION_COMMAND_SETDOCKBLOCK,
                           &dockingBlock_, sizeof(dockingBlock_));
#endif
        return EXIT_SUCCESS;
      }

      void CheckForDockingBlock(const u16 blockType)
      {
        // If we have a block to dock with set, see if this was it
        if(dockingBlock_ > 0 && dockingBlock_ == blockType)
        {
          isDockingBlockFound_ = true;
        }
      }

      void SetDockingMode(const bool isTemplateInitalized)
      {
        if(isTemplateInitalized)
        {
          PRINT("Tracking template initialized, switching to DOCKING mode.\n");
          isTemplateInitialized_ = true;
          isDockingBlockFound_   = true;

          // If we successfully initialized a tracking template,
          // switch to docking mode.  Otherwise, we'll keep looking
          // for the block and try again
          mode_ = DOCKING;
        }
        else {
          isTemplateInitialized_ = false;
          isDockingBlockFound_   = false;
        }
      }

      void UpdateTrackingStatus(const bool didTrackingSucceed)
      {
        if(didTrackingSucceed) {
          // Reset the failure counter
          numTrackFailures_ = 0;
        }
        else {
          ++numTrackFailures_;
          if(numTrackFailures_ == MAX_TRACKING_FAILURES) {

            // This resets docking, puttings us back in LOOKING_FOR_BLOCKS mode
            SetDockingBlock(dockingBlock_);
            numTrackFailures_ = 0;
          }
        }
      } // UpdateTrackingStatus()


      ReturnCode Update(void)
      {
        ReturnCode retVal = EXIT_SUCCESS;

        // NOTE: for now, we are always capturing at full resolution and
        //       then downsampling as we send the frame out for offboard
        //       processing.  Once the hardware camera supports it, we should
        //       capture at the correct resolution directly and pass that in
        //       (and remove the downsampling from USBSendFrame()

#ifdef SIMULATOR
        if (HAL::GetMicroCounter() < frameRdyTimeUS_) {
          return retVal;
        }
#endif


#if USE_OFFBOARD_VISION

        //PRINT("VisionSystem::Update(): waiting for processing result.\n");

        Messages::ProcessUARTMessages();

        if(Messages::StillLookingForID())
        {
          // Still waiting, skip further vision processing below.
          return EXIT_SUCCESS;
        }
#endif

#if USE_CAPTURE_IMAGES
        // TODO: assign in the proper place
        mode_ = CAPTURE_IMAGES;
#endif

        switch(mode_)
        {
          case IDLE:
            // Nothing to do!
            break;

          case LOOKING_FOR_BLOCKS:
          {
            FrameBuffer frame = {
              ddrBuffer_,
              HAL::CAMERA_MODE_VGA
            };

            CaptureHeadFrame(frame);

            // Note that if a docking block was specified and we see it while
            // looking for blocks, a tracking template will be initialized and,
            // if that's successful, we will switch to DOCKING mode.
            retVal = LookForBlocks(frame);
#ifdef SIMULATOR
            frameRdyTimeUS_ = HAL::GetMicroCounter() + LOOK_FOR_BLOCK_PERIOD_US;
#endif
            break;
          }

          case DOCKING:
          {
            if(not isTemplateInitialized_) {
              PRINT("VisionSystem::Update(): Reached DOCKING mode without "
                    "template initialized.\n");
              retVal = EXIT_FAILURE;
            }
            else {
              VisionSystem::FrameBuffer frame = {
                ddrBuffer_,
                HAL::CAMERA_MODE_VGA
              };

              CaptureHeadFrame(frame);

              if(TrackTemplate(frame) == EXIT_FAILURE) {
                PRINT("VisionSystem::Update(): TrackTemplate() failed.\n");
                retVal = EXIT_FAILURE;
              }
            }
            break;
          }

          case CAPTURE_IMAGES:
          {
#ifdef USE_CAPTURE_IMAGES
            // Do no robot control, just capture a buffer of images and send it to the PC over USB
            if(!isInitialized_) {
              Init();
            }

            VisionSystem::FrameBuffer frame = {
              ddrBuffer_,
              cameraMode
            };

            if(numCapturedImages < MAX_IMAGES_TO_CAPTURE) {

              const s32 imageHeight = HAL::CameraModeInfo[cameraMode].height;
              
#ifdef USE_QVGA_CAMERA
              const s32 imageWidth = 2 * HAL::CameraModeInfo[cameraMode].width;
#else
              const s32 imageWidth = HAL::CameraModeInfo[cameraMode].width;
#endif
              Embedded::Array<u8> image(imageHeight, imageWidth, ddrBuffer_, imageHeight*imageWidth);

              // Wait for the capture of the current frame to finish
              while (!HAL::CameraIsEndOfFrame(HAL::CAMERA_FRONT))
              {
              }

              // TODO: this will be set automatically at some point
              CameraSetIsEndOfFrame(HAL::CAMERA_FRONT, false);
              
              currentWarmupImage++;
              if(currentWarmupImage < NUM_WARMUP_IMAGES)
                break; // break out of the case
      
              //frame.timestamp = HAL::GetTimeStamp();

              captureImagesBuffer_.PushBack(image);

              numCapturedImages++;
              if(numCapturedImages == MAX_IMAGES_TO_CAPTURE) {
                s32 startIndex;
                const u8 * bufferStart = reinterpret_cast<const u8*>(captureImagesBuffer_.get_memoryStack().get_validBufferStart(startIndex));
                const s32 validUsedBytes = captureImagesBuffer_.get_memoryStack().get_usedBytes() - startIndex;

                //PRINT("%d %d\n", bufferStart, validUsedBytes);
#ifdef SIMULATOR
              frameRdyTimeUS_ = HAL::GetMicroCounter() + TRACK_BLOCK_PERIOD_US;
#endif

                for(s32 i=0; i<Embedded::SERIALIZED_BUFFER_HEADER_LENGTH; i++) {
                  Anki::Cozmo::HAL::USBPutChar(Embedded::SERIALIZED_BUFFER_HEADER[i]);
                }

                HAL::USBSendBuffer(bufferStart, validUsedBytes);

                for(s32 i=0; i<Embedded::SERIALIZED_BUFFER_FOOTER_LENGTH; i++) {
                  Anki::Cozmo::HAL::USBPutChar(Embedded::SERIALIZED_BUFFER_FOOTER[i]);
                }
              } // if(numCapturedImages == MAX_IMAGES_TO_CAPTURE)
            } else {
              // We're done, so just spin
              while(1) {}
            } // if(numCapturedImages < MAX_IMAGES_TO_CAPTURE) ... else
#else
            PRINT("Error: capture images code is not compiled\n");
#endif
          } // case CAPTURE_IMAGES:
          break;
/*
          case MAT_LOCALIZATION:
          {
            VisionSystem::FrameBuffer frame = {
              ddrBuffer_,
              MAT_LOCALIZATION_RESOLUTION
            };

            CaptureMatFrame(frame);
            LocalizeWithMat(frame);

            break;
          }

          case VISUAL_ODOMETRY:
          {
            VisionSystem::FrameBuffer frame = {
              ddrBuffer_,
              MAT_ODOMETRY_RESOLUTION
            };

            CaptureMatFrame(frame);
            GetRelativeOdometry(frame);

            break;
          }
 */
          default:
            PRINT("VisionSystem::Update(): reached default case in switch statement.");
            retVal = EXIT_FAILURE;
            break;
        } // SWITCH(mode_)

        return retVal;

      } // Update()


      ReturnCode CaptureHeadFrame(FrameBuffer &frame)
      {
        // Only QQQVGA can be captured in SINGLE mode.
        // Other modes must be captured in CONTINUOUS mode otherwise you get weird
        // rolling sync effects.
        // NB: CONTINUOUS mode contains tears that could affect vision algorithms
        // if moving too fast.
        const HAL::CameraUpdateMode updateMode = (frame.resolution == HAL::CAMERA_MODE_QQQVGA ?
                                                  HAL::CAMERA_UPDATE_SINGLE :
                                                  HAL::CAMERA_UPDATE_CONTINUOUS);

        CameraStartFrame(HAL::CAMERA_FRONT, frame.data, frame.resolution,
                         updateMode, 0, false);

        while (!HAL::CameraIsEndOfFrame(HAL::CAMERA_FRONT))
        {
        }

        // TODO: this will be set automatically at some point
        CameraSetIsEndOfFrame(HAL::CAMERA_FRONT, false);

        frame.timestamp = HAL::GetTimeStamp();

        return EXIT_SUCCESS;

      } // CaptureHeadFrame()

      ReturnCode CaptureMatFrame(FrameBuffer &frame)
      {
        PRINT("CaptureMatFrame(): mat camera available yet.\n");
        return EXIT_FAILURE;
      }

      void DownsampleHelper(const FrameBuffer& frame, Embedded::Array<u8>& image,
                            const HAL::CameraMode outputResolution,
                            Embedded::MemoryStack scratch)
      {
        using namespace Embedded;

        const u16 outWidth  = HAL::CameraModeInfo[outputResolution].width;
        const u16 outHeight = HAL::CameraModeInfo[outputResolution].height;

        AnkiAssert(image.get_size(0) == outHeight &&
                   image.get_size(1) == outWidth);
        //image = Array<u8>(outHeight, outWidth, scratch);

        const s32 downsamplePower = static_cast<s32>(HAL::CameraModeInfo[outputResolution].downsamplePower[frame.resolution]);

        if(downsamplePower > 0)
        {
          const u16 frameWidth   = HAL::CameraModeInfo[frame.resolution].width;
          const u16 frameHeight  = HAL::CameraModeInfo[frame.resolution].height;

          PRINT("Downsampling [%d x %d] frame by %d.\n",
                frameWidth, frameHeight, (1 << downsamplePower));

          Array<u8> fullRes(frameHeight, frameWidth,
                            frame.data, frameHeight*frameWidth,
                            Embedded::Flags::Buffer(false,false,false));

          ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(fullRes,
                                                             downsamplePower,
                                                             image,
                                                             scratch);
        }
        else {
          // No need to downsample, but need to copy into CMX from DDR
          // TODO: ask pete if this is correct
          image.Set(frame.data, outHeight*outWidth);
        }

      } // DownsampleHelper()

      ReturnCode LookForBlocks(const FrameBuffer &frame)
      {
        ReturnCode retVal = EXIT_FAILURE;

#if USE_OFFBOARD_VISION

        // Send the offboard vision processor the frame, with the command
        // to look for blocks in it. Note that if we previsouly sent the
        // offboard processor a message to set the docking block type, it will
        // also initialize a template tracker once that block type is seen
        HAL::USBSendFrame(frame.data, frame.timestamp,
                          frame.resolution, DETECTION_RESOLUTION,
                          HAL::USB_VISION_COMMAND_DETECTBLOCKS);

        Messages::LookForID( GET_MESSAGE_ID(Messages::TotalBlocksDetected) );

        retVal = EXIT_SUCCESS;

#else  // NOT defined(USE_MATLAB_FOR_HEAD_CAMERA)

        // TODO: Call embedded vision block detector
        // For each block that's found, create a CozmoMsg_ObservedBlockMarkerMsg
        // and process it.

        if(not detectorScratchInitialized_)
        {
          PRINT("Initializing detector scratch memory.\n");
          detectorScratch1_ = Embedded::MemoryStack(cmxBuffer_, DETECTOR_SCRATCH1_SIZE);
          detectorScratch2_ = Embedded::MemoryStack(cmxBuffer_ + DETECTOR_SCRATCH1_SIZE,
                                                    DETECTOR_SCRATCH2_SIZE);
          detectorScratchInitialized_ = true;
        }

        if(detectorScratch1_.IsValid() && detectorScratch2_.IsValid())
        {
          // So that we don't leak memory allocating image, markers, and
          // homographies below, we want to ensure that memory gets popped
          // when this scope ends
          PUSH_MEMORY_STACK(detectorScratch2_);

          const u16 detectWidth  = HAL::CameraModeInfo[DETECTION_RESOLUTION].width;
          const u16 detectHeight = HAL::CameraModeInfo[DETECTION_RESOLUTION].height;

          Embedded::Array<u8> image(detectHeight, detectWidth, detectorScratch2_);
          DownsampleHelper(frame, image, DETECTION_RESOLUTION, detectorScratch2_);

          // TOOD: move these parameters up above?
          const s32 scaleImage_thresholdMultiplier = 49152; // .75 * (2^16) = 49152
          const s32 scaleImage_numPyramidLevels = 3;

          const s32 component1d_minComponentWidth = 0;
          const s32 component1d_maxSkipDistance = 0;

          const f32 minSideLength = 0.03f*static_cast<f32>(MAX(detectWidth,detectHeight));
          const f32 maxSideLength = 0.97f*static_cast<f32>(MIN(detectWidth,detectHeight));

          const s32 component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
          const s32 component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
          const s32 component_sparseMultiplyThreshold = 1000 << 5;
          const s32 component_solidMultiplyThreshold = 2 << 5;

          const s32 component_percentHorizontal = 1 << 7; // 0.5, in SQ 23.8
          const s32 component_percentVertical = 1 << 7; // 0.5, in SQ 23.8

          const s32 maxExtractedQuads = 1000/2;
          const s32 quads_minQuadArea = 100/4;
          const s32 quads_quadSymmetryThreshold = 384;
          const s32 quads_minDistanceFromImageEdge = 2;

          const f32 decode_minContrastRatio = 1.25;

          const s32 maxMarkers = 100;
          const s32 maxConnectedComponentSegments = 25000/2;

          Embedded::FixedLengthList<Embedded::BlockMarker> markers(maxMarkers, detectorScratch2_);
          Embedded::FixedLengthList<Embedded::Array<f64> > homographies(maxMarkers, detectorScratch2_);

          for(s32 i=0; i<maxMarkers; i++) {
            Embedded::Array<f64> newArray(3, 3, detectorScratch2_);
            homographies[i] = newArray;
          } // for(s32 i=0; i<maximumSize; i++)

#if USE_MATLAB_VISUALIZATION
          matlabViz_.EvalStringEcho("delete(findobj(h_axes, 'Tag', 'DetectedQuad'));");
          matlabViz_.PutArray(image, "detectionImage");
          matlabViz_.EvalStringEcho("set(h_img, 'CData', detectionImage); "
                                    "set(h_axes, 'XLim', [.5 size(detectionImage,2)+.5], "
                                    "            'YLim', [.5 size(detectionImage,1)+.5]);");
#endif

          if(SimpleDetector_Steps12345_lowMemory(image,
                                                 markers,
                                                 homographies,
                                                 scaleImage_numPyramidLevels,
                                                 scaleImage_thresholdMultiplier,
                                                 component1d_minComponentWidth,
                                                 component1d_maxSkipDistance,
                                                 component_minimumNumPixels,
                                                 component_maximumNumPixels,
                                                 component_sparseMultiplyThreshold,
                                                 component_solidMultiplyThreshold,
                                                 component_percentHorizontal,
                                                 component_percentVertical,
                                                 quads_minQuadArea,
                                                 quads_quadSymmetryThreshold,
                                                 quads_minDistanceFromImageEdge,
                                                 decode_minContrastRatio,
                                                 maxConnectedComponentSegments,
                                                 maxExtractedQuads,
                                                 detectorScratch1_,
                                                 detectorScratch2_) == Embedded::RESULT_OK)
          {
            for(s32 i_marker = 0; i_marker < markers.get_size(); ++i_marker)
            {
              const Embedded::BlockMarker& crntMarker = markers[i_marker];

              // TODO: convert corners from shorts (fixed point?) to floats
              Messages::BlockMarkerObserved msg = {
                frame.timestamp,
                HeadController::GetAngleRad(), // headAngle
                static_cast<f32>(crntMarker.corners[0].x),
                static_cast<f32>(crntMarker.corners[0].y),
                static_cast<f32>(crntMarker.corners[1].x),
                static_cast<f32>(crntMarker.corners[1].y),
                static_cast<f32>(crntMarker.corners[2].x),
                static_cast<f32>(crntMarker.corners[2].y),
                static_cast<f32>(crntMarker.corners[3].x),
                static_cast<f32>(crntMarker.corners[3].y),
                static_cast<u16>(crntMarker.blockType),
                static_cast<u8>(crntMarker.faceType),
                static_cast<u8>(crntMarker.orientation)
              };

              Messages::ProcessBlockMarkerObservedMessage(msg);

#if USE_MATLAB_VISUALIZATION
              matlabViz_.PutQuad(crntMarker.corners, "detectedQuad");
              matlabViz_.EvalStringEcho("plot(detectedQuad([1 2 4 3 1],1)+1, "
                                        "     detectedQuad([1 2 4 3 1],2)+1, "
                                        "     'r', 'LineWidth', 2, "
                                        "     'Parent', h_axes, "
                                        "     'Tag', 'DetectedQuad');");
#endif

              // Processing the message could have set isDockingBlockFound to true
              // If it did, and we haven't already initialized the template tracker
              // (thanks to a previous marker setting isDockingBlockFound to true),
              // then initialize the template tracker now
              if(isDockingBlockFound_ && not isTemplateInitialized_)
              {
                using namespace Embedded;

                // I'd rather only initialize trackingQuad_ if InitTemplate()
                // succeeds, but InitTemplate downsamples it for the time being,
                // since we're still doing template initialization at tracking
                // resolution instead of the eventual goal of doing it at full
                // detection resolution.
                trackingQuad_ = Quadrilateral<f32>(Point<f32>(crntMarker.corners[0].x,
                                                              crntMarker.corners[0].y),
                                                   Point<f32>(crntMarker.corners[1].x,
                                                              crntMarker.corners[1].y),
                                                   Point<f32>(crntMarker.corners[2].x,
                                                              crntMarker.corners[2].y),
                                                   Point<f32>(crntMarker.corners[3].x,
                                                              crntMarker.corners[3].y));

                if(InitTemplate(frame, trackingQuad_) == EXIT_SUCCESS)
                {
                  SetDockingMode(static_cast<bool>(true));
                }

              } // if(isDockingBlockFound_ && not isTemplateInitialized_)
            } // for( each marker)
#if USE_MATLAB_VISUALIZATION
            matlabViz_.EvalString("drawnow");
#endif
            retVal = EXIT_SUCCESS;

          } // IF SimpleDetector_Steps12345_lowMemory() == RESULT_OK
        } // IF detectorScratch1 and 2 are valid

#endif // USE_OFFBOARD_VISION

        return retVal;

      } // lookForBlocks()


      ReturnCode LocalizeWithMat(const FrameBuffer &frame)
      {
        ReturnCode retVal = -1;

#if USE_OFFBOARD_VISION

        // Send the offboard vision processor the frame, with the command
        // to do mat localization
        HAL::USBSendFrame(frame.data, frame.timestamp,
                          frame.resolution, MAT_LOCALIZATION_RESOLUTION,
                          HAL::USB_VISION_COMMAND_MATLOCALIZATION);

        Messages::LookForID( GET_MESSAGE_ID(Messages::MatMarkerObserved) );

#else  // if USE_OFFBOARD_VISION
        /*
         // TODO: Hook this up to Pete's vision code
         PRINT("Robot::processMatImage(): embedded vision "
         "processing not hooked up yet.\n");
         retVal = -1;
         */
#endif // defined(USE_MATLAB_FOR_MAT_CAMERA)

        return retVal;

      } // localizeWithMat()


      ReturnCode InitTemplate(const FrameBuffer            &frame,
                              Embedded::Quadrilateral<f32> &templateQuad)
      {
        ReturnCode retVal = EXIT_FAILURE;
        isTemplateInitialized_ = false;

#if USE_OFFBOARD_VISION

        // When using offboard vision processing, template initialization is
        // rolled into looking for blocks, to ensure that the same frame in
        // which a desired docking block was detected is also used to
        // initialize the tracking template (without re-sending the frame
        // over USB).

        retVal = EXIT_SUCCESS;

#else
        trackerScratch_ = Embedded::MemoryStack(cmxBuffer_, TRACKER_SCRATCH_SIZE);

        detectorScratchInitialized_ = false;

        if(trackerScratch_.IsValid()) {

          using namespace Embedded::TemplateTracker;

          // TODO: At some point template initialization should happen at full detection resolution
          //       but for now, we have to downsample to tracking resolution
          const u16 trackHeight = HAL::CameraModeInfo[TRACKING_RESOLUTION].height;
          const u16 trackWidth  = HAL::CameraModeInfo[TRACKING_RESOLUTION].width;

          // NOTE: this image will sit in the MemoryStack until tracking fails
          //       and we reconstruct the MemoryStack
          Embedded::Array<u8> image(trackHeight, trackWidth, trackerScratch_);
          DownsampleHelper(frame, image, TRACKING_RESOLUTION, trackerScratch_);

          // Note that the templateRegion and the trackingQuad are both at
          // DETECTION_RESOLUTION, not necessarily the resolution of the frame.
          const s32 downsamplePower = static_cast<s32>(HAL::CameraModeInfo[TRACKING_RESOLUTION].downsamplePower[DETECTION_RESOLUTION]);
          const f32 downsampleFactor = static_cast<f32>(1 << downsamplePower);

          for(u8 i=0; i<4; ++i) {
            templateQuad[i].x /= downsampleFactor;
            templateQuad[i].y /= downsampleFactor;
          }

#if USE_FAST_LK
          tracker_ = LucasKanadeTrackerFast(image, templateQuad,
                                            NUM_TRACKING_PYRAMID_LEVELS,
                                            TRANSFORM_AFFINE,
                                            TRACKING_RIDGE_WEIGHT,
                                            trackerScratch_);
#else
          tracker_ = LucasKanadeTracker_f32(image, templateQuad,
                                            NUM_TRACKING_PYRAMID_LEVELS,
                                            TRANSFORM_AFFINE,
                                            TRACKING_RIDGE_WEIGHT,
                                            trackerScratch_);
#endif

          if(tracker_.IsValid()) {
            retVal = EXIT_SUCCESS;
          }

        } // if trackerScratch is valid

#endif // USE_OFFBOARD_VISION

        if(retVal == EXIT_SUCCESS) {
          isTemplateInitialized_ = true;
        }

        return retVal;

      } // InitTemplate()


      ReturnCode TrackTemplate(const FrameBuffer &frame)
      {
        ReturnCode retVal = EXIT_SUCCESS;

#if USE_OFFBOARD_VISION

        // Send the message out for tracking
        HAL::USBSendFrame(frame.data, frame.timestamp,
                          frame.resolution, TRACKING_RESOLUTION,
                          HAL::USB_VISION_COMMAND_TRACK);

        Messages::LookForID( GET_MESSAGE_ID(Messages::DockingErrorSignal) );

#else // ONBOARD VISION

        if(trackerScratch_.IsValid())
        {
          // So that we don't leak memory allocating image, etc, below,
          // we want to ensure that memory gets popped when this scope ends
          PUSH_MEMORY_STACK(trackerScratch_);

          //PRINT("TrackerScratch memory usage = %d of %d\n", trackerScratch_.get_usedBytes(), trackerScratch_.get_totalBytes());

          using namespace Embedded::TemplateTracker;

          AnkiAssert(tracker_.IsValid());

          const u16 trackWidth  = HAL::CameraModeInfo[TRACKING_RESOLUTION].width;
          const u16 trackHeight = HAL::CameraModeInfo[TRACKING_RESOLUTION].height;

          Embedded::Array<u8> image(trackHeight, trackWidth, trackerScratch_);
          DownsampleHelper(frame, image, TRACKING_RESOLUTION, trackerScratch_);

          Messages::DockingErrorSignal dockErrMsg;
          dockErrMsg.timestamp = frame.timestamp;

          bool converged;
#if USE_FAST_LK
          if(tracker_.UpdateTrack(image, TRACKING_MAX_ITERATIONS,
                                  TRACKING_CONVERGENCE_TOLERANCE,
                                  converged,
                                  trackerScratch_) == Embedded::RESULT_OK)
#else
          if(tracker_.UpdateTrack(image, TRACKING_MAX_ITERATIONS,
                                  TRACKING_CONVERGENCE_TOLERANCE,
                                  TRACKING_USE_WEIGHTS,
                                  converged,
                                  trackerScratch_) == Embedded::RESULT_OK)
#endif
          {
            dockErrMsg.didTrackingSucceed = static_cast<u8>(converged);
            if(converged)
            {
              using namespace Embedded;

              // TODO: Add CameraMode resolution to CameraInfo
              const f32 fxAdj = (static_cast<f32>(headCamInfo_->ncols) /
                                 static_cast<f32>(HAL::CameraModeInfo[TRACKING_RESOLUTION].width));

              Docking::ComputeDockingErrorSignal(tracker_.get_transformation(),
                                                 HAL::CameraModeInfo[TRACKING_RESOLUTION].width,
                                                 BLOCK_MARKER_WIDTH_MM,
                                                 headCamInfo_->focalLength_x / fxAdj,
                                                 dockErrMsg.x_distErr,
                                                 dockErrMsg.y_horErr,
                                                 dockErrMsg.angleErr,
                                                 trackerScratch_);


            } // IF converged

#if USE_MATLAB_VISUALIZATION
            matlabViz_.PutArray(image, "trackingImage");
//            matlabViz_.EvalStringExplicit("imwrite(trackingImage, "
//                                          "sprintf('~/temp/trackingImage%.3d.png', imageCtr)); "
//                                          "imageCtr = imageCtr + 1;");
            matlabViz_.EvalStringEcho("set(h_img, 'CData', trackingImage); "
                                      "set(h_axes, 'XLim', [.5 size(trackingImage,2)+.5], "
                                      "            'YLim', [.5 size(trackingImage,1)+.5]);");

            if(dockErrMsg.didTrackingSucceed)
            {
              matlabViz_.PutQuad(tracker_.get_transformation().get_transformedCorners(trackerScratch_), "transformedQuad");
              matlabViz_.EvalStringEcho("set(h_trackedQuad, 'Visible', 'on', "
                                        "    'XData', transformedQuad([1 2 4 3 1],1)+1, "
                                        "    'YData', transformedQuad([1 2 4 3 1],2)+1); "
                                        "title(h_axes, 'Tracking Succeeded');");
            }
            else
            {
              matlabViz_.EvalStringEcho("set(h_trackedQuad, 'Visible', 'off'); "
                                        "title(h_axes, 'Tracking Failed');");
            }

            matlabViz_.EvalString("drawnow");

#endif // #if USE_MATLAB_VISUALIZATION

            Messages::ProcessDockingErrorSignalMessage(dockErrMsg);
          }
          else {
            PRINT("VisionSystem::TrackTemplate(): UpdateTrack() failed.\n");
            retVal = EXIT_FAILURE;
          } // IF UpdateTrack result was OK
        }
        else {
          PRINT("VisionSystem::TrackTemplate(): tracker scratch memory is not valid.\n");
          retVal = EXIT_FAILURE;
        } // if/else trackerScratch is valid

#endif // #if USE_OFFBOARD_VISION

        return retVal;

      } // TrackTemplate()

    } // namespace VisionSystem

  } // namespace Cozmo
} // namespace Anki
