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

static bool isInitialized_ = false;

// TODO: remove
//#define SEND_DEBUG_STREAM
//#define RUN_SIMPLE_TRACKING_TEST

#if defined(SIMULATOR)
#undef SEND_DEBUG_STREAM
#undef RUN_SIMPLE_TRACKING_TEST
#endif

#define DOCKING_LUCAS_KANADE_SLOW       1 //< LucasKanadeTracker_Slow (doesn't seem to work?)
#define DOCKING_LUCAS_KANADE_AFFINE     2 //< LucasKanadeTracker_Affine (With Translation + Affine option)
#define DOCKING_LUCAS_KANADE_PROJECTIVE 3 //< LucasKanadeTracker_Projective (With Projective + Affine option)
#define DOCKING_BINARY_TRACKER          4 //< BinaryTracker
#define DOCKING_ALGORITHM DOCKING_LUCAS_KANADE_AFFINE

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW
typedef TemplateTracker::LucasKanadeTracker_Slow Tracker;
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
typedef TemplateTracker::LucasKanadeTracker_Affine Tracker;
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
typedef TemplateTracker::LucasKanadeTracker_Projective Tracker;
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
typedef TemplateTracker::BinaryTracker Tracker;
#endif

typedef enum {
  VISION_MODE_IDLE,
  VISION_MODE_LOOKING_FOR_MARKERS,
  VISION_MODE_TRACKING
} VisionSystemMode;

static u32 DownsampleHelper(
  const Array<u8>& in,
  Array<u8>& out,
  MemoryStack scratch);

typedef struct {
  u8  headerByte; // used to specify a frame's resolution in a packet if transmitting
  u16 width, height;
  u8 downsamplePower[HAL::CAMERA_MODE_COUNT];
} CameraModeInfo_t;

// NOTE: To get the downsampling power to go from resoution "FROM" to
//       resolution "TO", use:
//       u8 power = CameraModeInfo[FROM].downsamplePower[TO];
const CameraModeInfo_t CameraModeInfo[HAL::CAMERA_MODE_COUNT] =
{
  // VGA
  { 0xBA, 640, 480, {0, 1, 2, 3, 4} },
  // QVGA
  { 0xBC, 320, 240, {0, 0, 1, 2, 3} },
  // QQVGA
  { 0xB8, 160, 120, {0, 0, 0, 1, 2} },
  // QQQVGA
  { 0xBD,  80,  60, {0, 0, 0, 0, 1} },
  // QQQQVGA
  { 0xB7,  40,  30, {0, 0, 0, 0, 0} }
};

#if 0
#pragma mark --- DetectFiducialMarkersParameters ---
#endif

struct DetectFiducialMarkersParameters
{
  bool isInitialized;
  HAL::CameraMode detectionResolution;
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

  //Parameters() : isInitialized(false) { }

  DetectFiducialMarkersParameters() : isInitialized(false) { }

  void Initialize()
  {
    detectionResolution = HAL::CAMERA_MODE_QVGA;
    detectionWidth  = CameraModeInfo[detectionResolution].width;
    detectionHeight = CameraModeInfo[detectionResolution].height;

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

    isInitialized = true;
  } // DetectFiducialMarkersParameters()
}; // struct DetectFiducialMarkersParameters

/*
static DetectFiducialMarkersParameters parameters_;

static ReturnCode Initialize()
{
parameters_ = DetectFiducialMarkersParameters::Parameters();

return EXIT_SUCCESS;
}
*/

#if 0
#pragma mark --- TrackerParameters ---
#endif

struct TrackerParameters {
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE

  bool isInitialized;
  HAL::CameraMode trackingResolution;
  s32 trackingImageHeight;
  s32 trackingImageWidth;
  s32 numPyramidLevels;
  s32 maxIterations;
  f32 convergenceTolerance;
  bool useWeights;

  void Initialize()
  {
    // LK Trackers running at QQQVGA (80x60)
    //trackingResolution   = HAL::CAMERA_MODE_QQQVGA; // 80x60
    trackingResolution   = HAL::CAMERA_MODE_QQVGA; // 80x60

    trackingImageWidth   = CameraModeInfo[trackingResolution].width;
    trackingImageHeight  = CameraModeInfo[trackingResolution].height;
    numPyramidLevels     = 4;
    maxIterations        = 25;
    convergenceTolerance = 0.05f;
    useWeights           = true;

    isInitialized = true;
  } // TrackerParameters()

#else // #if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE

  bool isInitialized;
  HAL::CameraMode trackingResolution;
  s32 trackingImageHeight;
  s32 trackingImageWidth;
  u8  edgeDetection_grayvalueThreshold;
  s32 edgeDetection_minComponentWidth;
  s32 edgeDetection_maxDetectionsPerType;
  s32 edgeDetection_everyNLines;
  s32 matching_maxTranslationDistance;
  s32 matching_maxProjectiveDistance;
  s32 verification_maxTranslationDistance;
  f32 percentMatchedPixelsThreshold;

  void Initialize() //const HAL::CameraMode detectionResolution, const f32 detectionFocalLength)
  {
    // Binary tracker works at QVGA (unlike LK)
    trackingResolution = HAL::CAMERA_MODE_QVGA;

    trackingImageWidth  = CameraModeInfo[trackingResolution].width;
    trackingImageHeight = CameraModeInfo[trackingResolution].height;

    edgeDetection_grayvalueThreshold    = 128;
    edgeDetection_minComponentWidth     = 2;
    edgeDetection_maxDetectionsPerType  = 2500;
    edgeDetection_everyNLines           = 1;
    matching_maxTranslationDistance     = 7;
    matching_maxProjectiveDistance      = 7;
    verification_maxTranslationDistance = 1;
    percentMatchedPixelsThreshold       = 0.5f; // TODO: pick a reasonable value

    isInitialized = true;
  } // TrackerParameters();

#endif // #if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE

  TrackerParameters() : isInitialized(false) { }
}; // struct TrackerParameters

#if 0
#pragma mark --- VisionMemory ---
#endif

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

#if 0
#pragma mark --- DebugStream ---
#endif

namespace DebugStream
{
  static const s32 PRINTF_BUFFER_SIZE = 10000;
  static const s32 DEBUG_STREAM_BUFFER_SIZE = 2000000;

  static OFFCHIP u8 printfBufferRaw_[PRINTF_BUFFER_SIZE];
  static OFFCHIP u8 debugStreamBufferRaw_[DEBUG_STREAM_BUFFER_SIZE];

  static SerializedBuffer printfBuffer_;
  static SerializedBuffer debugStreamBuffer_;

#if !defined(SEND_DEBUG_STREAM)
  static ReturnCode SendFiducialDetection(const Array<u8> &image, const FixedLengthList<VisionMarker> &markers, MemoryStack ccmScratch, MemoryStack onchipScratch, MemoryStack offchipScratch) { return EXIT_SUCCESS; }
  static ReturnCode SendTrackingUpdate(const Array<u8> &image, const Transformations::PlanarTransformation_f32 &transformation, MemoryStack ccmScratch, MemoryStack onchipScratch, MemoryStack offchipScratch) { return EXIT_SUCCESS; }
  //static ReturnCode SendPrintf(const char * string) { return EXIT_SUCCESS; }
  //static ReturnCode SendArray(const Array<u8> &array) { return EXIT_SUCCESS; }
#else
  static const HAL::CameraMode debugStreamResolution_ = HAL::CAMERA_MODE_QQQVGA;

  static f32 lastBenchmarkTime_algorithmsOnly;
  static f32 lastBenchmarkDuration_algorithmsOnly;

  static f32 lastBenchmarkTime_total;
  static f32 lastBenchmarkDuration_total;

  static ReturnCode SendBuffer(SerializedBuffer &toSend)
  {
    const f32 curTime = GetTime();
    lastBenchmarkDuration_total = curTime - lastBenchmarkTime_total;
    lastBenchmarkTime_total = curTime;

    const f32 benchmarkTimes[2] = {lastBenchmarkDuration_algorithmsOnly, lastBenchmarkDuration_total};

    toSend.PushBack<f32>(&benchmarkTimes[0], 2*sizeof(f32));

    s32 startIndex;
    const u8 * bufferStart = reinterpret_cast<const u8*>(toSend.get_memoryStack().get_validBufferStart(startIndex));
    const s32 validUsedBytes = toSend.get_memoryStack().get_usedBytes() - startIndex;

    // TODO: does this help?
    /*for(s32 i=0; i<256; i++) {
    Anki::Cozmo::HAL::UARTPutChar('\0');
    }*/

    for(s32 i=0; i<Embedded::SERIALIZED_BUFFER_HEADER_LENGTH; i++) {
      Anki::Cozmo::HAL::UARTPutChar(Embedded::SERIALIZED_BUFFER_HEADER[i]);
    }

    for(s32 i=0; i<validUsedBytes; i++) {
      Anki::Cozmo::HAL::UARTPutChar(bufferStart[i]);
    }

    for(s32 i=0; i<Embedded::SERIALIZED_BUFFER_FOOTER_LENGTH; i++) {
      Anki::Cozmo::HAL::UARTPutChar(Embedded::SERIALIZED_BUFFER_FOOTER[i]);
    }

    // TODO: does this help?
    /*for(s32 i=0; i<256; i++) {
    Anki::Cozmo::HAL::UARTPutChar('\0');
    }*/

    //HAL::MicroWait(50000);

    lastBenchmarkTime_algorithmsOnly = GetTime();

    return EXIT_SUCCESS;
  }

  // TODO: Commented out to prevent unused compiler warning. Add back if needed.
  //static ReturnCode SendPrintf(const char * string)
  //{
  //  printfBuffer_ = SerializedBuffer(&printfBufferRaw_[0], PRINTF_BUFFER_SIZE);
  //  printfBuffer_.PushBackString(string);

  //  return SendBuffer(printfBuffer_);
  //} // void SendPrintf(const char * string)

  static ReturnCode SendFiducialDetection(const Array<u8> &image, const FixedLengthList<VisionMarker> &markers, MemoryStack ccmScratch, MemoryStack onchipScratch, MemoryStack offchipScratch)
  {
    const f32 curTime = GetTime();

    // lastBenchmarkTime_algorithmsOnly is set again when the transmission is complete
    lastBenchmarkDuration_algorithmsOnly = curTime - lastBenchmarkTime_algorithmsOnly;

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

    const s32 height = CameraModeInfo[debugStreamResolution_].height;
    const s32 width  = CameraModeInfo[debugStreamResolution_].width;

    Array<u8> imageSmall(height, width, offchipScratch);
    DownsampleHelper(image, imageSmall, ccmScratch);
    debugStreamBuffer_.PushBack(imageSmall);

    return SendBuffer(debugStreamBuffer_);
  } // ReturnCode SendDebugStream_Detection()

  static ReturnCode SendTrackingUpdate(const Array<u8> &image, const Transformations::PlanarTransformation_f32 &transformation, MemoryStack ccmScratch, MemoryStack onchipScratch, MemoryStack offchipScratch)
  {
    const f32 curTime = GetTime();

    // lastBenchmarkTime_algorithmsOnly is set again when the transmission is complete
    lastBenchmarkDuration_algorithmsOnly = curTime - lastBenchmarkTime_algorithmsOnly;

    const s32 height = CameraModeInfo[debugStreamResolution_].height;
    const s32 width  = CameraModeInfo[debugStreamResolution_].width;

    // TODO: compute max allocation correctly
    const s32 requiredBytes = height*width + 1024;

    /*if(ccmScratch.ComputeLargestPossibleAllocation() >= requiredBytes) {
    void * buffer = ccmScratch.Allocate(requiredBytes);
    debugStreamBuffer_ = SerializedBuffer(buffer, requiredBytes);
    } else */
    if(onchipScratch.ComputeLargestPossibleAllocation() >= requiredBytes) {
      void * buffer = onchipScratch.Allocate(requiredBytes);
      debugStreamBuffer_ = SerializedBuffer(buffer, requiredBytes);
    } else {
      debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
    }

    // TODO: get the true length
    const s32 oneTransformationLength = 512;
    void * restrict oneTransformation = ccmScratch.Allocate(oneTransformationLength);

    transformation.Serialize(oneTransformation, oneTransformationLength);

    debugStreamBuffer_.PushBack("PlanarTransformation_f32", oneTransformation, oneTransformationLength);

    Array<u8> imageSmall(height, width, offchipScratch);
    DownsampleHelper(image, imageSmall, ccmScratch);
    debugStreamBuffer_.PushBack(imageSmall);

    return SendBuffer(debugStreamBuffer_);
  } // static ReturnCode SendTrackingUpdate()

  //  static ReturnCode SendArray(const Array<u8> &array)
  //  {
  //    debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
  //    debugStreamBuffer_.PushBack(array);
  //    return SendBuffer(debugStreamBuffer_);
  //  }
#endif // #ifdef SEND_DEBUG_STREAM

  static ReturnCode Initialize()
  {
    // TODO: add the rest
    debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
    printfBuffer_ = SerializedBuffer(&printfBufferRaw_[0], PRINTF_BUFFER_SIZE);

    return EXIT_SUCCESS;
  }
} // namespace DebugStream

#if 0
#pragma mark --- SimulatorParameters ---
#endif

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

#if 0
#pragma mark --- MatlabVisualization ---
#endif

namespace MatlabVisualization
{
#if !USE_MATLAB_VISUALIZATION
  // Stubs (no-ops) that do nothing when visualization is disabled.

  static ReturnCode Initialize() { return EXIT_SUCCESS; }

  static ReturnCode ResetFiducialDetection(const Array<u8>& image) { return EXIT_SUCCESS; }

  static ReturnCode SendFiducialDetection(const Quadrilateral<s16> &corners,
    const Vision::MarkerType &markerType) { return EXIT_SUCCESS; }

  static ReturnCode SendDrawNow() { return EXIT_SUCCESS; }

  static ReturnCode SendTrackInit(const Array<u8> &image,
    const Tracker& tracker,
    MemoryStack scratch) { return EXIT_SUCCESS; }

  static ReturnCode SendTrack(const Array<u8>& image,
    const Tracker& tracker,
    const bool converged,
    MemoryStack scratch)  { return EXIT_SUCCESS; }
#else
  static Matlab matlabViz_;

  static ReturnCode Initialize()
  {
    //matlabViz_ = Matlab(false);
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

  static ReturnCode ResetFiducialDetection(const Array<u8>& image)
  {
    matlabViz_.EvalStringEcho("delete(findobj(h_axes, 'Tag', 'DetectedQuad'));");
    matlabViz_.PutArray(image, "detectionImage");
    matlabViz_.EvalStringEcho("set(h_img, 'CData', detectionImage); "
      "set(h_axes, 'XLim', [.5 size(detectionImage,2)+.5], "
      "            'YLim', [.5 size(detectionImage,1)+.5]);");

    return EXIT_SUCCESS;
  }

  static ReturnCode SendFiducialDetection(const Quadrilateral<s16> &corners,
    const Vision::MarkerType &markerCode )
  {
    matlabViz_.PutQuad(corners, "detectedQuad");
    matlabViz_.EvalStringEcho("plot(detectedQuad([1 2 4 3 1],1)+1, "
      "     detectedQuad([1 2 4 3 1],2)+1, "
      "     'r', 'LineWidth', 2, "
      "     'Parent', h_axes, "
      "     'Tag', 'DetectedQuad'); "
      "plot(detectedQuad([1 3],1)+1, "
      "     detectedQuad([1 3],2)+1, "
      "     'g', 'LineWidth', 2, "
      "     'Parent', h_axes, "
      "     'Tag', 'DetectedQuad'); "
      "text(mean(detectedQuad(:,1))+1, "
      "     mean(detectedQuad(:,2))+1, "
      "     '%s', 'Hor', 'c', 'Color', 'y', "
      "     'FontSize', 16, 'FontWeight', 'b', "
      "     'Interpreter', 'none', "
      "     'Tag', 'DetectedQuad');",
      Vision::MarkerTypeStrings[markerCode]);

    return EXIT_SUCCESS;
  }

  static ReturnCode SendDrawNow()
  {
    matlabViz_.EvalString("drawnow");

    return EXIT_SUCCESS;
  }

  static ReturnCode SendTrackInit(const Array<u8> &image,
    const Tracker& tracker,
    MemoryStack scratch)
  {
    ResetFiducialDetection(image);

    matlabViz_.PutQuad(tracker.get_transformation().get_transformedCorners(scratch), "templateQuad");

    matlabViz_.EvalStringEcho("h_template = axes('Pos', [0 0 .33 .33], 'Tag', 'TemplateAxes'); "
      "imagesc(detectionImage, 'Parent', h_template); hold on; "
      "plot(templateQuad([1 2 4 3 1],1), "
      "     templateQuad([1 2 4 3 1],2), 'r', "
      "     'LineWidth', 2, "
      "     'Parent', h_template); "
      "set(h_template, 'XLim', [0.9*min(templateQuad(:,1)) 1.1*max(templateQuad(:,1))], "
      "                'YLim', [0.9*min(templateQuad(:,2)) 1.1*max(templateQuad(:,2))]);");
    return EXIT_SUCCESS;
  }

  static ReturnCode SendTrack(const Array<u8>& image,
    const Tracker& tracker,
    const bool converged,
    MemoryStack scratch)
  {
    matlabViz_.PutArray(image, "trackingImage");
    //            matlabViz_.EvalStringExplicit("imwrite(trackingImage, "
    //                                          "sprintf('~/temp/trackingImage%.3d.png', imageCtr)); "
    //                                          "imageCtr = imageCtr + 1;");
    matlabViz_.EvalStringEcho("set(h_img, 'CData', trackingImage); "
      "set(h_axes, 'XLim', [.5 size(trackingImage,2)+.5], "
      "            'YLim', [.5 size(trackingImage,1)+.5]);");

    if(converged)
    {
      matlabViz_.PutQuad(tracker.get_transformation().get_transformedCorners(scratch), "transformedQuad");
      matlabViz_.EvalStringEcho("set(h_trackedQuad, 'Visible', 'on', "
        "    'XData', transformedQuad([1 2 4 3 1],1)+1, "
        "    'YData', transformedQuad([1 2 4 3 1],2)+1); "
        "title(h_axes, 'Tracking Succeeded');");
    }
    else
    {
      matlabViz_.EvalStringEcho("set(h_trackedQuad, 'Visible', 'off'); "
        "title(h_axes, 'Tracking Failed'); "
        "delete(findobj(0, 'Tag', 'TemplateAxes'));");
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

#if 0
#pragma mark --- VisionState ---
#endif

namespace VisionState {
  static const s32 MAX_TRACKING_FAILURES = 5;

  static const Anki::Cozmo::HAL::CameraInfo* headCamInfo_;

  static VisionSystemMode mode_;

  static Anki::Vision::MarkerType markerTypeToTrack_;
  static Quadrilateral<f32> trackingQuad_;
  static s32 numTrackFailures_ ; //< The tracker can fail to converge this many times before we give up and reset the docker
  static bool isTrackingMarkerSpecified_;
  static Tracker tracker_;
  static f32 trackingMarkerWidth_mm;

  static bool wasCalledOnce_, havePreviousRobotState_;
  static Messages::RobotState robotState_, prevRobotState_;

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
    trackingMarkerWidth_mm = 0;

    wasCalledOnce_ = false;
    havePreviousRobotState_ = false;

#ifdef RUN_SIMPLE_TRACKING_TEST
    Anki::Cozmo::VisionSystem::SetMarkerToTrack(Vision::MARKER_BATTERIES,
      DEFAULT_BLOCK_MARKER_WIDTH_MM);
#endif

    headCamInfo_ = HAL::GetHeadCamInfo();
    if(headCamInfo_ == NULL) {
      PRINT("VisionState::Initialize() - HeadCam Info pointer is NULL!\n");
      return EXIT_FAILURE;
    }

    //// Compute the resolution of the mat camera from its FOV and height off the mat:
    //f32 matCamHeightInPix = ((static_cast<f32>(matCamInfo_->nrows)*.5f) /
    //  tanf(matCamInfo_->fov_ver * .5f));
    //matCamPixPerMM_ = matCamHeightInPix / MAT_CAM_HEIGHT_FROM_GROUND_MM;

    return EXIT_SUCCESS;
  } // VisionState::Initialize()

  static ReturnCode UpdateRobotState(const Messages::RobotState newRobotState)
  {
    prevRobotState_ = robotState_;
    robotState_     = newRobotState;

    if(wasCalledOnce_) {
      havePreviousRobotState_ = true;
    } else {
      wasCalledOnce_ = true;
    }

    return EXIT_SUCCESS;
  } // VisionState::UpdateRobotState()

  static Radians GetHeadingChange()
  {
    AnkiAssert(havePreviousRobotState_);

    return robotState_.pose_angle - prevRobotState_.pose_angle;
  }
} // namespace VisionState

#if 0
#pragma mark --- Function Implementations ---
#endif

static u32 DownsampleHelper(const Array<u8>& in,
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

  return downsampleFactor;
}

static ReturnCode LookForMarkers(
  const Array<u8> &grayscaleImage,
  const DetectFiducialMarkersParameters &parameters,
  FixedLengthList<VisionMarker> &markers,
  MemoryStack offchipScratch,
  MemoryStack onchipScratch,
  MemoryStack ccmScratch)
{
  AnkiAssert(parameters.isInitialized);

  const s32 maxMarkers = markers.get_maximumSize();

  FixedLengthList<Array<f32> > homographies(maxMarkers, ccmScratch);

  markers.set_size(maxMarkers);
  homographies.set_size(maxMarkers);

  for(s32 i=0; i<maxMarkers; i++) {
    Array<f32> newArray(3, 3, ccmScratch);
    homographies[i] = newArray;
  }

  MatlabVisualization::ResetFiducialDetection(grayscaleImage);

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
    DebugStream::SendFiducialDetection(grayscaleImage, markers, ccmScratch, onchipScratch, offchipScratch);

    for(s32 i_marker = 0; i_marker < markers.get_size(); ++i_marker) {
      const VisionMarker crntMarker = markers[i_marker];

      MatlabVisualization::SendFiducialDetection(crntMarker.corners, crntMarker.markerType);
    }

    MatlabVisualization::SendDrawNow();
  } // if(result == RESULT_OK)

  return EXIT_SUCCESS;
} // LookForMarkers()

static ReturnCode InitTemplate(
  const Array<u8> &grayscaleImage,
  const Quadrilateral<f32> &trackingQuad,
  const TrackerParameters &parameters,
  Tracker &tracker,
  MemoryStack offchipScratch,
  MemoryStack &onchipScratch, //< NOTE: onchip is a reference
  MemoryStack ccmScratch)
{
  AnkiAssert(parameters.isInitialized);

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
  // TODO: At some point template initialization should happen at full detection resolution but for
  //       now, we have to downsample to tracking resolution

  Array<u8> grayscaleImageSmall(parameters.trackingImageHeight, parameters.trackingImageWidth, ccmScratch);
  u32 downsampleFactor = DownsampleHelper(grayscaleImage, grayscaleImageSmall, ccmScratch);

  AnkiAssert(downsampleFactor > 0);
  // Note that the templateRegion and the trackingQuad are both at DETECTION_RESOLUTION, not
  // necessarily the resolution of the frame.
  //const u32 downsampleFactor = parameters.detectionWidth / parameters.trackingImageWidth;
  //const u32 downsamplePower = Log2u32(downsampleFactor);

  /*Quadrilateral<f32> trackingQuadSmall;

  for(s32 i=0; i<4; ++i) {
  trackingQuadSmall[i].x = trackingQuad[i].x / downsampleFactor;
  trackingQuadSmall[i].y = trackingQuad[i].y / downsampleFactor;
  }*/

#endif // #if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW
  tracker = TemplateTracker::LucasKanadeTracker_Slow(
    grayscaleImageSmall,
    trackingQuad,
    parameters.numPyramidLevels,
    Transformations::TRANSFORM_TRANSLATION,
    0.0,
    onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
  tracker = TemplateTracker::LucasKanadeTracker_Affine(
    grayscaleImageSmall,
    trackingQuad,
    parameters.numPyramidLevels,
    Transformations::TRANSFORM_AFFINE,
    onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
  tracker = TemplateTracker::LucasKanadeTracker_Projective(
    grayscaleImageSmall,
    trackingQuad,
    parameters.numPyramidLevels,
    Transformations::TRANSFORM_PROJECTIVE,
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

  MatlabVisualization::SendTrackInit(grayscaleImage, tracker, onchipScratch);

  return EXIT_SUCCESS;
} // InitTemplate()

static ReturnCode TrackTemplate(
  const Array<u8> &grayscaleImage,
  const Quadrilateral<f32> &trackingQuad,
  const TrackerParameters &parameters,
  Tracker &tracker,
  bool &converged,
  MemoryStack offchipScratch,
  MemoryStack onchipScratch,
  MemoryStack ccmScratch)
{
  AnkiAssert(parameters.isInitialized);

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

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
  // TODO: At some point template initialization should happen at full detection resolution
  //       but for now, we have to downsample to tracking resolution
  Array<u8> grayscaleImageSmall(parameters.trackingImageHeight, parameters.trackingImageWidth, ccmScratch);
  DownsampleHelper(grayscaleImage, grayscaleImageSmall, ccmScratch);

  //DebugStream::SendArray(grayscaleImageSmall);
#endif

  converged = false;

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW
  const Result trackerResult = tracker.UpdateTrack(
    grayscaleImage,
    parameters.maxIterations,
    parameters.convergenceTolerance,
    parameters.useWeights,
    converged,
    onchipScratch);

#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
  const Result trackerResult = tracker.UpdateTrack(
    grayscaleImageSmall,
    parameters.maxIterations,
    parameters.convergenceTolerance,
    converged,
    onchipScratch);

  //tracker.get_transformation().Print("track");

#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
  const Result trackerResult = tracker.UpdateTrack(
    grayscaleImageSmall,
    parameters.maxIterations,
    parameters.convergenceTolerance,
    converged,
    onchipScratch);

  //tracker.get_transformation().Print("track");

#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
  s32 numMatches = -1;

  const Result trackerResult = tracker.UpdateTrack(
    grayscaleImage,
    parameters.edgeDetection_grayvalueThreshold,
    parameters.edgeDetection_minComponentWidth,
    parameters.edgeDetection_maxDetectionsPerType,
    parameters.edgeDetection_everyNLines,
    parameters.matching_maxTranslationDistance,
    parameters.matching_maxProjectiveDistance,
    parameters.verification_maxTranslationDistance,
    false,
    numMatches,
    ccmScratch, offchipScratch);

  //tracker.get_transformation().Print("track");

  const s32 numTemplatePixels = tracker.get_numTemplatePixels();

  const f32 percentMatchedPixels = static_cast<f32>(numMatches) / static_cast<f32>(numTemplatePixels);

  if(percentMatchedPixels >= parameters.percentMatchedPixelsThreshold) {
    converged = true;
  }

#else
#error Unknown DOCKING_ALGORITHM!
#endif

  if(trackerResult != RESULT_OK) {
    return EXIT_FAILURE;
  }

  // Check for a super shrunk or super large template
  {
    // TODO: make not hacky
    const Array<f32> &homography = tracker.get_transformation().get_homography();

    const s32 numValues = 4;
    const s32 numMaxValues = 2;
    f32 values[numValues] = {ABS(homography[0][0]), ABS(homography[0][1]), ABS(homography[1][0]), ABS(homography[1][1])};
    s32 maxInds[numMaxValues] = {0, 1};
    for(s32 i=1; i<numValues; i++) {
      if(values[i] > values[maxInds[0]]) {
        maxInds[0] = i;
      }
    }

    for(s32 i=0; i<numValues; i++) {
      if(i == maxInds[0])
        continue;

      if(values[i] > values[maxInds[1]]) {
        maxInds[1] = i;
      }
    }

    const f32 secondValue = values[maxInds[1]];

    if(secondValue < 0.1f || secondValue > 40.0f) {
      converged = false;
    }
  }

  MatlabVisualization::SendTrack(grayscaleImage, tracker, converged, offchipScratch);

  DebugStream::SendTrackingUpdate(grayscaleImage, tracker.get_transformation(), ccmScratch, onchipScratch, offchipScratch);

  return EXIT_SUCCESS;
} // TrackTemplate()

//
// These functions below should be the only ones to access global variables in a different namespace
//
namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
      static DetectFiducialMarkersParameters detectionParameters_;
      static TrackerParameters               trackerParameters_;
      static HAL::CameraMode                 captureResolution_;

      ReturnCode Init()
      {
        ReturnCode result = EXIT_SUCCESS;

        if(!isInitialized_) {
          captureResolution_ = HAL::CAMERA_MODE_QVGA;

          // WARNING: the order of these initializations matter!

          // TODO: Figure out the intertwinedness

          // Do this one FIRST! this initializes headCamInfo_
          result = VisionState::Initialize();
          if(result != EXIT_SUCCESS) { return result; }

          result = VisionMemory::Initialize();
          if(result != EXIT_SUCCESS) { return result; }

          result = DebugStream::Initialize();
          if(result != EXIT_SUCCESS) { return result; }

          AnkiAssert(VisionState::headCamInfo_ != NULL);

          detectionParameters_.Initialize();
          trackerParameters_.Initialize();

          //result = DetectFiducialMarkersParameters::Initialize();
          //if(result != EXIT_SUCCESS) { return result; }

          //result = TrackerParameters::Initialize(detectionParameters_.detectionResolution,
          //                                       VisionState::headCamInfo_->focalLength_x);
          //if(result != EXIT_SUCCESS) { return result; }

          result = SimulatorParameters::Initialize();
          if(result != EXIT_SUCCESS) { return result; }

          result = MatlabVisualization::Initialize();
          if(result != EXIT_SUCCESS) { return result; }

          //result = Offboard::Initialize();
          //if(result != EXIT_SUCCESS) { return result; }

          AnkiAssert(detectionParameters_.isInitialized);
          AnkiAssert(trackerParameters_.isInitialized);

          isInitialized_ = true;
        }

        return result;
      }

      void Destroy()
      {
      }

      ReturnCode SetMarkerToTrack(const Vision::MarkerType& markerToTrack,
        const f32 markerWidth_mm)
      {
        VisionState::markerTypeToTrack_ = markerToTrack;
        VisionState::trackingMarkerWidth_mm = markerWidth_mm;
        VisionState::mode_ = VISION_MODE_LOOKING_FOR_MARKERS;
        VisionState::numTrackFailures_ = 0;

        // If the marker type is valid, start looking for it
        if(markerToTrack < Vision::NUM_MARKER_TYPES &&
          markerWidth_mm > 0.f)
        {
          VisionState::isTrackingMarkerSpecified_ = true;
        } else {
          VisionState::isTrackingMarkerSpecified_ = false;
        }

        return EXIT_SUCCESS;
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

      ReturnCode Update(const Messages::RobotState robotState)
      {
        // This should be called from elsewhere first, but calling it again won't hurt
        Init();

#ifdef SIMULATOR
        if (HAL::GetMicroCounter() < SimulatorParameters::frameRdyTimeUS_) {
          return EXIT_SUCCESS;
        }
#endif

        VisionState::UpdateRobotState(robotState);

        //#if USE_OFFBOARD_VISION
        //        return Update_Offboard();
        //#endif // USE_OFFBOARD_VISION

        const TimeStamp_t imageTimeStamp = HAL::GetTimeStamp();

        const f32 exposure = 0.1f;

        if(VisionState::mode_ == VISION_MODE_IDLE) {
          // Nothing to do!
        }
        else if(VisionState::mode_ == VISION_MODE_LOOKING_FOR_MARKERS) {
#ifdef SIMULATOR
          SimulatorParameters::frameRdyTimeUS_ = HAL::GetMicroCounter() + SimulatorParameters::LOOK_FOR_BLOCK_PERIOD_US;
#endif

          VisionMemory::ResetBuffers();

          MemoryStack offchipScratch_local(VisionMemory::offchipScratch_);

          const s32 captureHeight = CameraModeInfo[captureResolution_].height;
          const s32 captureWidth  = CameraModeInfo[captureResolution_].width;

          Array<u8> grayscaleImage(captureHeight, captureWidth,
            offchipScratch_local, Flags::Buffer(false,false,false));

          HAL::CameraGetFrame(reinterpret_cast<u8*>(grayscaleImage.get_rawDataPointer()),
            captureResolution_, exposure, false);

          const ReturnCode result = LookForMarkers(
            grayscaleImage,
            detectionParameters_,
            VisionMemory::markers_,
            offchipScratch_local,
            VisionMemory::onchipScratch_,
            VisionMemory::ccmScratch_);

          if(result != EXIT_SUCCESS) {
            return EXIT_FAILURE;
          }

          const s32 numMarkers = VisionMemory::markers_.get_size();
          bool isTrackingMarkerFound = false;
          for(s32 i_marker = 0; i_marker < numMarkers; ++i_marker)
          {
            const VisionMarker& crntMarker = VisionMemory::markers_[i_marker];

            // Create a vision marker message and process it (which just queues it
            // in the mailbox to be picked up and sent out by main execution)
            {
              Messages::VisionMarker msg;
              msg.timestamp  = imageTimeStamp;
              msg.markerType = crntMarker.markerType;

              msg.x_imgLowerLeft = crntMarker.corners[Quadrilateral<f32>::BottomLeft].x;
              msg.y_imgLowerLeft = crntMarker.corners[Quadrilateral<f32>::BottomLeft].y;

              msg.x_imgUpperLeft = crntMarker.corners[Quadrilateral<f32>::TopLeft].x;
              msg.y_imgUpperLeft = crntMarker.corners[Quadrilateral<f32>::TopLeft].y;

              msg.x_imgUpperRight = crntMarker.corners[Quadrilateral<f32>::TopRight].x;
              msg.y_imgUpperRight = crntMarker.corners[Quadrilateral<f32>::TopRight].y;

              msg.x_imgLowerRight = crntMarker.corners[Quadrilateral<f32>::BottomRight].x;
              msg.y_imgLowerRight = crntMarker.corners[Quadrilateral<f32>::BottomRight].y;

              Messages::ProcessVisionMarkerMessage(msg);
            }

            // Was the desired marker found? If so, start tracking it.
            if(VisionState::isTrackingMarkerSpecified_ && !isTrackingMarkerFound &&
              crntMarker.markerType == VisionState::markerTypeToTrack_)
            {
              // We will start tracking the _first_ marker of the right type that
              // we see.
              // TODO: Something smarter to track the one closest to the image center or to the expected location provided by the basestation?
              isTrackingMarkerFound = true;

              // I'd rather only initialize trackingQuad_ if InitTemplate() succeeds, but
              // InitTemplate downsamples it for the time being, since we're still doing template
              // initialization at tracking resolution instead of the eventual goal of doing it at
              // full detection resolution.
              VisionState::trackingQuad_ = Quadrilateral<f32>(
                Point<f32>(crntMarker.corners[0].x, crntMarker.corners[0].y),
                Point<f32>(crntMarker.corners[1].x, crntMarker.corners[1].y),
                Point<f32>(crntMarker.corners[2].x, crntMarker.corners[2].y),
                Point<f32>(crntMarker.corners[3].x, crntMarker.corners[3].y));

              //VisionState::trackingQuad_.Print();
              //printf("\n");

              const ReturnCode result = InitTemplate(
                grayscaleImage,
                VisionState::trackingQuad_,
                trackerParameters_,
                VisionState::tracker_,
                offchipScratch_local,
                VisionMemory::onchipScratch_, //< NOTE: onchip is a reference
                VisionMemory::ccmScratch_);

              if(result != EXIT_SUCCESS) {
                return EXIT_FAILURE;
              }

              // Template initialization succeeded, switch to tracking mode:
              // TODO: Log or issue message?
              VisionState::mode_ = VISION_MODE_TRACKING;
            } // if(isTrackingMarkerSpecified && !isTrackingMarkerFound && markerType == markerToTrack)
          } // for(each marker)
        } else if(VisionState::mode_ == VISION_MODE_TRACKING) {
#ifdef SIMULATOR
          SimulatorParameters::frameRdyTimeUS_ = HAL::GetMicroCounter() + SimulatorParameters::TRACK_BLOCK_PERIOD_US;
#endif

          //
          // Capture image for tracking
          //

          MemoryStack offchipScratch_local(VisionMemory::offchipScratch_);
          MemoryStack onchipScratch_local(VisionMemory::onchipScratch_);

          const s32 captureHeight = CameraModeInfo[captureResolution_].height;
          const s32 captureWidth  = CameraModeInfo[captureResolution_].width;

          Array<u8> grayscaleImage(captureHeight, captureWidth,
            onchipScratch_local, Flags::Buffer(false,false,false));

          HAL::CameraGetFrame(reinterpret_cast<u8*>(grayscaleImage.get_rawDataPointer()),
            captureResolution_, exposure, false);

          // Adjust the tracker transformation by approximately how much we
          // think we've moved (horizontally) since the last tracking call.
          // This is computed by comparing the rotational change angle to the FOV
          // angle of the image, and taking the same fraction of the dimension
          // of the image to get the amount of pixel movement.
          const Radians theta = VisionState::GetHeadingChange();
          f32 horizontalShift = static_cast<f32>(VisionState::headCamInfo_->nrows/2) * theta.ToFloat() / VisionState::headCamInfo_->fov_ver;

          Array<f32> update(1,2,onchipScratch_local);
          update[0][0] = -horizontalShift;
          update[0][1] = 0.0f;
          VisionState::tracker_.UpdateTransformation(update, 1.0f, onchipScratch_local, Transformations::TRANSFORM_TRANSLATION);

          /*const Transformations::PlanarTransformation_f32& oldTransformation = VisionState::tracker_.get_transformation();
          Array<f32> H = oldTransformation.get_homography();
          H[0][2] += horizontalShift; // Adjst the x-translation component of the homography
          //PRINT("Adjusting transformation by %.3f pixels for %.3fdeg rotation\n", horizontalShift, theta.getDegrees());
          Transformations::PlanarTransformation_f32 newTransformation(oldTransformation.get_transformType(),
          oldTransformation.get_initialCorners(),
          H, VisionMemory::onchipScratch_);
          VisionState::tracker_.set_transformation(newTransformation);*/

          //
          // Update the tracker transformation using this image
          //

          // Set by TrackTemplate() call
          bool converged = false;

          const ReturnCode result = TrackTemplate(
            grayscaleImage,
            VisionState::trackingQuad_,
            trackerParameters_,
            VisionState::tracker_,
            converged,
            offchipScratch_local,
            onchipScratch_local,
            VisionMemory::ccmScratch_);

          if(result != EXIT_SUCCESS) {
            PRINT("VisionSystem::Update(): TrackTemplate() failed.\n");
            return EXIT_FAILURE;
          }

          //
          // Create docking error signal from tracker
          //

          Messages::DockingErrorSignal dockErrMsg;
          dockErrMsg.timestamp = imageTimeStamp;
          dockErrMsg.didTrackingSucceed = static_cast<u8>(converged);

          if(converged)
          {
            Docking::ComputeDockingErrorSignal(VisionState::tracker_.get_transformation(),
              detectionParameters_.detectionWidth,
              VisionState::trackingMarkerWidth_mm,
              VisionState::headCamInfo_->focalLength_x,
              dockErrMsg.x_distErr,
              dockErrMsg.y_horErr,
              dockErrMsg.angleErr,
              VisionMemory::onchipScratch_);

            // Reset the failure counter
            VisionState::numTrackFailures_ = 0;
          }
          else {
            VisionState::numTrackFailures_ += 1;

            if(VisionState::numTrackFailures_ == VisionState::MAX_TRACKING_FAILURES) {
              // This resets docking, puttings us back in VISION_MODE_LOOKING_FOR_MARKERS mode
              SetMarkerToTrack(VisionState::markerTypeToTrack_,
                VisionState::trackingMarkerWidth_mm);
            }
          }

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
