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
#define DOCKING_ALGORITHM DOCKING_LUCAS_KANADE_STANDARD

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
typedef TemplateTracker::LucasKanadeTracker_f32 Tracker;
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
typedef TemplateTracker::LucasKanadeTrackerFast Tracker;
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
typedef TemplateTracker::BinaryTracker Tracker;
#endif

typedef enum {
  VISION_MODE_IDLE,
  VISION_MODE_LOOKING_FOR_MARKERS,
  VISION_MODE_TRACKING
} VisionSystemMode;

static ReturnCode DownsampleHelper(
  const Array<u8>& in,
  Array<u8>& out,
  MemoryStack scratch);

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

    f32 blockMarkerWidthInMM;
    f32 horizontalFocalLengthInMM;

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

      blockMarkerWidthInMM = 29.5f; // TODO: Get this from the docking command message from basestation

      // TODO: Add CameraMode resolution to CameraInfo
#warning set horizontalFocalLengthInMM correctly, by passing in a parameter or something
      const f32 fxAdj = static_cast<f32>(detectionWidth) / static_cast<f32>(trackingImageWidth);
      const f32 focalLength_x = -10e10f; //headCamInfo_->focalLength_x
      horizontalFocalLengthInMM = focalLength_x / fxAdj;
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
    s32 trackingImageHeight;
    s32 trackingImageWidth;
    u8 edgeDetection_grayvalueThreshold;
    s32 edgeDetection_minComponentWidth;
    s32 edgeDetection_maxDetectionsPerType;
    s32 edgeDetection_everyNLines;
    s32 matching_maxTranslationDistance;
    s32 matching_maxProjectiveDistance;
    s32 verification_maxTranslationDistance;
    f32 percentMatchedPixelsThreshold;

    f32 blockMarkerWidthInMM;
    f32 horizontalFocalLengthInMM;

    Parameters()
    {
      // TODO: set via HAL
      detectionWidth  = 320;
      detectionHeight = 240;
      edgeDetection_grayvalueThreshold = 128;
      edgeDetection_minComponentWidth = 2;
      edgeDetection_maxDetectionsPerType = 2500;
      edgeDetection_everyNLines = 1;
      matching_maxTranslationDistance = 5;
      matching_maxProjectiveDistance = 5;
      verification_maxTranslationDistance = 1;
      percentMatchedPixelsThreshold = 0.5f; // TODO: pick a reasonable value

      blockMarkerWidthInMM = 29.5f; // TODO: Get this from the docking command message from basestation

      // TODO: Add CameraMode resolution to CameraInfo
#warning set horizontalFocalLengthInMM correctly, by passing in a parameter or something
      const f32 fxAdj = static_cast<f32>(detectionWidth) / static_cast<f32>(trackingImageWidth);
      const f32 focalLength_x = -10e10f; //headCamInfo_->focalLength_x
      horizontalFocalLengthInMM = focalLength_x / fxAdj;
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
  static const s32 MAX_MARKERS = 100;

  static OFFCHIP char offchipBuffer[OFFCHIP_BUFFER_SIZE];
  static ONCHIP char onchipBuffer[ONCHIP_BUFFER_SIZE];
  static CCM char ccmBuffer[CCM_BUFFER_SIZE];

  static MemoryStack offchipScratch_;
  static MemoryStack onchipScratch_;
  static MemoryStack ccmScratch_;

  // Markers is the one things that can move between functions, so it is always allocated in memory
  static FixedLengthList<VisionMarker> markers_;

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

    markers_ = FixedLengthList<VisionMarker>(VisionMemory::MAX_MARKERS, offchipScratch_);

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
  static ReturnCode SendFiducialDetection(const Array<u8> &image, const FixedLengthList<VisionMarker> &markers, MemoryStack ccmScratch, MemoryStack offchipScratch) { return EXIT_SUCCESS; }
  static ReturnCode SendTrackingUpdate(const Array<u8> &image, const Transformations::PlanarTransformation_f32 &transformation, MemoryStack ccmScratch, MemoryStack offchipScratch) { return EXIT_SUCCESS; }
  //static ReturnCode SendPrintf(const char * string) { return EXIT_SUCCESS; }
#else
  static ReturnCode SendBuffer(const SerializedBuffer &toSend)
  {
    s32 startIndex;
    const u8 * bufferStart = reinterpret_cast<const u8*>(toSend.get_memoryStack().get_validBufferStart(startIndex));
    const s32 validUsedBytes = toSend.get_memoryStack().get_usedBytes() - startIndex;

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

    return EXIT_SUCCESS;
  }

  // TODO: Commented out to prevent unused compiler warning. Add back if needed.
  //static ReturnCode SendPrintf(const char * string)
  //{
  //  printfBuffer_ = SerializedBuffer(&printfBufferRaw_[0], PRINTF_BUFFER_SIZE);
  //  printfBuffer_.PushBackString(string);

  //  return SendBuffer(printfBuffer_);
  //} // void SendPrintf(const char * string)

  static ReturnCode SendFiducialDetection(const Array<u8> &image, const FixedLengthList<VisionMarker> &markers, MemoryStack ccmScratch, MemoryStack offchipScratch)
  {
    debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);

    if(markers.get_size() != 0) {
      const s32 numMarkers = markers.get_size();
      const VisionMarker * pMarkers = markers.Pointer(0);

      void * restrict oneMarker = offchipScratch.Allocate(sizeof(VisionMarker));
      const s32 oneMarkerLength = sizeof(VisionMarker);

      for(s32 i=0; i<numMarkers; i++) {
        pMarkers[i].Serialize(oneMarker, oneMarkerLength);
        debugStreamBuffer_.PushBack("VisionMarker", oneMarker, oneMarkerLength);
      }
    }
    
    Array<u8> imageSmall(60, 80, offchipScratch);
    DownsampleHelper(image, imageSmall, ccmScratch);
    debugStreamBuffer_.PushBack(imageSmall);

    return SendBuffer(debugStreamBuffer_);
  } // ReturnCode SendDebugStream_Detection()

  static ReturnCode SendTrackingUpdate(const Array<u8> &image, const Transformations::PlanarTransformation_f32 &transformation, MemoryStack ccmScratch, MemoryStack offchipScratch)
  {
    debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);

    // TODO: get the true length
    const s32 oneTransformationLength = 512;
    void * restrict oneTransformation = ccmScratch.Allocate(oneTransformationLength);
    
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
    transformation.Serialize(oneTransformation, oneTransformationLength);
    // TODO: handle the upscaling
#else
    transformation.Serialize(oneTransformation, oneTransformationLength);
#endif
    
    debugStreamBuffer_.PushBack("PlanarTransformation_f32", oneTransformation, oneTransformationLength);
    
    Array<u8> imageSmall(60, 80, offchipScratch);
    DownsampleHelper(image, imageSmall, ccmScratch);
    debugStreamBuffer_.PushBack(imageSmall);    

    return SendBuffer(debugStreamBuffer_);
  } // static ReturnCode SendTrackingUpdate()
  
  static ReturnCode SendArray(const Array<u8> &array)
  {
    debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
    
    debugStreamBuffer_.PushBack(array);
    
    return SendBuffer(debugStreamBuffer_);
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
  FixedLengthList<VisionMarker> &markers,
  MemoryStack offchipScratch,
  MemoryStack onchipScratch,
  MemoryStack ccmScratch)
{
  // TODO: Call embedded vision block detector for each block that's found, create a
  //       CozmoMsg_ObservedBlockMarkerMsg and process it.

  const s32 maxMarkers = markers.get_maximumSize();

  Array<u8> grayscaleImage(parameters.detectionHeight, parameters.detectionWidth, offchipScratch);

  ImageProcessing::YUVToGrayscale(yuvImage, grayscaleImage);

  FixedLengthList<Array<f32> > homographies(maxMarkers, ccmScratch);

  markers.set_size(maxMarkers);
  homographies.set_size(maxMarkers);

  for(s32 i=0; i<maxMarkers; i++) {
    Array<f32> newArray(3, 3, ccmScratch);
    homographies[i] = newArray;
  }

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
    DebugStream::SendFiducialDetection(grayscaleImage, markers, ccmScratch, offchipScratch);

    for(s32 i_marker = 0; i_marker < markers.get_size(); ++i_marker) {
      const VisionMarker crntMarker = markers[i_marker];

      MatlabVisualization::SendFiducialDetection();
    }

    MatlabVisualization::SendDrawNow();
  } // if(result == RESULT_OK)

  return EXIT_SUCCESS;
} // LookForMarkers()

static ReturnCode InitTemplate(
  const Array<u16> &yuvImage,
  const Quadrilateral<f32> &trackingQuad,
  const TrackerParameters::Parameters &parameters,
  Tracker &tracker,
  MemoryStack offchipScratch,
  MemoryStack &onchipScratch, //< NOTE: onchip is a reference
  MemoryStack ccmScratch)
{
  Array<u8> grayscaleImage(parameters.detectionHeight, parameters.detectionWidth, offchipScratch);
  ImageProcessing::YUVToGrayscale(yuvImage, grayscaleImage);

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
  // TODO: At some point template initialization should happen at full detection resolution but for
  //       now, we have to downsample to tracking resolution

  Array<u8> grayscaleImageSmall(parameters.trackingImageHeight, parameters.trackingImageWidth, ccmScratch);
  DownsampleHelper(grayscaleImage, grayscaleImageSmall, ccmScratch);

  // Note that the templateRegion and the trackingQuad are both at DETECTION_RESOLUTION, not
  // necessarily the resolution of the frame.
  const u32 downsampleFactor = parameters.detectionWidth / parameters.trackingImageWidth;
  //const u32 downsamplePower = Log2u32(downsampleFactor);
  
  Quadrilateral<f32> trackingQuadSmall;
  
  for(s32 i=0; i<4; ++i) {
    trackingQuadSmall[i].x = trackingQuad[i].x / downsampleFactor;
    trackingQuadSmall[i].y = trackingQuad[i].y / downsampleFactor;
  }
#endif // #if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
  tracker = TemplateTracker::LucasKanadeTracker_f32(
    grayscaleImageSmall,
    trackingQuadSmall,
    parameters.numPyramidLevels,
    Transformations::TRANSFORM_TRANSLATION,
    parameters.ridgeWeight,
    onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
  tracker = TemplateTracker::LucasKanadeTrackerFast(
    grayscaleImageSmall,
    trackingQuadSmall,
    parameters.numPyramidLevels,
    Transformations::TRANSFORM_AFFINE,
    parameters.ridgeWeight,
    onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
  tracker = TemplateTracker::BinaryTracker(
    grayscaleImage,
    trackingQuad,
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

static ReturnCode TrackTemplate(
  const Array<u16> &yuvImage,
  const Quadrilateral<f32> &trackingQuad,
  const TrackerParameters::Parameters &parameters,
  Tracker &tracker,
  Messages::DockingErrorSignal &dockErrMsg,
  bool &converged,
  MemoryStack offchipScratch,
  MemoryStack onchipScratch,
  MemoryStack ccmScratch)
{
  //#if USE_OFFBOARD_VISION
  //  // Send the message out for tracking
  //  HAL::USBSendFrame(frame.data, frame.timestamp,
  //    frame.resolution, TRACKING_RESOLUTION,
  //    HAL::USB_VISION_COMMAND_TRACK);
  //
  //  Messages::LookForID( GET_MESSAGE_ID(Messages::DockingErrorSignal) );
  //
  //  return EXIT_SUCCESS;
  //#endif

  Array<u8> grayscaleImage(parameters.detectionHeight, parameters.detectionWidth, offchipScratch);
  ImageProcessing::YUVToGrayscale(yuvImage, grayscaleImage);

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
  // TODO: At some point template initialization should happen at full detection resolution
  //       but for now, we have to downsample to tracking resolution
  Array<u8> grayscaleImageSmall(parameters.trackingImageHeight, parameters.trackingImageWidth, ccmScratch);
  DownsampleHelper(grayscaleImage, grayscaleImageSmall, ccmScratch);
#endif

  converged = false;

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_STANDARD
  const Result trackerResult = tracker.UpdateTrack(
    grayscaleImage,
    parameters.maxIterations,
    parameters.convergenceTolerance,
    parameters.useWeights,
    converged,
    onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_FAST
  const Result trackerResult = tracker.UpdateTrack(
    grayscaleImageSmall, 
    parameters.maxIterations,
    parameters.convergenceTolerance,
    converged,
    onchipScratch);

#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
  s32 numMatches = -1;

  const Result trackerResult = tracker.UpdateTrack(
    grayscaleImage,
    parameters.edgeDetection_grayvalueThreshold, parameters.edgeDetection_minComponentWidth, parameters.edgeDetection_maxDetectionsPerType, parameters.edgeDetection_everyNLines,
    parameters.matching_maxTranslationDistance, parameters.matching_maxProjectiveDistance,
    parameters.verification_maxTranslationDistance,
    false,
    numMatches,
    ccmScratch, offchipScratch);

  const s32 numTemplatePixels = tracker.get_numTemplatePixels();

  const f32 percentMatchedPixels = static_cast<f32>(numMatches) / static_cast<f32>(numTemplatePixels);

  if(percentMatchedPixels >= parameters.percentMatchedPixelsThreshold) {
    converged = true;
  }
#endif

  if(trackerResult != RESULT_OK) {
    return EXIT_FAILURE;
  }

  dockErrMsg.didTrackingSucceed = static_cast<u8>(converged);

  if(converged) {
    Docking::ComputeDockingErrorSignal(tracker.get_transformation(),
      parameters.trackingImageWidth,
      parameters.blockMarkerWidthInMM,
      parameters.horizontalFocalLengthInMM,
      dockErrMsg.x_distErr,
      dockErrMsg.y_horErr,
      dockErrMsg.angleErr,
      onchipScratch);
  }

  DebugStream::SendTrackingUpdate(grayscaleImage, tracker.get_transformation(), ccmScratch, offchipScratch);

  MatlabVisualization::SendTrack();

  return EXIT_SUCCESS;
} // TrackTemplate()

//
// These functions below should be the only ones to access global variables in a different namespace
//
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
        VisionState::mode_ = VISION_MODE_LOOKING_FOR_MARKERS;
        VisionState::numTrackFailures_ = 0;

        // If the marker type is valid, start looking for it
        if(markerToTrack < Vision::NUM_MARKER_TYPES) {
          VisionState::isTrackingMarkerSpecified_ = true;
        } else {
          VisionState::isTrackingMarkerSpecified_ = false;
        }

        return EXIT_SUCCESS;
      }

#warning isTemplateInitalized is ignored
      void SetTrackingMode(const bool isTemplateInitalized)
      {
        VisionState::mode_ = VISION_MODE_TRACKING;
      }

      void UpdateTrackingStatus(const bool didTrackingSucceed)
      {
        if(didTrackingSucceed) {
          // Reset the failure counter
          VisionState::numTrackFailures_ = 0;
        } else {
          VisionState::numTrackFailures_ += 1;

          if(VisionState::numTrackFailures_ == VisionState::MAX_TRACKING_FAILURES) {
            // This resets docking, puttings us back in VISION_MODE_LOOKING_FOR_MARKERS mode
            // TODO: add back
            //SetMarkerToTrack(VisionState::markerTypeToTrack_);
          }
        }
      }

#warning This function is unused?
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
#ifdef SIMULATOR
          frameRdyTimeUS_ = HAL::GetMicroCounter() + LOOK_FOR_BLOCK_PERIOD_US;
#endif

          VisionMemory::ResetBuffers();

          const ReturnCode result = LookForMarkers(
            yuvImage,
            DetectFiducialMarkersParameters::parameters_,
            VisionMemory::markers_,
            VisionMemory::offchipScratch_,
            VisionMemory::onchipScratch_,
            VisionMemory::ccmScratch_);

          if(result != EXIT_SUCCESS) {
            return EXIT_FAILURE;
          }

          // Was the desired marker found? If so, start tracking it.
          if(VisionState::isTrackingMarkerSpecified_) {
            s32 desiredMarkerIndex = -1;
            const s32 numMarkers = VisionMemory::markers_.get_size();
            for(s32 i_marker = 0; i_marker < numMarkers; ++i_marker) {
              const VisionMarker crntMarker = VisionMemory::markers_[i_marker];
              if(crntMarker.markerType == VisionState::markerTypeToTrack_) {
                desiredMarkerIndex = i_marker;
                break;
              }
            } // if(VisionState::isTrackingMarkerSpecified_)

            if(desiredMarkerIndex != -1) {
              const VisionMarker crntMarker = VisionMemory::markers_[desiredMarkerIndex];

              // I'd rather only initialize trackingQuad_ if InitTemplate() succeeds, but
              // InitTemplate downsamples it for the time being, since we're still doing template
              // initialization at tracking resolution instead of the eventual goal of doing it at
              // full detection resolution.
              VisionState::trackingQuad_ = Quadrilateral<f32>(
                Point<f32>(crntMarker.corners[0].x, crntMarker.corners[0].y),
                Point<f32>(crntMarker.corners[1].x, crntMarker.corners[1].y),
                Point<f32>(crntMarker.corners[2].x, crntMarker.corners[2].y),
                Point<f32>(crntMarker.corners[3].x, crntMarker.corners[3].y));

              const ReturnCode result = InitTemplate(
                yuvImage,
                VisionState::trackingQuad_,
                TrackerParameters::parameters_,
                VisionState::tracker_,
                VisionMemory::offchipScratch_,
                VisionMemory::onchipScratch_, //< NOTE: onchip is a reference
                VisionMemory::ccmScratch_);

              if(result != EXIT_SUCCESS) {
                return EXIT_FAILURE;
              }

              SetTrackingMode(false);
            } // if(desiredMarkerIndex != -1)
          } // else if(VisionState::mode_ == VISION_MODE_LOOKING_FOR_MARKERS)
        } else if(VisionState::mode_ == VISION_MODE_TRACKING) {
#ifdef SIMULATOR
          frameRdyTimeUS_ = HAL::GetMicroCounter() + TRACK_BLOCK_PERIOD_US;
#endif

          Messages::DockingErrorSignal dockErrMsg;
          
          bool converged;

          // TODO: set dockErrMsg timestamp

          const ReturnCode result = TrackTemplate(
            yuvImage,
            VisionState::trackingQuad_,
            TrackerParameters::parameters_,
            VisionState::tracker_,
            dockErrMsg,
            converged,
            VisionMemory::offchipScratch_,
            VisionMemory::onchipScratch_,
            VisionMemory::ccmScratch_);

          if(result != EXIT_SUCCESS) {
            PRINT("VisionSystem::Update(): TrackTemplate() failed.\n");
            return EXIT_FAILURE;
          }
          
          UpdateTrackingStatus(converged);

          Messages::ProcessDockingErrorSignalMessage(dockErrMsg);
        } else {
          PRINT("VisionSystem::Update(): reached default case in switch statement.");
          return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
      } // Update()
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki
