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

using namespace Anki;
using namespace Anki::Embedded;
using namespace Anki::Cozmo;

//#if defined(SIMULATOR) && ANKICORETECH_EMBEDDED_USE_MATLAB && (!defined(USE_OFFBOARD_VISION) || !USE_OFFBOARD_VISION)
#if defined(SIMULATOR) && ANKICORETECH_EMBEDDED_USE_MATLAB
#define USE_MATLAB_VISUALIZATION 1
#else
#define USE_MATLAB_VISUALIZATION 0
#endif

#if USE_MATLAB_VISUALIZATION
#include "anki/common/robot/matlabInterface.h"
#endif

// m_buffer1 (aka Mr. Bufferly) is where the camera image is currently stored
// TODO: make nice
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

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
typedef TemplateTracker::LucasKanadeTrackerFast Tracker;
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
typedef TemplateTracker::LucasKanadeTracker_f32 Tracker;
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
typedef TemplateTracker::BinaryTracker Tracker;
#endif

typedef enum {
  VISION_MODE_IDLE,
  VISION_MODE_LOOKING_FOR_MARKERS,
  VISION_MODE_TRACKING
} VisionSystemMode;

namespace DetectFiducialMarkersParameters
{
  typedef struct Parameters
  {
    s32 detectionWidth;
    s32 detectionHeight;
    s32 scaleImage_thresholdMultiplier;
    s32 scaleImage_numPyramidLevels;
    s32 component1d_minComponentWidth;
    s32 component1d_maxSkipDistance;
    f32 minSideLength;
    f32 maxSideLength;
    s32 component_minimumNumPixels;
    s32 component_maximumNumPixels;
    s32 component_sparseMultiplyThreshold;
    s32 component_solidMultiplyThreshold;
    f32 component_minHollowRatio;
    s32 maxExtractedQuads;
    s32 quads_minQuadArea;
    s32 quads_quadSymmetryThreshold;
    s32 quads_minDistanceFromImageEdge;
    f32 decode_minContrastRatio;
    s32 maxConnectedComponentSegments;
    s32 maxMarkers;

    Parameters()
    {
      // TODO: get from HAL
      detectionWidth  = 320;
      detectionHeight = 240;

      scaleImage_thresholdMultiplier = 65536; // 1.0*(2^16)=65536
      scaleImage_numPyramidLevels = 3;

      component1d_minComponentWidth = 0;
      component1d_maxSkipDistance = 0;

      minSideLength = 0.03f*static_cast<f32>(MAX(detectionWidth,detectionHeight));
      maxSideLength = 0.97f*static_cast<f32>(MIN(detectionWidth,detectionHeight));

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
    } // Parameters
  } Parameters;

  static DetectFiducialMarkersParameters::Parameters parameters_;

  static ReturnCode Initialize()
  {
    parameters_ = DetectFiducialMarkersParameters::Parameters();

    return EXIT_SUCCESS;
  }
} // namespace DetectFiducialMarkersParameters

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST

namespace TrackerParameters {
  typedef struct Parameters
  {
    s32 detectionWidth;
    s32 detectionHeight;
    s32 trackingImageHeight;
    s32 trackingImageWidth;
    s32 numPyramidLevels;
    f32 ridgeWeight;
    s32 maxIterations;
    f32 convergenceTolerance;
    bool useWeights;

    Parameters()
    {
      // TODO: set via HAL
      detectionWidth  = 320;
      detectionHeight = 240;
      trackingImageHeight = 60;
      trackingImageWidth = 80;

      numPyramidLevels = 2;
      ridgeWeight = 0.0f;
      maxIterations = 25;
      convergenceTolerance = 0.05f;
      useWeights = true;
    } // Parameters
  } Parameters;

  static TrackerParameters::Parameters parameters_;

  // Set the default parameters
  static ReturnCode Initialize()
  {
    parameters_ = TrackerParameters::Parameters();

    return EXIT_SUCCESS;
  }
} // namespace LucasKanadeParameters

#else // #if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST

namespace TrackerParameters {
  typedef struct Parameters
  {
    s32 detectionWidth;
    s32 detectionHeight;
    u8 edgeDetection_grayvalueThreshold;
    s32 edgeDetection_minComponentWidth;
    s32 edgeDetection_maxDetectionsPerType;
    s32 edgeDetection_everyNLines;
    s32 matching_maxTranslationDistance;
    s32 matching_maxProjectiveDistance;
    s32 verification_maxTranslationDistance;
    f32 percentMatchedPixelsThreshold;

    Parameters()
    {
      // TODO: set via HAL
      detectionWidth  = 320;
      detectionHeight = 240;
      edgeDetection_grayvalueThreshold = 128;
      edgeDetection_minComponentWidth = 2;
      edgeDetection_maxDetectionsPerType = 2500;
      edgeDetection_everyNLines = 1;
      matching_maxTranslationDistance = 7;
      matching_maxProjectiveDistance = 7;
      verification_maxTranslationDistance = 1;
      percentMatchedPixelsThreshold = 0.5f; // TODO: pick a reasonable value
    }
  } Parameters;

  static TrackerParameters::Parameters parameters_;

  // Set the default parameters
  static ReturnCode Initialize()
  {
    parameters_ = TrackerParameters::Parameters();

    return EXIT_SUCCESS;
  }
} // namespace TrackerParameters

#endif // #if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST

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

  // WARNING: ResetBuffers should be used with caution
  static ReturnCode ResetBuffers()
  {
    offchipScratch_ = MemoryStack(offchipBuffer, OFFCHIP_BUFFER_SIZE);
    onchipScratch_ = MemoryStack(onchipBuffer, ONCHIP_BUFFER_SIZE);
    ccmScratch_ = MemoryStack(ccmBuffer, CCM_BUFFER_SIZE);

    if(!offchipScratch_.IsValid() || !onchipScratch_.IsValid() || !ccmScratch_.IsValid()) {
      PRINT("Error: InitializeScratchBuffers\n");
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  }

  static ReturnCode Initialize()
  {
    return ResetBuffers();
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

#if !defined(SEND_DEBUG_STREAM)
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

    return EXIT_SUCCESS;
  }

  static ReturnCode SendPrintf(const char * string)
  {
    printfBuffer_ = SerializedBuffer(&printfBufferRaw_[0], PRINTF_BUFFER_SIZE);
    printfBuffer_.PushBackString(string);

    return SendBuffer(printfBuffer_);
  } // void SendPrintf(const char * string)

  static ReturnCode SendFiducialDetection()
  {
    //debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);

    //// Stream the images as they come
    ////const s32 imageHeight = HAL::CameraModeInfo[HAL::CAMERA_MODE_QVGA].height;
    ////const s32 imageWidth = HAL::CameraModeInfo[HAL::CAMERA_MODE_QVGA].width;
    //Array<u8> imageLarge(240, 320, offchipScratch_);
    //Array<u8> imageSmall(60, 80, offchipScratch_);

    //YUVToGrayscaleHelper(frame, imageLarge);
    //DownsampleHelper(imageLarge, imageSmall, ccmScratch_);

    //debugStreamBuffer_.PushBack(imageSmall);

    //if(markers.get_size() != 0) {
    //  PUSH_MEMORY_STACK(offchipScratch_);

    //  const s32 numMarkers = markers.get_size();
    //  const VisionMarker * pMarkers = markers.Pointer(0);

    //  void * restrict oneMarker = offchipScratch_.Allocate(sizeof(VisionMarker));
    //  const s32 oneMarkerLength = sizeof(VisionMarker);

    //  for(s32 i=0; i<numMarkers; i++) {
    //    pMarkers[i].Serialize(oneMarker, oneMarkerLength);
    //    debugStreamBuffer_.PushBack("VisionMarker", oneMarker, oneMarkerLength);
    //  }
    //}

    //return SendBuffer(debugStreamBuffer_);

    return EXIT_SUCCESS;
  } // ReturnCode SendDebugStream_Detection()

  static ReturnCode SendTrackingUpdate()
  {
    //// Pushing here at the top is necessary, because of all the global variables
    //PUSH_MEMORY_STACK(offchipScratch_);
    //PUSH_MEMORY_STACK(ccmScratch_);

    //debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);

    //// Stream the images as they come
    ////const s32 imageHeight = HAL::CameraModeInfo[HAL::CAMERA_MODE_QVGA].height;
    ////const s32 imageWidth = HAL::CameraModeInfo[HAL::CAMERA_MODE_QVGA].width;
    //Array<u8> imageLarge(240, 320, offchipScratch_);
    //Array<u8> imageSmall(60, 80, offchipScratch_);

    //YUVToGrayscaleHelper(frame, imageLarge);
    //DownsampleHelper(imageLarge, imageSmall, ccmScratch_);

    //debugStreamBuffer_.PushBack(imageSmall);

    //{
    //  PUSH_MEMORY_STACK(offchipScratch_);

    //  // TODO: get the correct number
    //  //const s32 oneTransformationLength = sizeof(Transformations::PlanarTransformation_f32);
    //  const s32 oneTransformationLength = 512;
    //  void * restrict oneTransformation = offchipScratch_.Allocate(oneTransformationLength);

    //  const Transformations::PlanarTransformation_f32 transformation = tracker_.get_transformation();

    //  transformation.Serialize(oneTransformation, oneTransformationLength);
    //  debugStreamBuffer_.PushBack("PlanarTransformation_f32", oneTransformation, oneTransformationLength);
    //}

    //return SendBuffer(debugStreamBuffer_);

    return EXIT_SUCCESS;
  }
#endif // #ifdef SEND_DEBUG_STREAM

  static ReturnCode Initialize()
  {
    // TODO: add the rest
    debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
    printfBuffer_ = SerializedBuffer(&printfBufferRaw_[0], PRINTF_BUFFER_SIZE);

    return EXIT_SUCCESS;
  }
} // namespace DebugStream

namespace SimulatorParameters {
#if !defined(SIMULATOR)
  static ReturnCode Initialize()
  {
    return EXIT_SUCCESS;
  }
#else
  static const u32 LOOK_FOR_BLOCK_PERIOD_US = 200000;
  static const u32 TRACK_BLOCK_PERIOD_US = 100000;

  static u32 frameRdyTimeUS_;

  static ReturnCode Initialize()
  {
    frameRdyTimeUS_ = 0;

    return EXIT_SUCCESS;
  }
#endif
} // namespace SimulatorParameters

namespace MatlabVisualization
{
#if !USE_MATLAB_VISUALIZATION
  static ReturnCode Initialize() { return EXIT_SUCCESS; }
  static ReturnCode ResetFiducialDetection() { return EXIT_SUCCESS; }
  static ReturnCode SendFiducialDetection() { return EXIT_SUCCESS; }
  static ReturnCode SendDrawNow() { return EXIT_SUCCESS; }
  static ReturnCode SendTrack()  { return EXIT_SUCCESS; }
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

//namespace Offboard
//{
//  static ReturnCode Initialize()
//  {
//#if USE_OFFBOARD_VISION
//    PRINT("Offboard::Init(): Registering message IDs for offboard processing.\n");
//
//    // Register all the message IDs we need with Matlab:
//    HAL::SendMessageID("CozmoMsg_BlockMarkerObserved",
//      GET_MESSAGE_ID(Messages::BlockMarkerObserved));
//
//    HAL::SendMessageID("CozmoMsg_TemplateInitialized",
//      GET_MESSAGE_ID(Messages::TemplateInitialized));
//
//    HAL::SendMessageID("CozmoMsg_TotalVisionMarkersSeen",
//      GET_MESSAGE_ID(Messages::TotalVisionMarkersSeen));
//
//    HAL::SendMessageID("CozmoMsg_DockingErrorSignal",
//      GET_MESSAGE_ID(Messages::DockingErrorSignal));
//
//    HAL::SendMessageID("CozmoMsg_VisionMarker",
//      GET_MESSAGE_ID(Messages::VisionMarker));
//
//    {
//      for(s32 i=0; i<Vision::NUM_MARKER_TYPES; ++i) {
//        HAL::SendMessageID(Vision::MarkerTypeStrings[i], i);
//      }
//    }
//
//    //HAL::SendMessageID("CozmoMsg_HeadCameraCalibration",
//    //                   GET_MESSAGE_ID(Messages::HeadCameraCalibration));
//
//    // TODO: Update this to send mat and head cam calibration separately
//    PRINT("Offboard::Init(): Sending head camera calibration to "
//      "offoard vision processor.\n");
//
//    // Create a camera calibration message and send it to the offboard
//    // vision processor
//    Messages::HeadCameraCalibration headCalibMsg = {
//      headCamInfo_->focalLength_x,
//      headCamInfo_->focalLength_y,
//      headCamInfo_->fov_ver,
//      headCamInfo_->center_x,
//      headCamInfo_->center_y,
//      headCamInfo_->skew,
//      headCamInfo_->nrows,
//      headCamInfo_->ncols
//    };
//
//    //HAL::USBSendMessage(&msg, GET_MESSAGE_ID(HeadCameraCalibration));
//    HAL::USBSendPacket(HAL::USB_VISION_COMMAND_HEAD_CALIBRATION,
//      &headCalibMsg, sizeof(Messages::HeadCameraCalibration));
//
//    /* Don't need entire calibration, just pixPerMM
//
//    Messages::MatCameraCalibration matCalibMsg = {
//    matCamInfo_->focalLength_x,
//    matCamInfo_->focalLength_y,
//    matCamInfo_->fov_ver,
//    matCamInfo_->center_x,
//    matCamInfo_->center_y,
//    matCamInfo_->skew,
//    matCamInfo_->nrows,
//    matCamInfo_->ncols
//    };
//
//    HAL::USBSendPacket(HAL::USB_VISION_COMMAND_MAT_CALIBRATION,
//    &matCalibMsg, sizeof(Messages::MatCameraCalibration));
//    */
//    /*
//    HAL::USBSendPacket(HAL::USB_VISION_COMMAND_MAT_CALIBRATION,
//    &matCamPixPerMM_, sizeof(matCamPixPerMM_));
//    */
//#endif // USE_OFFBOARD_VISION
//    return EXIT_SUCCESS;
//  } // static ReturnCode Initialize()
//} // namespace Offboard

namespace VisionState {
  static const s32 MAX_TRACKING_FAILURES = 5;

  static const Anki::Cozmo::HAL::CameraInfo* headCamInfo_;

  static VisionSystemMode mode_;

  static Anki::Vision::MarkerType markerTypeToTrack_;
  static Quadrilateral<f32> trackingQuad_;
  static s32 numTrackFailures_ ; //< The tracker can fail to converge this many times before we give up and reset the docker
  static bool isTemplateInitialized_;
  static bool isTrackingMarkerSpecified_;
  static Tracker tracker_;

  //// Capture an entire frame using HAL commands and put it in the given frame buffer
  //typedef struct {
  //  u8* data;
  //  HAL::CameraMode resolution;
  //  TimeStamp_t  timestamp;
  //} FrameBuffer;

  static ReturnCode Initialize()
  {
    mode_ = VISION_MODE_LOOKING_FOR_MARKERS;
    headCamInfo_ = NULL;
    markerTypeToTrack_ = Anki::Vision::MARKER_UNKNOWN;
    numTrackFailures_ = 0;
    isTemplateInitialized_ = false;
    isTrackingMarkerSpecified_ = false;

#ifdef RUN_SIMPLE_TRACKING_TEST
    Anki::Cozmo::VisionSystem::SetMarkerToTrack(Vision::MARKER_BATTERIES);
#endif

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

    return EXIT_SUCCESS;
  }
} // namespace VisionState

////ReturnCode CaptureHeadFrame(FrameBuffer &frame);
//

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

static ReturnCode DownsampleHelper(
  const Array<u8>& in,
  Array<u8>& out,
  MemoryStack scratch)
{
  const s32 inWidth  = in.get_size(1);
  const s32 inHeight = in.get_size(0);

  const s32 outWidth  = out.get_size(1);
  const s32 outHeight = out.get_size(0);

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

  return EXIT_SUCCESS;
}

static ReturnCode LookForMarkers(
  const Array<u16> &yuvImage,
  const DetectFiducialMarkersParameters::Parameters &parameters,
  MemoryStack offchipScratch,
  MemoryStack onchipScratch,
  MemoryStack ccmScratch)
{
  // TODO: Call embedded vision block detector for each block that's found, create a
  //       CozmoMsg_ObservedBlockMarkerMsg and process it.

  //printf("Stacks usage: %d %d %d\n", offchipScratch_.get_usedBytes(), onchipScratch_.get_usedBytes(), ccmScratch_.get_usedBytes());

  Array<u8> grayscaleImage(parameters.detectionHeight, parameters.detectionWidth, offchipScratch);

  ImageProcessing::YUVToGrayscale(yuvImage, grayscaleImage);

  FixedLengthList<VisionMarker> markers(parameters.maxMarkers, offchipScratch);
  FixedLengthList<Array<f32> > homographies(parameters.maxMarkers, ccmScratch);

  markers.set_size(parameters.maxMarkers);
  homographies.set_size(parameters.maxMarkers);

  for(s32 i=0; i<parameters.maxMarkers; i++) {
    Array<f32> newArray(3, 3, ccmScratch);
    homographies[i] = newArray;
  } // for(s32 i=0; i<maximumSize; i++)

  MatlabVisualization::ResetFiducialDetection();

  InitBenchmarking();

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
    false,
    offchipScratch, onchipScratch, ccmScratch);

  if(result == RESULT_OK) {
    DebugStream::SendFiducialDetection();
    for(s32 i_marker = 0; i_marker < markers.get_size(); ++i_marker) {
      const VisionMarker crntMarker = markers[i_marker];

      MatlabVisualization::SendFiducialDetection();
    }

    //          for(s32 i_marker = 0; i_marker < markers.get_size(); ++i_marker)
    //          {
    //            const VisionMarker crntMarker = markers[i_marker];
    //
    //
    //            MatlabVisualization::SendFiducialDetection(crntMarker.corners);
    //
    //            // Processing the message could have set isDockingBlockFound to true
    //            // If it did, and we haven't already initialized the template tracker
    //            // (thanks to a previous marker setting isDockingBlockFound to true),
    //            // then initialize the template tracker now
    //            if(isTrackingMarkerFound_ && not isTemplateInitialized_)
    //            {
    //              using namespace Embedded;
    //
    //              // I'd rather only initialize trackingQuad_ if InitTemplate()
    //              // succeeds, but InitTemplate downsamples it for the time being,
    //              // since we're still doing template initialization at tracking
    //              // resolution instead of the eventual goal of doing it at full
    //              // detection resolution.
    //              trackingQuad_ = Quadrilateral<f32>(Point<f32>(crntMarker.corners[0].x,
    //                crntMarker.corners[0].y),
    //                Point<f32>(crntMarker.corners[1].x,
    //                crntMarker.corners[1].y),
    //                Point<f32>(crntMarker.corners[2].x,
    //                crntMarker.corners[2].y),
    //                Point<f32>(crntMarker.corners[3].x,
    //                crntMarker.corners[3].y));
    //
    //              //InitializeScratchBuffers(true);
    //
    //              if(InitTemplate(frame, trackingQuad_) == EXIT_SUCCESS)
    //              {
    //                AnkiAssert(isTemplateInitialized_ == true);
    //                SetTrackingMode(isTemplateInitialized_);
    //              }
    //            } // if(isDockingBlockFound_ && not isTemplateInitialized_)
    //          } // for( each marker)
    //
    //          MatlabVisualization::SendDrawNow();
  } // IF SimpleDetector_Steps12345_lowMemory() == RESULT_OK

  return EXIT_SUCCESS;
} // LookForMarkers()

static ReturnCode InitTemplate(
  const Array<u16> &yuvImage,
  const Quadrilateral<f32> &templateQuad,
  const TrackerParameters::Parameters &parameters,
  Tracker &tracker,
  MemoryStack offchipScratch,
  MemoryStack &onchipScratch, //< NOTE: onchip is a reference
  MemoryStack ccmScratch)
{
  Array<u8> grayscaleImage(parameters.detectionHeight, parameters.detectionWidth, offchipScratch);
  ImageProcessing::YUVToGrayscale(yuvImage, grayscaleImage);

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
  // TODO: At some point template initialization should happen at full detection resolution but for
  //       now, we have to downsample to tracking resolution

  Array<u8> grayscaleImageSmall(parameters.trackHeight, parameters.trackWidth, ccmScratch);
  DownsampleHelper(grayscaleImage, grayscaleImageSmall, ccmScratch);

  // Note that the templateRegion and the trackingQuad are both at DETECTION_RESOLUTION, not
  // necessarily the resolution of the frame.
  const s32 downsamplePower = static_cast<s32>(HAL::CameraModeInfo[TRACKING_RESOLUTION].downsamplePower[DETECTION_RESOLUTION]);
  const f32 downsampleFactor = static_cast<f32>(1 << downsamplePower);

  for(u8 i=0; i<4; ++i) {
    templateQuad[i].x /= downsampleFactor;
    templateQuad[i].y /= downsampleFactor;
  }
#endif // #if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
  tracker = TemplateTracker::LucasKanadeTrackerFast(
    imageSmall,
    templateQuad,
    NUM_TRACKING_PYRAMID_LEVELS,
    Transformations::TRANSFORM_AFFINE,
    TRACKING_RIDGE_WEIGHT,
    onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
  tracker = TemplateTracker::LucasKanadeTracker_f32(
    imageSmall,
    templateQuad,
    NUM_TRACKING_PYRAMID_LEVELS,
    Transformations::TRANSFORM_AFFINE,
    TRACKING_RIDGE_WEIGHT,
    onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
  tracker = TemplateTracker::BinaryTracker(
    grayscaleImage,
    templateQuad,
    parameters.edgeDetection_grayvalueThreshold,
    parameters.edgeDetection_minComponentWidth,
    parameters.edgeDetection_maxDetectionsPerType,
    parameters.edgeDetection_everyNLines,
    onchipScratch);
#endif

  if(!tracker.IsValid()) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
} // InitTemplate()

namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
      ReturnCode Init()
      {
        if(!isInitialized_) {
          // WARNING: the order of these initializations matter!
          // TODO: Figure out the intertwinedness
          // TODO: add error handling
          VisionState::Initialize();
          VisionMemory::Initialize();

          DebugStream::Initialize();
          DetectFiducialMarkersParameters::Initialize();
          TrackerParameters::Initialize();
          SimulatorParameters::Initialize();
          MatlabVisualization::Initialize();
          //Offboard::Initialize();

          isInitialized_ = true;
        }

        return EXIT_SUCCESS;
      }

      void Destroy()
      {
      }

      ReturnCode SetMarkerToTrack(const Vision::MarkerType& markerToTrack)
      {
        VisionState::markerTypeToTrack_ = markerToTrack;
        VisionState::isTemplateInitialized_ = false;
        VisionState::isTrackingMarkerSpecified_ = true;
        VisionState::mode_ = VISION_MODE_LOOKING_FOR_MARKERS;

        return EXIT_SUCCESS;
      }

      void SetTrackingMode(const bool isTemplateInitalized)
      {
      }

      void UpdateTrackingStatus(const bool didTrackingSucceed)
      {
      }

      void CheckForTrackingMarker(const u16 inputMarker)
      {
      }

      void StopTracking()
      {
        VisionState::mode_ = VISION_MODE_IDLE;
      }

      //      ReturnCode Update_Offboard()
      //      {
      //#if USE_OFFBOARD_VISION
      //        //PRINT("VisionSystem::Update(): waiting for processing result.\n");
      //        Messages::ProcessUARTMessages();
      //        if(Messages::StillLookingForID()) {
      //          // Still waiting, skip further vision processing below.
      //          return EXIT_SUCCESS;
      //        }
      //
      //        if(VisionState::mode_ == VISION_MODE_IDLE) {
      //        } else if(VisionState::mode_ == LOOKING_FOR_MARKERS) {
      //          // Send the offboard vision processor the frame, with the command to look for blocks in
      //          // it. Note that if we previsouly sent the offboard processor a message to set the docking
      //          // block type, it will also initialize a template tracker once that block type is seen
      //          HAL::USBSendFrame(frame.data, frame.timestamp,
      //            frame.resolution, DETECTION_RESOLUTION,
      //            HAL::USB_VISION_COMMAND_DETECTBLOCKS);
      //
      //          Messages::LookForID( GET_MESSAGE_ID(Messages::TotalVisionMarkersSeen) );
      //#ifdef SIMULATOR
      //          frameRdyTimeUS_ = HAL::GetMicroCounter() + LOOK_FOR_BLOCK_PERIOD_US;
      //#endif
      //        } else if(VisionState::mode_ == VISION_MODE_TRACKING) {
      //#ifdef SIMULATOR
      //          frameRdyTimeUS_ = HAL::GetMicroCounter() + TRACK_BLOCK_PERIOD_US;
      //#endif
      //
      //          if(TrackTemplate(frame) != EXIT_SUCCESS) {
      //            PRINT("VisionSystem::Update(): TrackTemplate() failed.\n");
      //            return EXIT_FAILURE;
      //          }
      //        } else {
      //          PRINT("VisionSystem::Update(): reached default case in switch statement.");
      //          return EXIT_FAILURE;
      //        }
      //#endif // USE_OFFBOARD_VISION
      //        return EXIT_SUCCESS;
      //      }// ReturnCode Update_Offboard()

      ReturnCode Update()
      {
        // This should be called from elsewhere first, but calling it again won't hurt
        Init();

#ifdef SIMULATOR
        if (HAL::GetMicroCounter() < frameRdyTimeUS_) {
          return retVal;
        }
#endif

        //#if USE_OFFBOARD_VISION
        //        return Update_Offboard();
        //#endif // USE_OFFBOARD_VISION

        // TODO: set size via HAL
        while(!isEOF)
        {
        }

        const Array<u16> yuvImage(240, 320, m_buffer1, 320*240*2, Flags::Buffer(false, false, true));

        if(VisionState::mode_ == VISION_MODE_IDLE) {
          // Nothing to do!
        } else if(VisionState::mode_ == VISION_MODE_LOOKING_FOR_MARKERS) {
          // Note that if a docking block was specified and we see it while looking for blocks, a
          // tracking template will be initialized and, if that's successful, we will switch to
          // DOCKING mode.
          if(LookForMarkers(
            yuvImage,
            DetectFiducialMarkersParameters::parameters_,
            VisionMemory::offchipScratch_,
            VisionMemory::onchipScratch_,
            VisionMemory::ccmScratch_) != EXIT_SUCCESS)
            return EXIT_FAILURE;
#ifdef SIMULATOR
          frameRdyTimeUS_ = HAL::GetMicroCounter() + LOOK_FOR_BLOCK_PERIOD_US;
#endif
        } else if(VisionState::mode_ == VISION_MODE_TRACKING) {
          //#ifdef SIMULATOR
          //          frameRdyTimeUS_ = HAL::GetMicroCounter() + TRACK_BLOCK_PERIOD_US;
          //#endif
          //
          //          if(TrackTemplate(frame) != EXIT_SUCCESS) {
          //            PRINT("VisionSystem::Update(): TrackTemplate() failed.\n");
          //            return EXIT_FAILURE;
          //          }
        } else {
          PRINT("VisionSystem::Update(): reached default case in switch statement.");
          return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
      } // Update()

      //      ReturnCode SetMarkerToTrack(const Vision::MarkerType& markerToTrack)
      //      {
      //        markerTypeToTrack_ = markerToTrack;
      //
      //        isTrackingMarkerFound_ = false;
      //        isTemplateInitialized_ = false;
      //        numTrackFailures_      = 0;
      //
      //        mode_ = VISION_MODE_LOOKING_FOR_MARKERS;
      //
      //#if USE_OFFBOARD_VISION
      //        // Let the offboard vision processor know that it should be looking for
      //        // this block type as well, so it can do template initialization on the
      //        // same frame where it sees the block, instead of needing a second
      //        // USBSendFrame call.
      //        HAL::USBSendPacket(HAL::USB_VISION_COMMAND_SETTRACKMARKER,
      //          &markerTypeToTrack_, sizeof(markerTypeToTrack_));
      //#endif
      //        return EXIT_SUCCESS;
      //      }

      //void CheckForTrackingMarker(const u16 inputMarker)
      //{
      //  // If we have a block to dock with set, see if this was it
      //  if(markerTypeToTrack_ == static_cast<Vision::MarkerType>(inputMarker))
      //  {
      //    isTrackingMarkerFound_ = true;
      //  }
      //}

      //void SetTrackingMode(const bool isTemplateInitalized)
      //{
      //  if(isTemplateInitalized && isTrackingMarkerFound_)
      //  {
      //    //PRINT("Tracking template initialized, switching to DOCKING mode.\n");
      //    isTemplateInitialized_ = true;
      //    isTrackingMarkerFound_ = true;

      //    // If we successfully initialized a tracking template,
      //    // switch to docking mode.  Otherwise, we'll keep looking
      //    // for the block and try again
      //    mode_ = VISION_MODE_TRACKING;
      //  }
      //  else {
      //    isTemplateInitialized_ = false;
      //    isTrackingMarkerFound_ = false;
      //  }
      //}

      //void UpdateTrackingStatus(const bool didTrackingSucceed)
      //{
      //  if(didTrackingSucceed) {
      //    // Reset the failure counter
      //    numTrackFailures_ = 0;
      //  }
      //  else {
      //    ++numTrackFailures_;
      //    if(numTrackFailures_ == MAX_TRACKING_FAILURES) {
      //      // This resets docking, puttings us back in VISION_MODE_LOOKING_FOR_MARKERS mode
      //      SetMarkerToTrack(markerTypeToTrack_);
      //      numTrackFailures_ = 0;
      //    }
      //  }
      //} // UpdateTrackingStatus()

      //void DownsampleHelper(const FrameBuffer& frame, Array<u8>& image,
      //  const HAL::CameraMode outputResolution,
      //  MemoryStack scratch)
      //{
      //  using namespace Embedded;

      //  const u16 outWidth  = HAL::CameraModeInfo[outputResolution].width;
      //  const u16 outHeight = HAL::CameraModeInfo[outputResolution].height;

      //  AnkiAssert(image.get_size(0) == outHeight &&
      //    image.get_size(1) == outWidth);
      //  //image = Array<u8>(outHeight, outWidth, scratch);

      //  const s32 downsamplePower = static_cast<s32>(HAL::CameraModeInfo[outputResolution].downsamplePower[frame.resolution]);

      //  if(downsamplePower > 0)
      //  {
      //    const u16 frameWidth   = HAL::CameraModeInfo[frame.resolution].width;
      //    const u16 frameHeight  = HAL::CameraModeInfo[frame.resolution].height;

      //    //PRINT("Downsampling [%d x %d] frame by %d.\n", frameWidth, frameHeight, (1 << downsamplePower));

      //    Array<u8> fullRes(frameHeight, frameWidth,
      //      frame.data, frameHeight*frameWidth,
      //      Flags::Buffer(false,false,false));

      //    ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(fullRes,
      //      downsamplePower,
      //      image,
      //      scratch);
      //  }
      //  else {
      //    // No need to downsample, but need to copy into CMX from DDR
      //    // TODO: ask pete if this is correct
      //    image.Set(frame.data, outHeight*outWidth);
      //  }
      //} // DownsampleHelper()

      //      ReturnCode TrackTemplate(const FrameBuffer &frame)
      //      {
      //        ReturnCode retVal = EXIT_SUCCESS;
      //
      //#if USE_OFFBOARD_VISION
      //
      //        // Send the message out for tracking
      //        HAL::USBSendFrame(frame.data, frame.timestamp,
      //          frame.resolution, TRACKING_RESOLUTION,
      //          HAL::USB_VISION_COMMAND_TRACK);
      //
      //        Messages::LookForID( GET_MESSAGE_ID(Messages::DockingErrorSignal) );
      //
      //#else // ONBOARD VISION
      //
      //        if(offchipScratch_.IsValid() && onchipScratch_.IsValid() && ccmScratch_.IsValid())
      //        {
      //          // So that we don't leak memory allocating image, etc, below,
      //          // we want to ensure that memory gets popped when this scope ends
      //          PUSH_MEMORY_STACK(offchipScratch_);
      //          PUSH_MEMORY_STACK(onchipScratch_);
      //          PUSH_MEMORY_STACK(ccmScratch_);
      //
      //          //PRINT("TrackerScratch memory usage = %d of %d\n", trackerScratch_.get_usedBytes(), trackerScratch_.get_totalBytes());
      //
      //          using namespace TemplateTracker;
      //
      //          AnkiAssert(tracker_.IsValid());
      //
      //          const u16 detectionWidth  = HAL::CameraModeInfo[DETECTION_RESOLUTION].width;
      //          const u16 detectionHeight = HAL::CameraModeInfo[DETECTION_RESOLUTION].height;
      //
      //          // NOTE: this image will sit in the MemoryStack until tracking fails
      //          //       and we reconstruct the MemoryStack
      //
      //#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
      //          // TODO: At some point template initialization should happen at full detection resolution
      //          //       but for now, we have to downsample to tracking resolution
      //          const u16 trackHeight = HAL::CameraModeInfo[TRACKING_RESOLUTION].height;
      //          const u16 trackWidth  = HAL::CameraModeInfo[TRACKING_RESOLUTION].width;
      //
      //          Array<u8> imageLarge(detectionHeight, detectionWidth, offchipScratch_);
      //          Array<u8> imageSmall(trackHeight, trackWidth, ccmScratch_);
      //          DownsampleHelper(imageLarge, imageSmall, ccmScratch_);
      //          YUVToGrayscaleHelper(frame, imageLarge);
      //#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
      //          Array<u8> imageLarge(detectionHeight, detectionWidth, onchipScratch_);
      //          YUVToGrayscaleHelper(frame, imageLarge);
      //#endif
      //
      //          Messages::DockingErrorSignal dockErrMsg;
      //          dockErrMsg.timestamp = frame.timestamp;
      //
      //          bool converged = false;
      //
      //#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
      //          const Result trackerResult = tracker_.UpdateTrack(imageSmall, TRACKING_MAX_ITERATIONS,
      //            TRACKING_CONVERGENCE_TOLERANCE,
      //            converged,
      //            onchipScratch_);
      //#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
      //          const Result trackerResult = tracker_.UpdateTrack(imageSmall, TRACKING_MAX_ITERATIONS,
      //            TRACKING_CONVERGENCE_TOLERANCE,
      //            TRACKING_USE_WEIGHTS,
      //            converged,
      //            onchipScratch_);
      //#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
      //          s32 numMatches = -1;
      //
      //          const Result trackerResult = tracker_.UpdateTrack(imageLarge,
      //            edgeDetection_grayvalueThreshold, edgeDetection_minComponentWidth, edgeDetection_maxDetectionsPerType, edgeDetection_everyNLines,
      //            matching_maxTranslationDistance, matching_maxProjectiveDistance,
      //            verification_maxTranslationDistance,
      //            false,
      //            numMatches,
      //            ccmScratch_, offchipScratch_);
      //
      //          const s32 numTemplatePixels = tracker_.get_numTemplatePixels();
      //
      //          const f32 percentMatchedPixels = static_cast<f32>(numMatches) / static_cast<f32>(numTemplatePixels);
      //
      //          if(percentMatchedPixels >= percentMatchedPixelsThreshold) {
      //            converged = true;
      //          }
      //
      //          //tracker_.get_transformation().Print("tracker_");
      //          //printf("%f %f\n", *tracker_.get_transformation().get_homography().Pointer(0,2), *tracker_.get_transformation().get_homography().Pointer(1,2));
      //
      //          //PRINT("percentMatchedPixels = %f\n", percentMatchedPixels);
      //#endif
      //
      //          if(trackerResult == RESULT_OK)
      //          {
      //            dockErrMsg.didTrackingSucceed = static_cast<u8>(converged);
      //            if(converged)
      //            {
      //              using namespace Embedded;
      //
      //              // TODO: Add CameraMode resolution to CameraInfo
      //              const f32 fxAdj = (static_cast<f32>(headCamInfo_->ncols) /
      //                static_cast<f32>(HAL::CameraModeInfo[TRACKING_RESOLUTION].width));
      //
      //              Docking::ComputeDockingErrorSignal(tracker_.get_transformation(),
      //                HAL::CameraModeInfo[TRACKING_RESOLUTION].width,
      //                29.5f, // TODO: Get this from the docking command message from basestation
      //                headCamInfo_->focalLength_x / fxAdj,
      //                dockErrMsg.x_distErr,
      //                dockErrMsg.y_horErr,
      //                dockErrMsg.angleErr,
      //                onchipScratch_);
      //
      //              SendTrackingUpdate();
      //            } // IF converged
      //
      //            MatlabVisualization::SendTrack()
      //
      //              Messages::ProcessDockingErrorSignalMessage(dockErrMsg);
      //          }
      //          else {
      //            PRINT("VisionSystem::TrackTemplate(): UpdateTrack() failed.\n");
      //            retVal = EXIT_FAILURE;
      //          } // IF UpdateTrack result was OK
      //        }
      //        else {
      //          PRINT("VisionSystem::TrackTemplate(): tracker scratch memory is not valid.\n");
      //          retVal = EXIT_FAILURE;
      //        } // if/else trackerScratch is valid
      //
      //#endif // #if USE_OFFBOARD_VISION
      //
      //        return retVal;
      //      } // TrackTemplate()
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki
