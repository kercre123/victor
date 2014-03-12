#include "anki/common/robot/config.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/serialize.h"

#include "anki/vision/robot/docking_vision.h"
#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/fiducialMarkers.h"
#include "anki/vision/robot/imageProcessing.h"
#include "anki/vision/robot/lucasKanade.h"
#include "anki/vision/robot/binaryTracker.h"

#include "anki/common/shared/radians.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/benchmarking_c.h"

#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/messages.h"
#include "anki/cozmo/robot/visionSystem.h"

// TODO: make it so we don't have to include this entire file to get just the enums
#include "anki/vision/robot/visionMarkerDecisionTrees.h"

#include "headController.h"

#if defined(SIMULATOR) && ANKICORETECH_EMBEDDED_USE_MATLAB && (!defined(USE_OFFBOARD_VISION) || !USE_OFFBOARD_VISION)
#define USE_MATLAB_VISUALIZATION 1
#else
#define USE_MATLAB_VISUALIZATION 0
#endif

#if USE_MATLAB_VISUALIZATION
#include "anki/common/robot/matlabInterface.h"
#endif

// m_buffer1 (aka Mr. Bufferly) is where the camera image is currently stored
#define M_BUFFER1_SIZE (320*240*2)
extern u8 m_buffer1[];

// TODO: make nice
extern volatile bool isEOF;

// TODO: remove
#define SEND_DEBUG_STREAM
#define RUN_SIMPLE_TRACKING_TEST

namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
      typedef enum {
        IDLE,
        LOOKING_FOR_MARKERS,
        TRACKING
      } Mode;

#define DOCKING_LUCAS_KANADE_STANDARD 1 //< LucasKanadeTracker_f32
#define DOCKING_LUCAS_KANADE_FAST     2 //< LucasKanadeTrackerFast
#define DOCKING_BINARY_TRACKER        3 //< BinaryTracker
#define DOCKING_ALGORITHM DOCKING_BINARY_TRACKER

      // Private "Members" of the VisionSystem:
      namespace {
        // Constants / parameters:

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
        const s32 NUM_TRACKING_PYRAMID_LEVELS = 2;
        const f32 TRACKING_RIDGE_WEIGHT = 0.f;
        const s32 TRACKING_MAX_ITERATIONS = 25;
        const f32 TRACKING_CONVERGENCE_TOLERANCE = .05f;
        const bool TRACKING_USE_WEIGHTS = true;
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
        const u8 edgeDetection_grayvalueThreshold = 128; // TODO: extract from fiducial marker
        const s32 edgeDetection_minComponentWidth = 2;
        const s32 edgeDetection_maxDetectionsPerType = 2500;
        const s32 edgeDetection_everyNLines = 1;

        const s32 matching_maxTranslationDistance = 7;
        const s32 matching_maxProjectiveDistance = 7;
        const s32 verification_maxTranslationDistance = 1;
        const f32 percentMatchedPixelsThreshold = 0.5f; // TODO: pick a reasonable value
#endif

        bool isInitialized_ = false;

        //
        // Memory
        //

        const s32 OFFCHIP_BUFFER_SIZE = 2000000;
        const s32 ONCHIP_BUFFER_SIZE = 170000; // The max here is somewhere between 175000 and 180000 bytes
        const s32 CCM_BUFFER_SIZE = 50000; // The max here is probably 65536 (0x10000) bytes

#ifdef SEND_DEBUG_STREAM
        const s32 PRINTF_BUFFER_SIZE = 10000;
        static OFFCHIP u8 printfBufferRaw_[PRINTF_BUFFER_SIZE];
        Embedded::SerializedBuffer printfBuffer_;

        const s32 DEBUG_STREAM_BUFFER_SIZE = 2000000;
        static OFFCHIP u8 debugStreamBufferRaw_[DEBUG_STREAM_BUFFER_SIZE];
        Embedded::SerializedBuffer debugStreamBuffer_;
#endif

#ifdef SIMULATOR
        static char offchipBuffer[OFFCHIP_BUFFER_SIZE];
        static char onchipBuffer[ONCHIP_BUFFER_SIZE];
        static char ccmBuffer[CCM_BUFFER_SIZE];

        u32 frameRdyTimeUS_ = 0;
        const u32 LOOK_FOR_BLOCK_PERIOD_US = 200000;
        const u32 TRACK_BLOCK_PERIOD_US = 100000;
#else
        static OFFCHIP char offchipBuffer[OFFCHIP_BUFFER_SIZE];
        static ONCHIP char onchipBuffer[ONCHIP_BUFFER_SIZE];
        static CCM char ccmBuffer[CCM_BUFFER_SIZE];
#endif

        static Embedded::MemoryStack offchipScratch_;
        static Embedded::MemoryStack onchipScratch_;
        static Embedded::MemoryStack ccmScratch_;
        bool scratchInitialized_ = false;

        Mode mode_ = LOOKING_FOR_MARKERS;

        const HAL::CameraInfo* headCamInfo_ = NULL;
        //const HAL::CameraInfo* matCamInfo_  = NULL;

        // Whether or not we're in the process of waiting for an image to be acquired
        // TODO: need one of these for both mat and head cameras?
        //bool continuousCaptureStarted_ = false;

        Vision::MarkerType trackingMarker_;

        bool isTrackingMarkerFound_ = false;

        bool isTemplateInitialized_ = false;

        // The tracker can fail to converge this many times before we give up
        // and reset the docker
        const u8 MAX_TRACKING_FAILURES = 5;
        u8 numTrackFailures_ = 0;

        //f32 matCamPixPerMM_ = 1.f;

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
        Embedded::TemplateTracker::LucasKanadeTrackerFast tracker_;
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
        Embedded::TemplateTracker::LucasKanadeTracker_f32 tracker_;
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
        Embedded::TemplateTracker::BinaryTracker tracker_;
#endif

        Embedded::Quadrilateral<f32> trackingQuad_;

#if USE_MATLAB_VISUALIZATION
        Embedded::Matlab matlabViz_(false);
#endif
      } // private namespace

      //
      // Forward declarations:
      //

      // Capture an entire frame using HAL commands and put it in the given
      // frame buffer
      typedef struct {
        u8* data;
        HAL::CameraMode resolution;
        TimeStamp_t  timestamp;
      } FrameBuffer;

      //ReturnCode CaptureHeadFrame(FrameBuffer &frame);

      ReturnCode LookForMarkers(const FrameBuffer &frame);

      ReturnCode InitTemplate(const FrameBuffer &frame,
        Embedded::Quadrilateral<f32>& templateRegion);

      ReturnCode TrackTemplate(const FrameBuffer &frame);

      //ReturnCode GetRelativeOdometry(const FrameBuffer &frame);

      void DownsampleHelper(const FrameBuffer& frame, Embedded::Array<u8>& image,
        const HAL::CameraMode outputResolution,
        Embedded::MemoryStack scratch);

      void DownsampleHelper(const Embedded::Array<u8>& in,
        Embedded::Array<u8>& out,
        Embedded::MemoryStack scratch);

      ReturnCode YUVToGrayscaleHelper(const FrameBuffer& yuvFrame, Embedded::Array<u8>& grayscaleImage);

      // Warning, has side effects on local buffers
#warning forceReset is a big hack. This file should be fixed in a principled way.
      ReturnCode InitializeScratchBuffers(bool forceReset);

      //#pragma mark --- VisionSystem Method Implementations ---

      ReturnCode Init(void)
      {
        isInitialized_ = false;

#if 0
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
#endif // #if 0

        /*
        // Compute the resolution of the mat camera from its FOV and height
        // off the mat:
        f32 matCamHeightInPix = ((static_cast<f32>(matCamInfo_->nrows)*.5f) /
        tanf(matCamInfo_->fov_ver * .5f));
        matCamPixPerMM_ = matCamHeightInPix / MAT_CAM_HEIGHT_FROM_GROUND_MM;
        */

#if USE_OFFBOARD_VISION
        PRINT("VisionSystem::Init(): Registering message IDs for offboard processing.\n");

        // Register all the message IDs we need with Matlab:
        HAL::SendMessageID("CozmoMsg_BlockMarkerObserved",
          GET_MESSAGE_ID(Messages::BlockMarkerObserved));

        HAL::SendMessageID("CozmoMsg_TemplateInitialized",
          GET_MESSAGE_ID(Messages::TemplateInitialized));

        HAL::SendMessageID("CozmoMsg_TotalVisionMarkersSeen",
          GET_MESSAGE_ID(Messages::TotalVisionMarkersSeen));

        HAL::SendMessageID("CozmoMsg_DockingErrorSignal",
          GET_MESSAGE_ID(Messages::DockingErrorSignal));

        HAL::SendMessageID("CozmoMsg_VisionMarker",
          GET_MESSAGE_ID(Messages::VisionMarker));

        {
          for(s32 i=0; i<Vision::NUM_MARKER_TYPES; ++i) {
            HAL::SendMessageID(Vision::MarkerTypeStrings[i], i);
          }
        }

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
        /*
        HAL::USBSendPacket(HAL::USB_VISION_COMMAND_MAT_CALIBRATION,
        &matCamPixPerMM_, sizeof(matCamPixPerMM_));
        */
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

#ifdef SEND_DEBUG_STREAM
        debugStreamBuffer_ = Embedded::SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
#endif

        isInitialized_ = true;

        mode_ = LOOKING_FOR_MARKERS;

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

      ReturnCode SetMarkerToTrack(const Vision::MarkerType& markerToTrack)
      {
        trackingMarker_ = markerToTrack;

        isTrackingMarkerFound_ = false;
        isTemplateInitialized_ = false;
        numTrackFailures_      = 0;

        mode_ = LOOKING_FOR_MARKERS;

#if USE_OFFBOARD_VISION
        // Let the offboard vision processor know that it should be looking for
        // this block type as well, so it can do template initialization on the
        // same frame where it sees the block, instead of needing a second
        // USBSendFrame call.
        HAL::USBSendPacket(HAL::USB_VISION_COMMAND_SETTRACKMARKER,
          &trackingMarker_, sizeof(trackingMarker_));
#endif
        return EXIT_SUCCESS;
      }

      void CheckForTrackingMarker(const u16 inputMarker)
      {
        // If we have a block to dock with set, see if this was it
        if(trackingMarker_ == static_cast<Vision::MarkerType>(inputMarker))
        {
          isTrackingMarkerFound_ = true;
        }
      }

      void SetTrackingMode(const bool isTemplateInitalized)
      {
        if(isTemplateInitalized && isTrackingMarkerFound_)
        {
          //PRINT("Tracking template initialized, switching to DOCKING mode.\n");
          isTemplateInitialized_ = true;
          isTrackingMarkerFound_ = true;

          // If we successfully initialized a tracking template,
          // switch to docking mode.  Otherwise, we'll keep looking
          // for the block and try again
          mode_ = TRACKING;
        }
        else {
          isTemplateInitialized_ = false;
          isTrackingMarkerFound_ = false;
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
            // This resets docking, puttings us back in LOOKING_FOR_MARKERS mode
            SetMarkerToTrack(trackingMarker_);
            numTrackFailures_ = 0;
          }
        }
      } // UpdateTrackingStatus()

#ifdef SEND_DEBUG_STREAM
      void SendPrintf(const char * string)
      {
        printfBuffer_ = Embedded::SerializedBuffer(&printfBufferRaw_[0], PRINTF_BUFFER_SIZE);

        printfBuffer_.PushBackString(string);

        s32 startIndex;
        const u8 * bufferStart = reinterpret_cast<const u8*>(printfBuffer_.get_memoryStack().get_validBufferStart(startIndex));
        const s32 validUsedBytes = printfBuffer_.get_memoryStack().get_usedBytes() - startIndex;

        for(s32 i=0; i<Embedded::SERIALIZED_BUFFER_HEADER_LENGTH; i++) {
          Anki::Cozmo::HAL::USBPutChar(Embedded::SERIALIZED_BUFFER_HEADER[i]);
        }

        HAL::USBSendBuffer(bufferStart, validUsedBytes);

        for(s32 i=0; i<Embedded::SERIALIZED_BUFFER_FOOTER_LENGTH; i++) {
          Anki::Cozmo::HAL::USBPutChar(Embedded::SERIALIZED_BUFFER_FOOTER[i]);
        }
      } // void SendPrintf(const char * string)
#endif // #ifdef SEND_DEBUG_STREAM

#ifdef SEND_DEBUG_STREAM
      /*ReturnCode SendDebugStream_Detection(const Embedded::FixedLengthList<Embedded::VisionMarker> &markers, const FrameBuffer &frame)
      {
      } // ReturnCode SendDebugStream_Detection()*/
#endif // #ifdef SEND_DEBUG_STREAM

      ReturnCode Update(void)
      {
        ReturnCode retVal = EXIT_SUCCESS;

        InitializeScratchBuffers(false);

        FrameBuffer frame = {
          m_buffer1,
          HAL::CAMERA_MODE_QVGA
        };

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

#endif // USE_OFFBOARD_VISION

        /*#ifdef SEND_DEBUG_STREAM
        // TODO: assign in the proper place
        mode_ = DEBUG_STREAM;
        #endif*/
        //mode_ = IDLE;

        switch(mode_)
        {
        case IDLE:
          // Nothing to do!
          break;

        case LOOKING_FOR_MARKERS:
          {
            // TODO: fix
            //CaptureHeadFrame(frame);

            // Note that if a docking block was specified and we see it while
            // looking for blocks, a tracking template will be initialized and,
            // if that's successful, we will switch to DOCKING mode.
            retVal = LookForMarkers(frame);
#ifdef SIMULATOR
            frameRdyTimeUS_ = HAL::GetMicroCounter() + LOOK_FOR_BLOCK_PERIOD_US;
#endif
            break;
          }

        case TRACKING:
          {
            if(not isTemplateInitialized_) {
              PRINT("VisionSystem::Update(): Reached DOCKING mode without "
                "template initialized.\n");
              retVal = EXIT_FAILURE;
            }
            else {
              //CaptureHeadFrame(frame);

#ifdef SIMULATOR
              frameRdyTimeUS_ = HAL::GetMicroCounter() + TRACK_BLOCK_PERIOD_US;
#endif

              if(TrackTemplate(frame) == EXIT_FAILURE) {
                PRINT("VisionSystem::Update(): TrackTemplate() failed.\n");
                retVal = EXIT_FAILURE;
              }
            }
            break;
          }

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

      /*      ReturnCode CaptureHeadFrame(FrameBuffer &frame)
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
      } // CaptureHeadFrame()*/

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

          //PRINT("Downsampling [%d x %d] frame by %d.\n", frameWidth, frameHeight, (1 << downsamplePower));

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

      void DownsampleHelper(const Embedded::Array<u8>& in,
        Embedded::Array<u8>& out,
        Embedded::MemoryStack scratch)
      {
        using namespace Embedded;

        const s32 inWidth  = in.get_size(1);
        const s32 inHeight = in.get_size(0);

        const s32 outWidth  = out.get_size(1);
        const s32 outHeight = out.get_size(0);

        const u32 downsampleFactor = inWidth / outWidth;

        const u32 downsamplePower = Log2u32(downsampleFactor);

        if(downsamplePower > 0)
        {
          //PRINT("Downsampling [%d x %d] frame by %d.\n", inWidth, inHeight, (1 << downsamplePower));

          ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(in,
            downsamplePower,
            out,
            scratch);
        }
        else {
          // No need to downsample, just copy the buffer
          out.Set(in);
        }
      }

      ReturnCode YUVToGrayscaleHelper(const FrameBuffer& yuvFrame, Embedded::Array<u8>& grayscaleImage)
      {
        using namespace Embedded;

        const s32 imageHeight = grayscaleImage.get_size(0);
        const s32 imageWidth  = grayscaleImage.get_size(1);

        const HAL::CameraModeInfo_t cameraInfo = HAL::CameraModeInfo[yuvFrame.resolution];

        if(cameraInfo.height != imageHeight || cameraInfo.width != imageWidth) {
          PRINT("Error: YUVToGrayscaleHelper\n");
          return EXIT_FAILURE;
        }

        for(s32 y=0; y<imageHeight; y++) {
          const u8 * restrict pYuvFrame = yuvFrame.data + y*imageWidth*2;
          u8 * restrict pGrayscaleImage = grayscaleImage.Pointer(y,0);

          for(s32 x=0; x<imageWidth; x++) {
            pGrayscaleImage[x] = pYuvFrame[2*x];
          }
        }

        return EXIT_SUCCESS;
      } // void YUVToGrayscaleHelper(const FrameBuffer& yuvFrame, Embedded::Array<u8>& grayscaleImage)

      ReturnCode InitializeScratchBuffers(bool forceReset)
      {
        if(forceReset || !scratchInitialized_) {
          //PRINT("Initializing scratch memory.\n");

          offchipScratch_ = Embedded::MemoryStack(offchipBuffer, OFFCHIP_BUFFER_SIZE);
          onchipScratch_ = Embedded::MemoryStack(onchipBuffer, ONCHIP_BUFFER_SIZE);
          ccmScratch_ = Embedded::MemoryStack(ccmBuffer, CCM_BUFFER_SIZE);

          if(!offchipScratch_.IsValid() || !onchipScratch_.IsValid() || !ccmScratch_.IsValid()) {
            PRINT("Error: InitializeScratchBuffers\n");
            return EXIT_FAILURE;
          }

          scratchInitialized_ = true;
        }

        return EXIT_SUCCESS;
      }

      ReturnCode LookForMarkers(const FrameBuffer &frame)
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

        Messages::LookForID( GET_MESSAGE_ID(Messages::TotalVisionMarkersSeen) );

        retVal = EXIT_SUCCESS;

#else  // NOT defined(USE_MATLAB_FOR_HEAD_CAMERA)

        // TODO: Call embedded vision block detector
        // For each block that's found, create a CozmoMsg_ObservedBlockMarkerMsg
        // and process it.

        if(InitializeScratchBuffers(true) != EXIT_SUCCESS) {
          return EXIT_FAILURE;
        }

        {
          // So that we don't leak memory allocating image, markers, and
          // homographies below, we want to ensure that memory gets popped
          // when this scope ends
          PUSH_MEMORY_STACK(offchipScratch_);
          //PUSH_MEMORY_STACK(onchipScratch_);
          PUSH_MEMORY_STACK(ccmScratch_);

          //printf("Stacks usage: %d %d %d\n", offchipScratch_.get_usedBytes(), onchipScratch_.get_usedBytes(), ccmScratch_.get_usedBytes());

          const u16 detectWidth  = HAL::CameraModeInfo[DETECTION_RESOLUTION].width;
          const u16 detectHeight = HAL::CameraModeInfo[DETECTION_RESOLUTION].height;

          Embedded::Array<u8> image(detectHeight, detectWidth, offchipScratch_);
          YUVToGrayscaleHelper(frame, image);

          // TOOD: move these parameters up above?
          const s32 scaleImage_thresholdMultiplier = 65536; // 1.0*(2^16)=65536
          //const s32 scaleImage_thresholdMultiplier = 49152; // .75*(2^16)=49152
          const s32 scaleImage_numPyramidLevels = 3;

          const s32 component1d_minComponentWidth = 0;
          const s32 component1d_maxSkipDistance = 0;

          const f32 minSideLength = 0.03f*static_cast<f32>(MAX(detectWidth,detectHeight));
          const f32 maxSideLength = 0.97f*static_cast<f32>(MIN(detectWidth,detectHeight));

          const s32 component_minimumNumPixels = static_cast<s32>(Embedded::Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
          const s32 component_maximumNumPixels = static_cast<s32>(Embedded::Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
          const s32 component_sparseMultiplyThreshold = 1000 << 5;
          const s32 component_solidMultiplyThreshold = 2 << 5;

          //const s32 component_percentHorizontal = 1 << 7; // 0.5, in SQ 23.8
          //const s32 component_percentVertical = 1 << 7; // 0.5, in SQ 23.8
          const f32 component_minHollowRatio = 1.0f;

          const s32 maxExtractedQuads = 1000/2;
          const s32 quads_minQuadArea = 100/4;
          const s32 quads_quadSymmetryThreshold = 512; // ANS: corresponds to 2.0, loosened from 384 (1.5), for large mat markers at extreme perspective distortion
          const s32 quads_minDistanceFromImageEdge = 2;

          const f32 decode_minContrastRatio = 1.25;

          const s32 maxConnectedComponentSegments = 39000; // 322*240/2 = 38640

          const s32 maxMarkers = 100;

          Embedded::FixedLengthList<Embedded::VisionMarker> markers(maxMarkers, offchipScratch_);
          Embedded::FixedLengthList<Embedded::Array<f32> > homographies(maxMarkers, ccmScratch_);

          markers.set_size(maxMarkers);
          homographies.set_size(maxMarkers);

          for(s32 i=0; i<maxMarkers; i++) {
            Embedded::Array<f32> newArray(3, 3, ccmScratch_);
            homographies[i] = newArray;
          } // for(s32 i=0; i<maximumSize; i++)

#if USE_MATLAB_VISUALIZATION
          matlabViz_.EvalStringEcho("delete(findobj(h_axes, 'Tag', 'DetectedQuad'));");
          matlabViz_.PutArray(image, "detectionImage");
          matlabViz_.EvalStringEcho("set(h_img, 'CData', detectionImage); "
            "set(h_axes, 'XLim', [.5 size(detectionImage,2)+.5], "
            "            'YLim', [.5 size(detectionImage,1)+.5]);");
#endif

          InitBenchmarking();

          const Embedded::Result result = DetectFiducialMarkers(
            image,
            markers,
            homographies,
            scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
            component1d_minComponentWidth, component1d_maxSkipDistance,
            component_minimumNumPixels, component_maximumNumPixels,
            component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
            component_minHollowRatio,
            quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
            decode_minContrastRatio,
            maxConnectedComponentSegments,
            maxExtractedQuads,
            false,
            offchipScratch_, onchipScratch_, ccmScratch_);

          if(result == Embedded::RESULT_OK) {
#ifdef SEND_DEBUG_STREAM
            {
              // Pushing here at the top is necessary, because of all the global variables
              PUSH_MEMORY_STACK(offchipScratch_);
              PUSH_MEMORY_STACK(ccmScratch_);

              if(!isInitialized_) {
                Init();
              }

              debugStreamBuffer_ = Embedded::SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);

              // Stream the images as they come
              //const s32 imageHeight = HAL::CameraModeInfo[HAL::CAMERA_MODE_QVGA].height;
              //const s32 imageWidth = HAL::CameraModeInfo[HAL::CAMERA_MODE_QVGA].width;
              Embedded::Array<u8> imageLarge(240, 320, offchipScratch_);
              Embedded::Array<u8> imageSmall(60, 80, offchipScratch_);

              YUVToGrayscaleHelper(frame, imageLarge);
              DownsampleHelper(imageLarge, imageSmall, ccmScratch_);

              debugStreamBuffer_.PushBack(imageSmall);

              if(markers.get_size() != 0) {
                PUSH_MEMORY_STACK(offchipScratch_);

                const s32 numMarkers = markers.get_size();
                const Embedded::VisionMarker * pMarkers = markers.Pointer(0);

                void * restrict oneMarker = offchipScratch_.Allocate(sizeof(Embedded::VisionMarker));
                const s32 oneMarkerLength = sizeof(Embedded::VisionMarker);

                for(s32 i=0; i<numMarkers; i++) {
                  pMarkers[i].Serialize(oneMarker, oneMarkerLength);
                  debugStreamBuffer_.PushBack("VisionMarker", oneMarker, oneMarkerLength);
                }
              }

              s32 startIndex;
              const u8 * bufferStart = reinterpret_cast<const u8*>(debugStreamBuffer_.get_memoryStack().get_validBufferStart(startIndex));
              const s32 validUsedBytes = debugStreamBuffer_.get_memoryStack().get_usedBytes() - startIndex;

              for(s32 i=0; i<Embedded::SERIALIZED_BUFFER_HEADER_LENGTH; i++) {
                Anki::Cozmo::HAL::UARTPutChar(Embedded::SERIALIZED_BUFFER_HEADER[i]);
              }

              for(s32 i=0; i<validUsedBytes; i++) {
                Anki::Cozmo::HAL::UARTPutChar(bufferStart[i]);
              }

              for(s32 i=0; i<Embedded::SERIALIZED_BUFFER_FOOTER_LENGTH; i++) {
                Anki::Cozmo::HAL::UARTPutChar(Embedded::SERIALIZED_BUFFER_FOOTER[i]);
              }

              HAL::MicroWait(50000);
            } // ReturnCode SendDebugStream_Detection()
#endif

            if(markers.get_size() == 0) {
              //PRINT("No markers detected\n");
              return EXIT_SUCCESS;
            } else {
              //markers.Print("markers");
            }

            for(s32 i_marker = 0; i_marker < markers.get_size(); ++i_marker)
            {
              const Embedded::VisionMarker crntMarker = markers[i_marker];

              // TODO: convert corners from shorts (fixed point?) to floats
              /*
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
              */

              /*Messages::VisionMarker msg;
              msg.timestamp = frame.timestamp;
              msg.x_imgUpperLeft  = static_cast<f32>(crntMarker.corners[0].x);
              msg.y_imgUpperLeft  = static_cast<f32>(crntMarker.corners[0].y);
              msg.x_imgLowerLeft  = static_cast<f32>(crntMarker.corners[1].x);
              msg.y_imgLowerLeft  = static_cast<f32>(crntMarker.corners[1].y);
              msg.x_imgUpperRight = static_cast<f32>(crntMarker.corners[2].x);
              msg.y_imgUpperRight = static_cast<f32>(crntMarker.corners[2].y);
              msg.x_imgLowerRight = static_cast<f32>(crntMarker.corners[3].x);
              msg.y_imgLowerRight = static_cast<f32>(crntMarker.corners[3].y);
              msg.markerType = static_cast<u16>(crntMarker.markerType);

              Messages::ProcessVisionMarkerMessage(msg);*/

#ifdef RUN_SIMPLE_TRACKING_TEST
              // We're looking for the battery marker. If it is seen, start tracking it.
              if(!isTrackingMarkerFound_) {
                if(crntMarker.markerType == Vision::MARKER_BATTERIES) {
                  trackingMarker_ = crntMarker.markerType;
                  SetMarkerToTrack(trackingMarker_);
                  isTrackingMarkerFound_ = true;
                  isTemplateInitialized_ = false;
                }
              }
#endif

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
              if(isTrackingMarkerFound_ && not isTemplateInitialized_)
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

                //InitializeScratchBuffers(true);

                if(InitTemplate(frame, trackingQuad_) == EXIT_SUCCESS)
                {
                  AnkiAssert(isTemplateInitialized_ == true);
                  SetTrackingMode(isTemplateInitialized_);
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
      } // LookForMarkers()

      ReturnCode InitTemplate(const FrameBuffer &frame,
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
        if(InitializeScratchBuffers(false) != EXIT_SUCCESS) {
          return EXIT_FAILURE;
        }

        {
          PUSH_MEMORY_STACK(offchipScratch_);
          //PUSH_MEMORY_STACK(onchipScratch_);
          PUSH_MEMORY_STACK(ccmScratch_);

          using namespace Embedded::TemplateTracker;

          const u16 detectWidth  = HAL::CameraModeInfo[DETECTION_RESOLUTION].width;
          const u16 detectHeight = HAL::CameraModeInfo[DETECTION_RESOLUTION].height;

          // NOTE: this image will sit in the MemoryStack until tracking fails
          //       and we reconstruct the MemoryStack
          Embedded::Array<u8> imageLarge(detectHeight, detectWidth, offchipScratch_);
          YUVToGrayscaleHelper(frame, imageLarge);

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
          // TODO: At some point template initialization should happen at full detection resolution
          //       but for now, we have to downsample to tracking resolution
          const u16 trackHeight = HAL::CameraModeInfo[TRACKING_RESOLUTION].height;
          const u16 trackWidth  = HAL::CameraModeInfo[TRACKING_RESOLUTION].width;

          Embedded::Array<u8> imageSmall(trackHeight, trackWidth, ccmScratch_);
          DownsampleHelper(imageLarge, imageSmall, ccmScratch_);

          // Note that the templateRegion and the trackingQuad are both at
          // DETECTION_RESOLUTION, not necessarily the resolution of the frame.
          const s32 downsamplePower = static_cast<s32>(HAL::CameraModeInfo[TRACKING_RESOLUTION].downsamplePower[DETECTION_RESOLUTION]);
          const f32 downsampleFactor = static_cast<f32>(1 << downsamplePower);

          for(u8 i=0; i<4; ++i) {
            templateQuad[i].x /= downsampleFactor;
            templateQuad[i].y /= downsampleFactor;
          }
#endif // #if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
          tracker_ = LucasKanadeTrackerFast(imageSmall, templateQuad,
            NUM_TRACKING_PYRAMID_LEVELS,
            Embedded::Transformations::TRANSFORM_AFFINE,
            TRACKING_RIDGE_WEIGHT,
            onchipScratch_);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
          tracker_ = LucasKanadeTracker_f32(imageSmall, templateQuad,
            NUM_TRACKING_PYRAMID_LEVELS,
            Embedded::Transformations::TRANSFORM_AFFINE,
            TRACKING_RIDGE_WEIGHT,
            onchipScratch_);
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
          tracker_ =  BinaryTracker(imageLarge, templateQuad,
            edgeDetection_grayvalueThreshold, edgeDetection_minComponentWidth, edgeDetection_maxDetectionsPerType, edgeDetection_everyNLines,
            onchipScratch_);
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

        if(offchipScratch_.IsValid() && onchipScratch_.IsValid() && ccmScratch_.IsValid())
        {
          // So that we don't leak memory allocating image, etc, below,
          // we want to ensure that memory gets popped when this scope ends
          PUSH_MEMORY_STACK(offchipScratch_);
          PUSH_MEMORY_STACK(onchipScratch_);
          PUSH_MEMORY_STACK(ccmScratch_);

          //PRINT("TrackerScratch memory usage = %d of %d\n", trackerScratch_.get_usedBytes(), trackerScratch_.get_totalBytes());

          using namespace Embedded::TemplateTracker;

          AnkiAssert(tracker_.IsValid());

          const u16 detectWidth  = HAL::CameraModeInfo[DETECTION_RESOLUTION].width;
          const u16 detectHeight = HAL::CameraModeInfo[DETECTION_RESOLUTION].height;

          // NOTE: this image will sit in the MemoryStack until tracking fails
          //       and we reconstruct the MemoryStack

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
          // TODO: At some point template initialization should happen at full detection resolution
          //       but for now, we have to downsample to tracking resolution
          const u16 trackHeight = HAL::CameraModeInfo[TRACKING_RESOLUTION].height;
          const u16 trackWidth  = HAL::CameraModeInfo[TRACKING_RESOLUTION].width;

          Embedded::Array<u8> imageLarge(detectHeight, detectWidth, offchipScratch_);
          Embedded::Array<u8> imageSmall(trackHeight, trackWidth, ccmScratch_);
          DownsampleHelper(imageLarge, imageSmall, ccmScratch_);
          YUVToGrayscaleHelper(frame, imageLarge);
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
          Embedded::Array<u8> imageLarge(detectHeight, detectWidth, onchipScratch_);
          YUVToGrayscaleHelper(frame, imageLarge);
#endif

          Messages::DockingErrorSignal dockErrMsg;
          dockErrMsg.timestamp = frame.timestamp;

          bool converged = false;

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
          const Embedded::Result trackerResult = tracker_.UpdateTrack(imageSmall, TRACKING_MAX_ITERATIONS,
            TRACKING_CONVERGENCE_TOLERANCE,
            converged,
            onchipScratch_);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
          const Embedded::Result trackerResult = tracker_.UpdateTrack(imageSmall, TRACKING_MAX_ITERATIONS,
            TRACKING_CONVERGENCE_TOLERANCE,
            TRACKING_USE_WEIGHTS,
            converged,
            onchipScratch_);
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
          s32 numMatches = -1;

          const Embedded::Result trackerResult = tracker_.UpdateTrack(imageLarge,
            edgeDetection_grayvalueThreshold, edgeDetection_minComponentWidth, edgeDetection_maxDetectionsPerType, edgeDetection_everyNLines,
            matching_maxTranslationDistance, matching_maxProjectiveDistance,
            verification_maxTranslationDistance,
            false,
            numMatches,
            ccmScratch_, offchipScratch_);

          const s32 numTemplatePixels = tracker_.get_numTemplatePixels();

          const f32 percentMatchedPixels = static_cast<f32>(numMatches) / static_cast<f32>(numTemplatePixels);

          if(percentMatchedPixels >= percentMatchedPixelsThreshold) {
            converged = true;
          }

          //tracker_.get_transformation().Print("tracker_");
          //printf("%f %f\n", *tracker_.get_transformation().get_homography().Pointer(0,2), *tracker_.get_transformation().get_homography().Pointer(1,2));

          //PRINT("percentMatchedPixels = %f\n", percentMatchedPixels);
#endif

          if(trackerResult == Embedded::RESULT_OK)
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
                29.5f, // TODO: Get this from the docking command message from basestation
                headCamInfo_->focalLength_x / fxAdj,
                dockErrMsg.x_distErr,
                dockErrMsg.y_horErr,
                dockErrMsg.angleErr,
                onchipScratch_);

#ifdef SEND_DEBUG_STREAM
              {
                // Pushing here at the top is necessary, because of all the global variables
                PUSH_MEMORY_STACK(offchipScratch_);
                PUSH_MEMORY_STACK(ccmScratch_);

                debugStreamBuffer_ = Embedded::SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);

                // Stream the images as they come
                //const s32 imageHeight = HAL::CameraModeInfo[HAL::CAMERA_MODE_QVGA].height;
                //const s32 imageWidth = HAL::CameraModeInfo[HAL::CAMERA_MODE_QVGA].width;
                Embedded::Array<u8> imageLarge(240, 320, offchipScratch_);
                Embedded::Array<u8> imageSmall(60, 80, offchipScratch_);

                YUVToGrayscaleHelper(frame, imageLarge);
                DownsampleHelper(imageLarge, imageSmall, ccmScratch_);

                debugStreamBuffer_.PushBack(imageSmall);

                {
                  PUSH_MEMORY_STACK(offchipScratch_);

                  // TODO: get the correct number
                  //const s32 oneTransformationLength = sizeof(Embedded::Transformations::PlanarTransformation_f32);
                  const s32 oneTransformationLength = 512;
                  void * restrict oneTransformation = offchipScratch_.Allocate(oneTransformationLength);
                  
                  const Embedded::Transformations::PlanarTransformation_f32 transformation = tracker_.get_transformation();

                  transformation.Serialize(oneTransformation, oneTransformationLength);
                  debugStreamBuffer_.PushBack("PlanarTransformation_f32", oneTransformation, oneTransformationLength);
                }

                s32 startIndex;
                const u8 * bufferStart = reinterpret_cast<const u8*>(debugStreamBuffer_.get_memoryStack().get_validBufferStart(startIndex));
                const s32 validUsedBytes = debugStreamBuffer_.get_memoryStack().get_usedBytes() - startIndex;
                
                for(s32 i=0; i<Embedded::SERIALIZED_BUFFER_HEADER_LENGTH; i++) {
                  Anki::Cozmo::HAL::UARTPutChar(Embedded::SERIALIZED_BUFFER_HEADER[i]);
                }

                for(s32 i=0; i<validUsedBytes; i++) {
                  Anki::Cozmo::HAL::UARTPutChar(bufferStart[i]);
                }

                for(s32 i=0; i<Embedded::SERIALIZED_BUFFER_FOOTER_LENGTH; i++) {
                  Anki::Cozmo::HAL::UARTPutChar(Embedded::SERIALIZED_BUFFER_FOOTER[i]);
                }

                HAL::MicroWait(50000);
              }
#endif
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
