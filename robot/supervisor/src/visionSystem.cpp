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

#define USE_MATLAB_TRACKER  0 
#define USE_MATLAB_DETECTOR 0


#if USE_MATLAB_VISUALIZATION || USE_MATLAB_TRACKER || USE_MATLAB_DETECTOR
#include "anki/common/robot/matlabInterface.h"
#endif

static bool isInitialized_ = false;

#ifdef THIS_IS_PETES_BOARD      
#define SEND_DEBUG_STREAM
#define RUN_SIMPLE_TRACKING_TEST
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
  f32 scaleTemplateRegionPercent;
  s32 numPyramidLevels;
  s32 maxIterations;
  f32 convergenceTolerance;
  bool useWeights;
  s32 numSamples;

  void Initialize()
  {
    //trackingResolution   = HAL::CAMERA_MODE_QQQVGA; // 80x60
    trackingResolution   = HAL::CAMERA_MODE_QQVGA; // 160x120

    trackingImageWidth   = CameraModeInfo[trackingResolution].width;
    trackingImageHeight  = CameraModeInfo[trackingResolution].height;
    scaleTemplateRegionPercent = 1.1f;    
    numPyramidLevels     = 3;
    maxIterations        = 25;
    //convergenceTolerance = 0.05f;
    convergenceTolerance = 1.f;
    useWeights           = true;
    numSamples           = 1000; // currently only used by Matlab

    isInitialized = true;
  } // TrackerParameters()

#else // #if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE

  bool isInitialized;
  HAL::CameraMode trackingResolution;
  s32 trackingImageHeight;
  s32 trackingImageWidth;
  f32 scaleTemplateRegionPercent;
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
    scaleTemplateRegionPercent = 1.1f;    
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

      /*
      void * restrict oneMarker = offchipScratch.Allocate(sizeof(VisionMarker));
      const s32 oneMarkerLength = sizeof(VisionMarker);

      for(s32 i=0; i<numMarkers; i++) {
        pMarkers[i].Serialize(oneMarker, oneMarkerLength);
        debugStreamBuffer_.PushBack("VisionMarker", oneMarker, oneMarkerLength);
      }
      */
      
      for(s32 i=0; i<numMarkers; i++) {
        pMarkers[i].Serialize(debugStreamBuffer_);
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

    if(onchipScratch.ComputeLargestPossibleAllocation() >= requiredBytes) {
      void * buffer = onchipScratch.Allocate(requiredBytes);
      debugStreamBuffer_ = SerializedBuffer(buffer, requiredBytes);
    } else {
      debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
    }

    //transformation.Print();

    transformation.Serialize(debugStreamBuffer_);

    Array<u8> imageSmall(height, width, offchipScratch);
    DownsampleHelper(image, imageSmall, ccmScratch);
    debugStreamBuffer_.PushBack(imageSmall);

    return SendBuffer(debugStreamBuffer_);
  } // static ReturnCode SendTrackingUpdate()

  
#if DOCKING_ALGORITHM ==  DOCKING_BINARY_TRACKER
  static ReturnCode SendBinaryTracker(const TemplateTracker::BinaryTracker &tracker, MemoryStack ccmScratch, MemoryStack onchipScratch, MemoryStack offchipScratch) 
  { 
    // TODO: compute max allocation correctly
    const s32 requiredBytes = 48000;

    if(onchipScratch.ComputeLargestPossibleAllocation() >= requiredBytes) {
      void * buffer = onchipScratch.Allocate(requiredBytes);
      debugStreamBuffer_ = SerializedBuffer(buffer, requiredBytes);
    } else {
      debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
    }
    
    tracker.Serialize(debugStreamBuffer_);
    
    return SendBuffer(debugStreamBuffer_);
  }
#endif
  
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
  
  static ReturnCode SendTrackerPrediction_Before(const Array<u8>& image,
                                                 const Tracker& tracker,
                                                 MemoryStack scratch) { return EXIT_SUCCESS; }
  
  static ReturnCode SendTrackerPrediction_After(const Tracker& tracker,
                                                MemoryStack scratch) { return EXIT_SUCCESS; }
  
  static ReturnCode SendTrackerPrediction_Compare(const Tracker& tracker,
                                                  MemoryStack scratch) { return EXIT_SUCCESS; }
  
#else
  static Matlab matlabViz_;
  static bool beforeCalled_;
  static const bool SHOW_TRACKER_PREDICTION = false;
  static const bool saveTrackingResults_ = false;
  
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
                              "imageCtr = 0; ");
    
    if(SHOW_TRACKER_PREDICTION) {
      matlabViz_.EvalStringEcho("h_fig_tform = figure('Name', 'TransformAdjust'); "
                                "colormap(h_fig_tform, gray);");
    }

    beforeCalled_ = false;
    
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

  static ReturnCode SendTrackInit(const Array<u8> &image, const Quadrilateral<f32>& quad)
  {
    ResetFiducialDetection(image);

    matlabViz_.PutQuad(quad, "templateQuad");

    matlabViz_.EvalStringEcho("h_template = axes('Pos', [0 0 .33 .33], 'Tag', 'TemplateAxes'); "
      "imagesc(detectionImage, 'Parent', h_template); hold on; "
      "plot(templateQuad([1 2 4 3 1],1), "
      "     templateQuad([1 2 4 3 1],2), 'r', "
      "     'LineWidth', 2, "
      "     'Parent', h_template); "
      "set(h_template, 'XLim', [0.9*min(templateQuad(:,1)) 1.1*max(templateQuad(:,1))], "
      "                'YLim', [0.9*min(templateQuad(:,2)) 1.1*max(templateQuad(:,2))]);");
    
    if(saveTrackingResults_) {
#if USE_MATLAB_TRACKER
      const char* fnameStr1 = "matlab";
#else // not matlab tracker:
      const char* fnameStr1 = "embedded";
#endif
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
      const char* fnameStr2 = "affine";
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
      const char* fnameStr2 = "projective";
#else
      const char* fnameStr2 = "unknown";
#endif
      
      matlabViz_.EvalStringEcho("saveDir = fullfile('~', 'temp', '%s', '%s'); "
                                "if ~isdir(saveDir), mkdir(saveDir); end, "
                                "fid = fopen(fullfile(saveDir, 'quads.txt'), 'wt'); saveCtr = 0; "
                                "fileCloser = onCleanup(@()fclose(fid)); "
                                "imwrite(detectionImage, fullfile(saveDir, sprintf('image_%%.5d.png', saveCtr))); "
                                "fprintf(fid, '[%%d] (%%f,%%f) (%%f,%%f) (%%f,%%f) (%%f,%%f)\\n', "
                                "        saveCtr, "
                                "        templateQuad(1,1), templateQuad(1,2), "
                                "        templateQuad(2,1), templateQuad(2,2), "
                                "        templateQuad(3,1), templateQuad(3,2), "
                                "        templateQuad(4,1), templateQuad(4,2));",
                                fnameStr1, fnameStr2);
    }
    
    
    return EXIT_SUCCESS;
  }
  
  static ReturnCode SendTrackInit(const Array<u8> &image,
                                  const Tracker& tracker,
                                  MemoryStack scratch)
  {
    return SendTrackInit(image, tracker.get_transformation().get_transformedCorners(scratch));
  }
  
  
  static ReturnCode SendTrack(const Array<u8>& image,
                              const Quadrilateral<f32>& quad,
                              const bool converged)
  {
    matlabViz_.PutArray(image, "trackingImage");
    matlabViz_.PutQuad(quad, "transformedQuad");
    
    //            matlabViz_.EvalStringExplicit("imwrite(trackingImage, "
    //                                          "sprintf('~/temp/trackingImage%.3d.png', imageCtr)); "
    //                                          "imageCtr = imageCtr + 1;");
    matlabViz_.EvalStringEcho("set(h_img, 'CData', trackingImage); "
                              "set(h_axes, 'XLim', [.5 size(trackingImage,2)+.5], "
                              "            'YLim', [.5 size(trackingImage,1)+.5]);"
                              "set(h_trackedQuad, 'Visible', 'on', "
                              "            'XData', transformedQuad([1 2 4 3 1],1)+1, "
                              "            'YData', transformedQuad([1 2 4 3 1],2)+1); ");
    
    if(saveTrackingResults_) {
      matlabViz_.EvalStringEcho("saveCtr = saveCtr + 1; "
                                "imwrite(trackingImage, fullfile(saveDir, sprintf('image_%%.5d.png', saveCtr))); "
                                "fprintf(fid, '[%%d] (%%f,%%f) (%%f,%%f) (%%f,%%f) (%%f,%%f)\\n', "
                                "        saveCtr, "
                                "        transformedQuad(1,1), transformedQuad(1,2), "
                                "        transformedQuad(2,1), transformedQuad(2,2), "
                                "        transformedQuad(3,1), transformedQuad(3,2), "
                                "        transformedQuad(4,1), transformedQuad(4,2));");
    }
    
    if(converged)
    {
      matlabViz_.EvalStringEcho("title(h_axes, 'Tracking Succeeded', 'FontSize', 16);");
    } else  {
      matlabViz_.EvalStringEcho( //"set(h_trackedQuad, 'Visible', 'off'); "
                                "title(h_axes, 'Tracking Failed', 'FontSize', 15); ");
      //        "delete(findobj(0, 'Tag', 'TemplateAxes'));");
      
      if(saveTrackingResults_) {
        matlabViz_.EvalStringEcho("fclose(fid);");
      }
    }
    
    matlabViz_.EvalString("drawnow");

    return EXIT_SUCCESS;
  }
  
  
  static ReturnCode SendTrack(const Array<u8>& image,
                              const Tracker& tracker,
                              const bool converged,
                              MemoryStack scratch)
  {
    return SendTrack(image, tracker.get_transformation().get_transformedCorners(scratch), converged);
  }
  
  
  static void SendTrackerPrediction_Helper(s32 subplotNum, const char *titleStr)
  {
    matlabViz_.EvalStringEcho("h = subplot(1,2,%d, 'Parent', h_fig_tform), "
                              "hold(h, 'off'), imagesc(img, 'Parent', h), axis(h, 'image'), hold(h, 'on'), "
                              "plot(quad([1 2 4 3 1],1), quad([1 2 4 3 1],2), 'r', 'LineWidth', 2, 'Parent', h); "
                              "title(h, '%s');", subplotNum, titleStr);
  }
  
  static ReturnCode SendTrackerPrediction_Before(const Array<u8>& image,
                                                 const Quadrilateral<f32>& quad)
  {
    if(SHOW_TRACKER_PREDICTION) {
      matlabViz_.PutArray(image, "img");
      matlabViz_.PutQuad(quad, "quad");
      SendTrackerPrediction_Helper(1, "Before Prediction");
      beforeCalled_ = true;
    }
    return EXIT_SUCCESS;
  }
  
  /*
  inline static ReturnCode SendTrackerPrediction_Before(const Array<u8>& image,
                                                        const Tracker& tracker,
                                                        MemoryStack scratch)
  {
    return SendTrackerPrediction_Before(image, tracker.get_transformation().get_transformedCorners(scratch));
  }
*/
  
  static ReturnCode SendTrackerPrediction_After(const Quadrilateral<f32>& quad)
  {
    if(SHOW_TRACKER_PREDICTION) {
      AnkiAssert(beforeCalled_ = true);
      matlabViz_.PutQuad(quad, "quad");
      SendTrackerPrediction_Helper(2, "After Prediction");
    }
    return EXIT_SUCCESS;
  }
  
  /*
  inline static ReturnCode SendTrackerPrediction_After(const Tracker& tracker, MemoryStack scratch)
  {
    return SendTrackerPrediction_After(tracker.get_transformation().get_transformedCorners(scratch));
  }
   */
  
  /*
  static ReturnCode SendTrackerPrediction_Compare(const Quadrilateral<f32>& quad)
  {
    if(SHOW_TRACKER_PREDICTION) {
      AnkiAssert(beforeCalled_ = true);
      matlabViz_.PutQuad(quad, "quad");
      SendTrackerPrediction_Helper(3, "After Tracking");
      beforeCalled_ = false;
    }
    return EXIT_SUCCESS;
  }
  
  inline static ReturnCode SendTrackerPrediction_Compare(const Tracker& tracker, MemoryStack scratch)
  {
    return SendTrackerPrediction_Compare(tracker.get_transformation().get_transformedCorners(scratch));
  }
   */
  
  
#endif //#if USE_MATLAB_VISUALIZATION
} // namespace MatlabVisualization


#if 0
#pragma mark --- MatlabVisionProcessor ---
#endif

namespace MatlabVisionProcessor {

#if USE_MATLAB_TRACKER || USE_MATLAB_DETECTOR
  
  static Matlab matlabProc_(false);
  static bool isInitialized_ = false;
  static bool haveTemplate_;
  static bool initTrackerAtFullRes_;
  
  static TrackerParameters trackerParameters_;
  
  static Transformations::TransformType transformType_ = Transformations::TRANSFORM_AFFINE;

  // Now coming from trackerParameters_
  //static HAL::CameraMode trackingResolution_;
  //static f32 scaleTemplateRegionPercent_;
  //static s32 maxIterations_;
  //static f32 convergenceTolerance_;
  
  static f32 errorTolerance_;
  static s32 scaleFactor_;
  
  ReturnCode Initialize()
  {
    if(!isInitialized_) {
      
      matlabProc_.EvalStringEcho("run(fullfile('..','..','..','..','matlab','initCozmoPath'));");

      trackerParameters_.Initialize();
      
      initTrackerAtFullRes_ = false;
      
      scaleFactor_ = (1<<CameraModeInfo[HAL::CAMERA_MODE_QVGA].downsamplePower[trackerParameters_.trackingResolution]);
      haveTemplate_ = false;
      

      //scaleTemplateRegionPercent_ = 0.1f;
      //maxIterations_ = 25;
      //convergenceTolerance_ = 1.f;
      
      errorTolerance_ = 0.5f;
      
      isInitialized_ = true;
    }
    
    return EXIT_SUCCESS;
  } // MatlabVisionProcess::Initialize()
  
  ReturnCode InitTemplate(const Array<u8>& imgFull,
                          const Quadrilateral<f32>& trackingQuad,
                          MemoryStack scratch)
  {
    if(!isInitialized_) {
      return EXIT_FAILURE;
    }
    
    matlabProc_.PutQuad(trackingQuad, "initTrackingQuad");
    
    if(initTrackerAtFullRes_ || (imgFull.get_size(0) == trackerParameters_.trackingImageHeight &&
                                 imgFull.get_size(1) == trackerParameters_.trackingImageWidth))
    {
      matlabProc_.PutArray(imgFull, "img");
    }
    else {
      Array<u8> imgSmall(trackerParameters_.trackingImageHeight,
                         trackerParameters_.trackingImageWidth,
                         scratch);
      
      DownsampleHelper(imgFull, imgSmall, scratch);
      matlabProc_.PutArray(imgSmall, "img");
      matlabProc_.EvalStringEcho("initTrackingQuad = initTrackingQuad / %d;",
                                 scaleFactor_);
    }
    
    matlabProc_.EvalStringEcho("LKtracker = LucasKanadeTracker(img, "
                               "  initTrackingQuad + 1, "
                               "  'Type', '%s', 'RidgeWeight', 0, "
                               "  'DebugDisplay', false, 'UseBlurring', false, "
                               "  'UseNormalization', false, "
                               "  'TrackingResolution', [%d %d], "
                               "  'TemplateRegionPaddingFraction', %f, "
                               "  'NumScales', %d, "
                               "  'NumSamples', %d);",
                               (transformType_ == Transformations::TRANSFORM_AFFINE ? "affine" : "homography"),
                               trackerParameters_.trackingImageWidth,
                               trackerParameters_.trackingImageHeight,
                               trackerParameters_.scaleTemplateRegionPercent - 1.f,
                               trackerParameters_.numPyramidLevels,
                               trackerParameters_.numSamples);
    
    haveTemplate_ = true;
    
    MatlabVisualization::SendTrackInit(imgFull, trackingQuad);
    
    return EXIT_SUCCESS;
    
  } // MatlabVisionProcessor::InitTemplate()
  
  
  void UpdateTracker(const Array<f32>& predictionUpdate)
  {
    AnkiAssert(predictionUpdate.get_size(0)==1);
    
    switch(predictionUpdate.get_size(1))
    {
      case 2:
        AnkiAssert(transformType_ == Transformations::TRANSFORM_TRANSLATION);
        
        matlabProc_.EvalStringEcho("update = ([1 0 %f; "
                                   "           0 1 %f; "
                                   "           0 0  1]);",
                                   predictionUpdate[0][0], predictionUpdate[0][1]);
        break;
        
      case 6:
        AnkiAssert(transformType_ == Transformations::TRANSFORM_PROJECTIVE ||
                   transformType_ == Transformations::TRANSFORM_AFFINE);
        
        matlabProc_.EvalStringEcho("update = ([1+%f  %f   %f; "
                                   "            %f  1+%f  %f; "
                                   "             0    0    1]);",
                                   predictionUpdate[0][0], predictionUpdate[0][1],
                                   predictionUpdate[0][2], predictionUpdate[0][3],
                                   predictionUpdate[0][4], predictionUpdate[0][5]);
        break;
        
      default:
        AnkiError("MatlabVisionProcess::UpdateTracker",
                  "Unrecognized tracker transformation update size (%d vs. 2 or 6)",
                  predictionUpdate.get_size(1));
    } // switch
    
    matlabProc_.EvalStringEcho("S = [%d 0 0; 0 %d 0; 0 0 1]; "
                               "transform = S*double(LKtracker.tform)*inv(S); "
                               "newTform = transform / update; " // transform * inv(update)
                               "newTform = inv(S) * newTform * S; "
                               "newTform = newTform / newTform(3,3); "
                               "LKtracker.set_tform(newTform);",
                               scaleFactor_, scaleFactor_);
    
  } // MatlabVisionProcess::UpdateTracker()
  
  ReturnCode TrackTemplate(const Array<u8>& imgFull, bool& converged, MemoryStack scratch)
  {
    if(!haveTemplate_) {
      AnkiWarn("MatlabVisionProcess::TrackTemplate",
               "TrackTemplate called before tracker initialized.");
      return EXIT_FAILURE;
    }
    
    if(imgFull.get_size(0) == trackerParameters_.trackingImageHeight &&
       imgFull.get_size(1) == trackerParameters_.trackingImageWidth)
    {
      matlabProc_.PutArray(imgFull, "img");
    }
    else {
      Array<u8> img(trackerParameters_.trackingImageHeight,
                    trackerParameters_.trackingImageWidth,
                    scratch);
      
      DownsampleHelper(imgFull, img, scratch);
      matlabProc_.PutArray(img, "img");
    }
    
    matlabProc_.EvalStringEcho(//"desktop, keyboard; "
                               "converged = LKtracker.track(img, "
                               "   'MaxIterations', %d, "
                               "   'ConvergenceTolerance', %f, "
                               "   'ErrorTolerance', %f); "
                               "corners = %d*(double(LKtracker.corners) - 1);",
                               trackerParameters_.maxIterations,
                               trackerParameters_.convergenceTolerance,
                               errorTolerance_,
                               scaleFactor_);
    
    converged = mxIsLogicalScalarTrue(matlabProc_.GetArray("converged"));
    
    Quadrilateral<f32> quad = matlabProc_.GetQuad<f32>("corners");
    
    MatlabVisualization::SendTrack(imgFull, quad, converged);
    
    return EXIT_SUCCESS;
    
  } // MatlabVisionProcessor::TrackTemplate()
  
  
  Transformations::PlanarTransformation_f32 GetTrackerTransform(MemoryStack& memory)
  {
    Array<f32> homography = Array<f32>(3,3,memory);
    
    matlabProc_.EvalStringEcho("S = [%d 0 0; 0 %d 0; 0 0 1]; "
                               "transform = S*double(LKtracker.tform)*inv(S); "
                               "transform = transform / transform(3,3); "
                               "corners = %d*(double(LKtracker.corners) - 1); "
                               "xcen = %d*(double(LKtracker.xcen) - 1); "
                               "ycen = %d*(double(LKtracker.ycen) - 1);",
                               scaleFactor_, scaleFactor_, scaleFactor_, scaleFactor_, scaleFactor_);
    
    Quadrilateral<f32> quad = matlabProc_.GetQuad<f32>("corners");
    const mxArray* mxTform = matlabProc_.GetArray("transform");
    
    AnkiAssert(mxTform != NULL && mxGetM(mxTform) == 3 && mxGetN(mxTform)==3);
    
    const double* mxTformData = mxGetPr(mxTform);
    
    homography[0][0] = mxTformData[0];
    homography[1][0] = mxTformData[1];
    homography[2][0] = mxTformData[2];
    
    homography[0][1] = mxTformData[3];
    homography[1][1] = mxTformData[4];
    homography[2][1] = mxTformData[5];
    
    homography[0][2] = mxTformData[6];
    homography[1][2] = mxTformData[7];
    homography[2][2] = mxTformData[8];
    
    const Point2f centerOffset(mxGetScalar(matlabProc_.GetArray("xcen")),
                               mxGetScalar(matlabProc_.GetArray("ycen")) );
    
    return Transformations::PlanarTransformation_f32(transformType_, quad, homography, centerOffset, memory);
    
  } // MatlabVisionProcessor::GetTrackerTransform()
  
#else
  
  ReturnCode Initialize() { return EXIT_SUCCESS; }
  
#endif // USE_MATLAB_TRACKER || USE_MATLAB_DETECTOR
  
} // namespace MatlabVisionProcessor



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
  static const s32 MAX_TRACKING_FAILURES = 1;

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
    parameters.scaleTemplateRegionPercent,
    parameters.numPyramidLevels,
    Transformations::TRANSFORM_TRANSLATION,
    0.0,
    onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
  tracker = TemplateTracker::LucasKanadeTracker_Affine(
    grayscaleImageSmall,
    trackingQuad,
    parameters.scaleTemplateRegionPercent,
    parameters.numPyramidLevels,
    Transformations::TRANSFORM_AFFINE,
    onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
  tracker = TemplateTracker::LucasKanadeTracker_Projective(
    grayscaleImageSmall,
    trackingQuad,
    parameters.scaleTemplateRegionPercent,
    parameters.numPyramidLevels,
    Transformations::TRANSFORM_PROJECTIVE,
    onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
  tracker = TemplateTracker::BinaryTracker(
    grayscaleImage,
    trackingQuad,
    parameters.scaleTemplateRegionPercent,
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

  
#if DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER  
  DebugStream::SendBinaryTracker(tracker, ccmScratch, onchipScratch, offchipScratch);
#endif

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
  
  //MatlabVisualization::SendTrackerPrediction_Compare(tracker, offchipScratch);
  
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
          
          result = MatlabVisionProcessor::Initialize();
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

      
      //
      // Tracker Prediction
      //
      // Adjust the tracker transformation by approximately how much we
      // think we've moved since the last tracking call.
      //
      ReturnCode TrackerPredictionUpdate(const Array<u8>& grayscaleImage, MemoryStack scratch)
      {
        // Get the observed vertical size of the marker
#if USE_MATLAB_TRACKER
        // TODO: Tidy this up
        MatlabVisionProcessor::matlabProc_.EvalStringEcho("currentQuad = %d*(double(LKtracker.corners)-1);", MatlabVisionProcessor::scaleFactor_);
        const Quadrilateral<f32> currentQuad = MatlabVisionProcessor::matlabProc_.GetQuad<f32>("currentQuad");
#else
        const Quadrilateral<f32> currentQuad = VisionState::tracker_.get_transformation().get_transformedCorners(scratch);
#endif
        
        MatlabVisualization::SendTrackerPrediction_Before(grayscaleImage, currentQuad);
        
        const Quadrilateral<f32> sortedQuad  = currentQuad.ComputeClockwiseCorners();
        
        f32 dx = sortedQuad[3].x - sortedQuad[0].x;
        f32 dy = sortedQuad[3].y - sortedQuad[0].y;
        const f32 observedVerticalSize_pix = sqrtf( dx*dx + dy*dy );
        
        // Compare observed vertical size to actual block marker size (projected
        // to be orthogonal to optical axis, using head angle) to approximate the
        // distance to the marker along the camera's optical axis
        const f32 cosHeadAngle = cosf(VisionState::robotState_.headAngle);
        const f32 sinHeadAngle = sinf(VisionState::robotState_.headAngle);
        const f32 d = (VisionState::trackingMarkerWidth_mm* cosHeadAngle *
                       VisionState::headCamInfo_->focalLength_y /
                       observedVerticalSize_pix);
        
        // Ask VisionState how much we've moved since last call (in robot coordinates)
        Radians theta;
        f32 T_fwd_robot, T_hor_robot;
        VisionState::GetPoseChange(T_fwd_robot, T_hor_robot, theta);

        // Convert to how much we've moved along (and orthogonal to) the camera's optical axis
        const f32 T_fwd_cam =  T_fwd_robot*cosHeadAngle;
        const f32 T_ver_cam = -T_fwd_robot*sinHeadAngle;

        // Predict approximate horizontal shift from two things:
        // 1. The rotation fo the robot
        //    Compute pixel-per-degree of the camera and multiply by degrees rotated
        // 2. Convert horizontal shift of the robot to pixel shift, using
        //    focal length
        f32 horizontalShift_pix = (static_cast<f32>(VisionState::headCamInfo_->nrows/2) * theta.ToFloat() /
                                   VisionState::headCamInfo_->fov_ver) + (T_hor_robot*VisionState::headCamInfo_->focalLength_x/d);
        
        // Predict approximate scale change by comparing the distance to the
        // object before and after forward motion
        const f32 scaleChange = d / (d - T_fwd_cam);
        
        // Predict approximate vertical shift in the camera plane by comparing
        // vertical motion (orthogonal to camera's optical axis) to the focal
        // length
        const f32 verticalShift_pix = T_ver_cam * VisionState::headCamInfo_->focalLength_y/d;
        
        PRINT("Adjusting transformation: %.3fpix H shift for %.3fdeg rotation, "
              "%.3f scaling and %.3f V shift for %.3f translation forward (%.3f cam)\n",
              horizontalShift_pix, theta.getDegrees(), scaleChange,
              verticalShift_pix, T_fwd_robot, T_fwd_cam);
        
        // Adjust the Transformation
        // Note: UpdateTransformation is doing *inverse* composition (thus using the negatives)
        if(VisionState::tracker_.get_transformation().get_transformType() == Transformations::TRANSFORM_TRANSLATION) {
          Array<f32> update(1,2,scratch);
          update[0][0] = -horizontalShift_pix;
          update[0][1] = -verticalShift_pix;
          
#if USE_MATLAB_TRACKER
          MatlabVisionProcessor::UpdateTracker(update);
#else
          VisionState::tracker_.UpdateTransformation(update, 1.f, scratch,
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
          VisionState::tracker_.UpdateTransformation(update, 1.f, scratch,
                                                     Transformations::TRANSFORM_AFFINE);
#endif
        } // if(tracker transformation type == TRANSLATION...)
        
#if USE_MATLAB_TRACKER
        // TODO: Tidy this up
        MatlabVisionProcessor::matlabProc_.EvalStringEcho("predictedQuad = %d*(double(LKtracker.corners)-1);", MatlabVisionProcessor::scaleFactor_);
        const Quadrilateral<f32> predictedQuad = MatlabVisionProcessor::matlabProc_.GetQuad<f32>("predictedQuad");
#else
        const Quadrilateral<f32> predictedQuad = VisionState::tracker_.get_transformation().get_transformedCorners(scratch);
#endif

        MatlabVisualization::SendTrackerPrediction_After(predictedQuad);
        
        return EXIT_SUCCESS;
        
      } // TrackerPredictionUpdate()
      
      
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

              
#if USE_MATLAB_TRACKER
              const ReturnCode result = MatlabVisionProcessor::InitTemplate(grayscaleImage,
                                                                            VisionState::trackingQuad_,
                                                                            VisionMemory::ccmScratch_);
#else
              const ReturnCode result = InitTemplate(grayscaleImage,
                                                     VisionState::trackingQuad_,
                                                     trackerParameters_,
                                                     VisionState::tracker_,
                                                     offchipScratch_local,
                                                     VisionMemory::onchipScratch_, //< NOTE: onchip is a reference
                                                     VisionMemory::ccmScratch_);
#endif
              
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

          //
          // Tracker Prediction
          //
          // Adjust the tracker transformation by approximately how much we
          // think we've moved since the last tracking call.
          //
          
          ReturnCode predictionResult = TrackerPredictionUpdate(grayscaleImage, onchipScratch_local);
          
          if(predictionResult != EXIT_SUCCESS) {
            PRINT("VisionSystem::Update(): TrackTemplate() failed.\n");
            return EXIT_FAILURE;
          }
          

          //
          // Update the tracker transformation using this image
          //

          // Set by TrackTemplate() call
          bool converged = false;

          
#if USE_MATLAB_TRACKER
          
          const ReturnCode trackResult = MatlabVisionProcessor::TrackTemplate(grayscaleImage, converged, VisionMemory::ccmScratch_);
          
#else
          const ReturnCode trackResult = TrackTemplate(
            grayscaleImage,
            VisionState::trackingQuad_,
            trackerParameters_,
            VisionState::tracker_,
            converged,
            offchipScratch_local,
            onchipScratch_local,
            VisionMemory::ccmScratch_);
#endif
          
          if(trackResult != EXIT_SUCCESS) {
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
            
#if USE_MATLAB_TRACKER
            Transformations::PlanarTransformation_f32 transform = MatlabVisionProcessor::GetTrackerTransform(VisionMemory::onchipScratch_);
#else
            const Transformations::PlanarTransformation_f32& transform = VisionState::tracker_.get_transformation();
#endif
            
            Docking::ComputeDockingErrorSignal(transform,
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
        } // if(converged)

        return EXIT_SUCCESS;
      } // Update()
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki
