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

using namespace Anki::Embedded;

#if defined(SIMULATOR) && ANKICORETECH_EMBEDDED_USE_MATLAB && (!defined(USE_OFFBOARD_VISION) || !USE_OFFBOARD_VISION)
#define USE_MATLAB_VISUALIZATION 1
#else
#define USE_MATLAB_VISUALIZATION 0
#endif

#if USE_MATLAB_VISUALIZATION
#include "anki/common/robot/matlabInterface.h"
#endif

// m_buffer1 (aka Mr. Bufferly) is where the camera image is currently stored
// TODO: make nice
#define M_BUFFER1_SIZE (320*240*2)
extern u8 m_buffer1[];
extern volatile bool isEOF;

static bool isInitialized_ = false;

// TODO: remove
#define SEND_DEBUG_STREAM
#define RUN_SIMPLE_TRACKING_TEST

#define DOCKING_LUCAS_KANADE_STANDARD 1 //< LucasKanadeTracker_f32
#define DOCKING_LUCAS_KANADE_FAST     2 //< LucasKanadeTrackerFast
#define DOCKING_BINARY_TRACKER        3 //< BinaryTracker
#define DOCKING_ALGORITHM DOCKING_BINARY_TRACKER

typedef enum {
  VISION_MODE_IDLE,
  VISION_MODE_LOOKING_FOR_MARKERS,
  VISION_MODE_TRACKING
} VisionSystemMode;

namespace DetectFiducialMarkersParameters
{
  static s32 detectWidth;
  static s32 detectHeight;
  static s32 scaleImage_thresholdMultiplier;
  static s32 scaleImage_numPyramidLevels;
  static s32 component1d_minComponentWidth;
  static s32 component1d_maxSkipDistance;
  static f32 minSideLength;
  static f32 maxSideLength;
  static s32 component_minimumNumPixels;
  static s32 component_maximumNumPixels;
  static s32 component_sparseMultiplyThreshold;
  static s32 component_solidMultiplyThreshold;
  static f32 component_minHollowRatio;
  static s32 maxExtractedQuads;
  static s32 quads_minQuadArea;
  static s32 quads_quadSymmetryThreshold;
  static s32 quads_minDistanceFromImageEdge;
  static f32 decode_minContrastRatio;
  static s32 maxConnectedComponentSegments;
  static s32 maxMarkers;

  static ReturnCode Initialize()
  {
    detectWidth  = HAL::CameraModeInfo[DETECTION_RESOLUTION].width;
    detectHeight = HAL::CameraModeInfo[DETECTION_RESOLUTION].height;

    scaleImage_thresholdMultiplier = 65536; // 1.0*(2^16)=65536
    scaleImage_numPyramidLevels = 3;

    component1d_minComponentWidth = 0;
    component1d_maxSkipDistance = 0;

    minSideLength = 0.03f*static_cast<f32>(MAX(detectWidth,detectHeight));
    maxSideLength = 0.97f*static_cast<f32>(MIN(detectWidth,detectHeight));

    component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
    component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
    component_sparseMultiplyThreshold = 1000 << 5;
    component_solidMultiplyThreshold = 2 << 5;

    component_minHollowRatio = 1.0f;

    maxExtractedQuads = 1000/2;
    quads_minQuadArea = 100/4;
    quads_quadSymmetryThreshold = 512; // ANS: corresponds to 2.0, loosened from 384 (1.5), for large mat markers at extreme perspective distortion
    quads_minDistanceFromImageEdge = 2;

    decode_minContrastRatio = 1.25;

    maxConnectedComponentSegments = 39000; // 322*240/2 = 38640

    maxMarkers = 100;

    return EXIT_SUCCESS;
  }
} // namespace DetectFiducialMarkersParameters

namespace LucasKanadeParameters {
  static s32 numPyramidLevels;
  static f32 ridgeWeight;
  static s32 maxIterations;
  static f32 convergenceTolerance;
  static bool useWeights;

  // Set the default parameters
  ReturnCode Initialize()
  {
    numPyramidLevels = 2;
    ridgeWeight = 0.0f;
    maxIterations = 25;
    convergenceTolerance = 0.05f;
    useWeights = true;

    return EXIT_SUCCESS;
  }
} // namespace LucasKanadeParameters

namespace BinaryTrackerParameters {
  static u8 edgeDetection_grayvalueThreshold;
  static s32 edgeDetection_minComponentWidth;
  static s32 edgeDetection_maxDetectionsPerType;
  static s32 edgeDetection_everyNLines;
  static s32 matching_maxTranslationDistance;
  static s32 matching_maxProjectiveDistance;
  static s32 verification_maxTranslationDistance;
  static f32 percentMatchedPixelsThreshold;

  // Set the default parameters
  ReturnCode Initialize()
  {
    edgeDetection_grayvalueThreshold = 128;
    edgeDetection_minComponentWidth = 2;
    edgeDetection_maxDetectionsPerType = 2500;
    edgeDetection_everyNLines = 1;
    matching_maxTranslationDistance = 7;
    matching_maxProjectiveDistance = 7;
    verification_maxTranslationDistance = 1;
    percentMatchedPixelsThreshold = 0.5f; // TODO: pick a reasonable value

    return EXIT_SUCCESS;
  }
} // namespace BinaryTrackerParameters

namespace VisionMemory
{
  static const s32 OFFCHIP_BUFFER_SIZE = 2000000;
  static const s32 ONCHIP_BUFFER_SIZE = 170000; // The max here is somewhere between 175000 and 180000 bytes
  static const s32 CCM_BUFFER_SIZE = 50000; // The max here is probably 65536 (0x10000) bytes

  static OFFCHIP char offchipBuffer[OFFCHIP_BUFFER_SIZE];
  static ONCHIP char onchipBuffer[ONCHIP_BUFFER_SIZE];
  static CCM char ccmBuffer[CCM_BUFFER_SIZE];

  static MemoryStack offchipScratch_;
  static MemoryStack onchipScratch_;
  static MemoryStack ccmScratch_;

  ReturnCode Initialize()
  {
    // TODO: add the rest

    return EXIT_SUCCESS;
  }
} // namespace VisionMemory

namespace DebugStream
{
  static const s32 PRINTF_BUFFER_SIZE = 10000;
  static const s32 DEBUG_STREAM_BUFFER_SIZE = 2000000;

  static OFFCHIP u8 printfBufferRaw_[PRINTF_BUFFER_SIZE];
  static OFFCHIP u8 debugStreamBufferRaw_[DEBUG_STREAM_BUFFER_SIZE];

  static SerializedBuffer printfBuffer_;
  static SerializedBuffer debugStreamBuffer_;

#ifdef !SEND_DEBUG_STREAM
  static ReturnCode SendFiducialDetection() { return EXIT_SUCCESS; }
  static ReturnCode SendTrackingUpdate() { return EXIT_SUCCESS; }
  static ReturnCode SendPrintf(const char * string) { return EXIT_SUCCESS; }
#else
  static ReturnCode SendBuffer(const SerializedBuffer &toSend)
  {
    s32 startIndex;
    const u8 * bufferStart = reinterpret_cast<const u8*>(toSend.get_memoryStack().get_validBufferStart(startIndex));
    const s32 validUsedBytes = toSend.get_memoryStack().get_usedBytes() - startIndex;

    for(s32 i=0; i<SERIALIZED_BUFFER_HEADER_LENGTH; i++) {
      Anki::Cozmo::HAL::USBPutChar(SERIALIZED_BUFFER_HEADER[i]);
    }

    HAL::USBSendBuffer(bufferStart, validUsedBytes);

    for(s32 i=0; i<SERIALIZED_BUFFER_FOOTER_LENGTH; i++) {
      Anki::Cozmo::HAL::USBPutChar(SERIALIZED_BUFFER_FOOTER[i]);
    }

    HAL::MicroWait(50000);
  }

  static ReturnCode SendPrintf(const char * string)
  {
    printfBuffer_ = SerializedBuffer(&printfBufferRaw_[0], PRINTF_BUFFER_SIZE);
    printfBuffer_.PushBackString(string);

    return SendBuffer(printfBuffer_);
  } // void SendPrintf(const char * string)

  static ReturnCode SendFiducialDetection()
  {
    // Pushing here at the top is necessary, because of all the global variables
    PUSH_MEMORY_STACK(offchipScratch_);
    PUSH_MEMORY_STACK(ccmScratch_);

    if(!isInitialized_) {
      Init();
    }

    debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);

    // Stream the images as they come
    //const s32 imageHeight = HAL::CameraModeInfo[HAL::CAMERA_MODE_QVGA].height;
    //const s32 imageWidth = HAL::CameraModeInfo[HAL::CAMERA_MODE_QVGA].width;
    Array<u8> imageLarge(240, 320, offchipScratch_);
    Array<u8> imageSmall(60, 80, offchipScratch_);

    YUVToGrayscaleHelper(frame, imageLarge);
    DownsampleHelper(imageLarge, imageSmall, ccmScratch_);

    debugStreamBuffer_.PushBack(imageSmall);

    if(markers.get_size() != 0) {
      PUSH_MEMORY_STACK(offchipScratch_);

      const s32 numMarkers = markers.get_size();
      const VisionMarker * pMarkers = markers.Pointer(0);

      void * restrict oneMarker = offchipScratch_.Allocate(sizeof(VisionMarker));
      const s32 oneMarkerLength = sizeof(VisionMarker);

      for(s32 i=0; i<numMarkers; i++) {
        pMarkers[i].Serialize(oneMarker, oneMarkerLength);
        debugStreamBuffer_.PushBack("VisionMarker", oneMarker, oneMarkerLength);
      }
    }

    return SendBuffer(debugStreamBuffer_);
  } // ReturnCode SendDebugStream_Detection()

  static ReturnCode SendTrackingUpdate()
  {
    // Pushing here at the top is necessary, because of all the global variables
    PUSH_MEMORY_STACK(offchipScratch_);
    PUSH_MEMORY_STACK(ccmScratch_);

    debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);

    // Stream the images as they come
    //const s32 imageHeight = HAL::CameraModeInfo[HAL::CAMERA_MODE_QVGA].height;
    //const s32 imageWidth = HAL::CameraModeInfo[HAL::CAMERA_MODE_QVGA].width;
    Array<u8> imageLarge(240, 320, offchipScratch_);
    Array<u8> imageSmall(60, 80, offchipScratch_);

    YUVToGrayscaleHelper(frame, imageLarge);
    DownsampleHelper(imageLarge, imageSmall, ccmScratch_);

    debugStreamBuffer_.PushBack(imageSmall);

    {
      PUSH_MEMORY_STACK(offchipScratch_);

      // TODO: get the correct number
      //const s32 oneTransformationLength = sizeof(Transformations::PlanarTransformation_f32);
      const s32 oneTransformationLength = 512;
      void * restrict oneTransformation = offchipScratch_.Allocate(oneTransformationLength);

      const Transformations::PlanarTransformation_f32 transformation = tracker_.get_transformation();

      transformation.Serialize(oneTransformation, oneTransformationLength);
      debugStreamBuffer_.PushBack("PlanarTransformation_f32", oneTransformation, oneTransformationLength);
    }

    return SendBuffer(debugStreamBuffer_);
  }
#endif // #ifdef SEND_DEBUG_STREAM

  ReturnCode Initialize()
  {
    // TODO: add the rest
    debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
    printfBuffer_ = SerializedBuffer(&printfBufferRaw_[0], PRINTF_BUFFER_SIZE);

    return EXIT_SUCCESS;
  }
} // namespace DebugStream

namespace SimulatorParameters {
  static const u32 LOOK_FOR_BLOCK_PERIOD_US = 200000;
  static const u32 TRACK_BLOCK_PERIOD_US = 100000;

  static u32 frameRdyTimeUS_;

  ReturnCode Initialize()
  {
    frameRdyTimeUS_ = 0;

    return EXIT_SUCCESS;
  }
} // namespace SimulatorParameters

namespace MatlabVisualization
{
#if !USE_MATLAB_VISUALIZATION
  static ReturnCode Initialize() { return EXIT_SUCCESS; }
  static ReturnCode ResetFiducialDetection() { return EXIT_SUCCESS; }
  static ReturnCode SendFiducialDetection(const Quadrilateral<s16> &corners) { return EXIT_SUCCESS; }
  static ReturnCode SendDrawNow() { return EXIT_SUCCESS; }
  static ReturnCode SendTrack()  { return EXIT_SUCCESS; }
  static ReturnCode SendTrack() { return EXIT_SUCCESS; }
#else
  static Matlab matlabViz_;

  static ReturnCode Initialize()
  {
    matlabViz_ = Matlab(false);
    matlabViz_.EvalStringEcho("h_fig  = figure('Name', 'VisionSystem'); "
      "h_axes = axes('Pos', [.1 .1 .8 .8], 'Parent', h_fig); "
      "h_img  = imagesc(0, 'Parent', h_axes); "
      "axis(h_axes, 'image', 'off'); "
      "hold(h_axes, 'on'); "
      "colormap(h_fig, gray); "
      "h_trackedQuad = plot(nan, nan, 'b', 'LineWidth', 2, "
      "                     'Parent', h_axes); "
      "imageCtr = 0;");

    return EXIT_SUCCESS;
  }

  static ReturnCode ResetFiducialDetection()
  {
    matlabViz_.EvalStringEcho("delete(findobj(h_axes, 'Tag', 'DetectedQuad'));");
    matlabViz_.PutArray(image, "detectionImage");
    matlabViz_.EvalStringEcho("set(h_img, 'CData', detectionImage); "
      "set(h_axes, 'XLim', [.5 size(detectionImage,2)+.5], "
      "            'YLim', [.5 size(detectionImage,1)+.5]);");

    return EXIT_SUCCESS;
  }

  static ReturnCode SendFiducialDetection(const Quadrilateral<s16> &corners)
  {
    matlabViz_.PutQuad(crntMarker.corners, "detectedQuad");
    matlabViz_.EvalStringEcho("plot(detectedQuad([1 2 4 3 1],1)+1, "
      "     detectedQuad([1 2 4 3 1],2)+1, "
      "     'r', 'LineWidth', 2, "
      "     'Parent', h_axes, "
      "     'Tag', 'DetectedQuad');");

    return EXIT_SUCCESS;
  }

  static ReturnCode SendDrawNow()
  {
    matlabViz_.EvalString("drawnow");

    return EXIT_SUCCESS;
  }

  static ReturnCode SendTrack()
  {
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

    return EXIT_SUCCESS;
  }

#endif //#if USE_MATLAB_VISUALIZATION
} // namespace MatlabVisualization

namespace VisionState {
  static const s32 MAX_TRACKING_FAILURES = 5;

  static VisionSystemMode mode_ = VISION_MODE_LOOKING_FOR_MARKERS;
  static const Anki::Cozmo::HAL::CameraInfo* headCamInfo_;
  static Anki::Vision::MarkerType trackingMarker_;
  static s32 numTrackFailures_ ; //< The tracker can fail to converge this many times before we give up and reset the docker
  static Quadrilateral<f32> trackingQuad_;

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
  static TemplateTracker::LucasKanadeTrackerFast tracker_;
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
  static TemplateTracker::LucasKanadeTracker_f32 tracker_;
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
  static TemplateTracker::BinaryTracker tracker_;
#endif

  // TODO: what do these do?
  //bool isTrackingMarkerFound_ = false;
  //bool isTemplateInitialized_ = false;

  //// Capture an entire frame using HAL commands and put it in the given frame buffer
  //typedef struct {
  //  u8* data;
  //  HAL::CameraMode resolution;
  //  TimeStamp_t  timestamp;
  //} FrameBuffer;

  static ReturnCode SendOffboardInitialization()
  {
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
    return EXIT_SUCCESS;
  }

  ReturnCode Initialize()
  {
    mode_ = VISION_MODE_LOOKING_FOR_MARKERS;
    headCamInfo_ = NULL;
    trackingMarker_ = Anki::Vision::MARKER_UNKNOWN;
    numTrackFailures_ = 0;

#if 0
    //headCamInfo_ = HAL::GetHeadCamInfo();
    //if(headCamInfo_ == NULL) {
    //  PRINT("VisionSystem::Init() - HeadCam Info pointer is NULL!\n");
    //  return EXIT_FAILURE;
    //}

    //// Compute the resolution of the mat camera from its FOV and height off the mat:
    //f32 matCamHeightInPix = ((static_cast<f32>(matCamInfo_->nrows)*.5f) /
    //  tanf(matCamInfo_->fov_ver * .5f));
    //matCamPixPerMM_ = matCamHeightInPix / MAT_CAM_HEIGHT_FROM_GROUND_MM;
#endif //#if 0

    SendOffboardInitialization();

    return EXIT_SUCCESS;
  }
} // namespace VisionState

static void Initialize()
{
  if(!isInitialized_) {
    // WARNING: the order of these initializations matter!
    // TODO: Figure out the intertwinedness
    // TODO: add error handling
    VisionState::Initialize();
    VisionMemory::Initialize();

    DebugStream::Initialize();
    DetectFiducialMarkersParameters::Initialize();
    LucasKanadeParameters::Initialize();
    BinaryTrackerParameters::Initialize();
    SimulatorParameters::Initialize();
    MatlabVisualization::Initialize();

    isInitialized_ = true;
  }
}

////ReturnCode CaptureHeadFrame(FrameBuffer &frame);
//
//ReturnCode LookForMarkers(const FrameBuffer &frame);
//
//ReturnCode InitTemplate(const FrameBuffer &frame,
//                        Quadrilateral<f32>& templateRegion);
//
//ReturnCode TrackTemplate(const FrameBuffer &frame);
//
////ReturnCode GetRelativeOdometry(const FrameBuffer &frame);
//
//void DownsampleHelper(const FrameBuffer& frame, Array<u8>& image,
//                      const HAL::CameraMode outputResolution,
//                      MemoryStack scratch);
//
//void DownsampleHelper(const Array<u8>& in,
//                      Array<u8>& out,
//                      MemoryStack scratch);
//
//ReturnCode YUVToGrayscaleHelper(const FrameBuffer& yuvFrame, Array<u8>& grayscaleImage);
//
//// Warning, has side effects on local buffers
//#warning forceReset is a big hack. This file should be fixed in a principled way.
//ReturnCode InitializeScratchBuffers(bool forceReset);

namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
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
        default:
          PRINT("VisionSystem::Update(): reached default case in switch statement.");
          retVal = EXIT_FAILURE;
          break;
        } // SWITCH(mode_)

        return retVal;
      } // Update()

      void DownsampleHelper(const FrameBuffer& frame, Array<u8>& image,
        const HAL::CameraMode outputResolution,
        MemoryStack scratch)
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
            Flags::Buffer(false,false,false));

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

      void DownsampleHelper(const Array<u8>& in,
        Array<u8>& out,
        MemoryStack scratch)
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

      ReturnCode YUVToGrayscaleHelper(const FrameBuffer& yuvFrame, Array<u8>& grayscaleImage)
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
      } // void YUVToGrayscaleHelper(const FrameBuffer& yuvFrame, Array<u8>& grayscaleImage)

      ReturnCode InitializeScratchBuffers(bool forceReset)
      {
        if(forceReset || !scratchInitialized_) {
          //PRINT("Initializing scratch memory.\n");

          offchipScratch_ = MemoryStack(offchipBuffer, OFFCHIP_BUFFER_SIZE);
          onchipScratch_ = MemoryStack(onchipBuffer, ONCHIP_BUFFER_SIZE);
          ccmScratch_ = MemoryStack(ccmBuffer, CCM_BUFFER_SIZE);

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

          Array<u8> image(detectHeight, detectWidth, offchipScratch_);
          YUVToGrayscaleHelper(frame, image);

          FixedLengthList<VisionMarker> markers(maxMarkers, offchipScratch_);
          FixedLengthList<Array<f32> > homographies(maxMarkers, ccmScratch_);

          markers.set_size(maxMarkers);
          homographies.set_size(maxMarkers);

          for(s32 i=0; i<maxMarkers; i++) {
            Array<f32> newArray(3, 3, ccmScratch_);
            homographies[i] = newArray;
          } // for(s32 i=0; i<maximumSize; i++)

          MatlabVisualization::ResetFiducialDetection();

          InitBenchmarking();

          const Result result = DetectFiducialMarkers(
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

          if(result == RESULT_OK) {
            SendFiducialDetection();

            for(s32 i_marker = 0; i_marker < markers.get_size(); ++i_marker)
            {
              const VisionMarker crntMarker = markers[i_marker];

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
              MatlabVisualization::SendFiducialDetection(crntMarker.corners);

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

            MatlabVisualization::SendDrawNow();

            retVal = EXIT_SUCCESS;
          } // IF SimpleDetector_Steps12345_lowMemory() == RESULT_OK
        } // IF detectorScratch1 and 2 are valid

#endif // USE_OFFBOARD_VISION

        return retVal;
      } // LookForMarkers()

      ReturnCode InitTemplate(const FrameBuffer &frame,
        Quadrilateral<f32> &templateQuad)
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

          using namespace TemplateTracker;

          const u16 detectWidth  = HAL::CameraModeInfo[DETECTION_RESOLUTION].width;
          const u16 detectHeight = HAL::CameraModeInfo[DETECTION_RESOLUTION].height;

          // NOTE: this image will sit in the MemoryStack until tracking fails
          //       and we reconstruct the MemoryStack
          Array<u8> imageLarge(detectHeight, detectWidth, offchipScratch_);
          YUVToGrayscaleHelper(frame, imageLarge);

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
          // TODO: At some point template initialization should happen at full detection resolution
          //       but for now, we have to downsample to tracking resolution
          const u16 trackHeight = HAL::CameraModeInfo[TRACKING_RESOLUTION].height;
          const u16 trackWidth  = HAL::CameraModeInfo[TRACKING_RESOLUTION].width;

          Array<u8> imageSmall(trackHeight, trackWidth, ccmScratch_);
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
            Transformations::TRANSFORM_AFFINE,
            TRACKING_RIDGE_WEIGHT,
            onchipScratch_);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
          tracker_ = LucasKanadeTracker_f32(imageSmall, templateQuad,
            NUM_TRACKING_PYRAMID_LEVELS,
            Transformations::TRANSFORM_AFFINE,
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

          using namespace TemplateTracker;

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

          Array<u8> imageLarge(detectHeight, detectWidth, offchipScratch_);
          Array<u8> imageSmall(trackHeight, trackWidth, ccmScratch_);
          DownsampleHelper(imageLarge, imageSmall, ccmScratch_);
          YUVToGrayscaleHelper(frame, imageLarge);
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
          Array<u8> imageLarge(detectHeight, detectWidth, onchipScratch_);
          YUVToGrayscaleHelper(frame, imageLarge);
#endif

          Messages::DockingErrorSignal dockErrMsg;
          dockErrMsg.timestamp = frame.timestamp;

          bool converged = false;

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
          const Result trackerResult = tracker_.UpdateTrack(imageSmall, TRACKING_MAX_ITERATIONS,
            TRACKING_CONVERGENCE_TOLERANCE,
            converged,
            onchipScratch_);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
          const Result trackerResult = tracker_.UpdateTrack(imageSmall, TRACKING_MAX_ITERATIONS,
            TRACKING_CONVERGENCE_TOLERANCE,
            TRACKING_USE_WEIGHTS,
            converged,
            onchipScratch_);
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
          s32 numMatches = -1;

          const Result trackerResult = tracker_.UpdateTrack(imageLarge,
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

          if(trackerResult == RESULT_OK)
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

              SendTrackingUpdate();
            } // IF converged

            MatlabVisualization::SendTrack()

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
