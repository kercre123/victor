#include "anki/common/robot/config.h"
#include "anki/common/robot/serialize.h"
#include "anki/common/robot/benchmarking.h"

#include "anki/cozmo/robot/hal.h"
#include "visionSystem.h"

#include "visionDebugStream.h"

#if !defined(SEND_DEBUG_STREAM)
#error SEND_DEBUG_STREAM should be defined somewhere.
#endif

namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
      namespace DebugStream
      {
        namespace { // private namespace for all state

          SerializedBuffer debugStreamBuffer_;
          
#if SEND_DEBUG_STREAM
          const s32 DEBUG_STREAM_BUFFER_SIZE = 1000000;
          const s32 MAX_BYTES_PER_SECOND = 500000;
          const s32 SEND_EVERY_N_FRAMES = 1;

          OFFCHIP u8 debugStreamBufferData_[DEBUG_STREAM_BUFFER_SIZE];

          s32 lastSecond;
          s32 bytesSinceLastSecond;

          s32 frameNumber = 0;

          static u32 computeBenchmarkResults_start;
          static u32 computeBenchmarkResults_end;

          const Vision::CameraResolution debugStreamResolution_ = Vision::CAMERA_RES_QQQVGA;
#endif
        } // private namespace

        Result SendBuffer(SerializedBuffer &toSend, MemoryStack scratch)
        {
#if SEND_DEBUG_STREAM
          const f32 curSecond = Round<s32>(GetTimeF32());

          // Hack, to prevent overwhelming the send buffer
          if(curSecond != lastSecond) {
            bytesSinceLastSecond = 0;
            lastSecond = curSecond;
          } else {
            if(bytesSinceLastSecond > MAX_BYTES_PER_SECOND) {
              return RESULT_OK;
            } else {
              bytesSinceLastSecond += toSend.get_memoryStack().get_usedBytes();
            }
          }

          /*for(s32 i=0; i<500; i++) {
            BeginBenchmark("Test");
            EndBenchmark("Test");
          }*/
          
          EndBenchmark("TotalTime");

          const u32 lastComputeBenchmarkResults_elapsedTime = computeBenchmarkResults_end - computeBenchmarkResults_start;

          computeBenchmarkResults_start = GetTimeU32();

          FixedLengthList<BenchmarkElement> benchmarks = ComputeBenchmarkResults(scratch);

          const s32 totalTimeIndex = GetNameIndex("TotalTime", benchmarks);
          if(totalTimeIndex >= 0) {
            // Hack to add the time for computing the benchmark to the compiled times
            if(benchmarks.get_size() < benchmarks.get_maximumSize()) {
              BenchmarkElement newElement("ComputeBenchmarkResults");
              newElement.inclusive_mean = lastComputeBenchmarkResults_elapsedTime;
              newElement.inclusive_min = lastComputeBenchmarkResults_elapsedTime;
              newElement.inclusive_max = lastComputeBenchmarkResults_elapsedTime;
              newElement.inclusive_total = lastComputeBenchmarkResults_elapsedTime;
              newElement.exclusive_mean = lastComputeBenchmarkResults_elapsedTime;
              newElement.exclusive_min = lastComputeBenchmarkResults_elapsedTime;
              newElement.exclusive_max = lastComputeBenchmarkResults_elapsedTime;
              newElement.exclusive_total = lastComputeBenchmarkResults_elapsedTime;
              newElement.numEvents = 1;

              const s32 numBenchmarks = benchmarks.get_size();
              BenchmarkElement * restrict pBenchmarks = benchmarks.Pointer(0);

              pBenchmarks[totalTimeIndex].inclusive_total += lastComputeBenchmarkResults_elapsedTime;
              
              benchmarks.PushBack(newElement);
            }

            toSend.PushBack<BenchmarkElement>("Benchmarks", benchmarks);
          } // if(totalTimeIndex >= 0)

          InitBenchmarking();

          computeBenchmarkResults_end = GetTimeU32();

          BeginBenchmark("TotalTime");

          BeginBenchmark("UARTPutMessage");

          s32 startIndex;
          u8 * bufferStart = reinterpret_cast<u8*>(toSend.get_memoryStack().get_validBufferStart(startIndex));
          const s32 validUsedBytes = toSend.get_memoryStack().get_usedBytes() - startIndex;

          const s32 startDiff = static_cast<s32>( reinterpret_cast<size_t>(bufferStart) - reinterpret_cast<size_t>(toSend.get_memoryStack().get_buffer()) );
          const s32 endDiff = toSend.get_memoryStack().get_totalBytes() - toSend.get_memoryStack().get_usedBytes();

          if( (startDiff >= SERIALIZED_BUFFER_HEADER_LENGTH) && (endDiff >= SERIALIZED_BUFFER_FOOTER_LENGTH) ) {
            memcpy(bufferStart - SERIALIZED_BUFFER_HEADER_LENGTH, &Embedded::SERIALIZED_BUFFER_HEADER[0], SERIALIZED_BUFFER_HEADER_LENGTH);
            memcpy(bufferStart + validUsedBytes, &Embedded::SERIALIZED_BUFFER_FOOTER[0], SERIALIZED_BUFFER_FOOTER_LENGTH);

            Anki::Cozmo::HAL::UARTPutMessage(
              0,
              bufferStart - SERIALIZED_BUFFER_HEADER_LENGTH,
              validUsedBytes + SERIALIZED_BUFFER_HEADER_LENGTH + SERIALIZED_BUFFER_FOOTER_LENGTH);
          }

          EndBenchmark("UARTPutMessage");
#endif // SEND_DEBUG_STREAM

          return RESULT_OK;
        }

        Result SendBenchmarksOnly(MemoryStack scratch)
        {
          return SendBuffer(debugStreamBuffer_, scratch);
        }
        
        Result SendFiducialDetection(const Array<u8> &image, const FixedLengthList<VisionMarker> &markers, MemoryStack ccmScratch, MemoryStack onchipScratch, MemoryStack offchipScratch)
        {
          Result result = RESULT_OK;

#if SEND_DEBUG_STREAM
          debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferData_[0], DEBUG_STREAM_BUFFER_SIZE);

          if(markers.get_size() != 0) {
            const s32 numMarkers = markers.get_size();
            const VisionMarker * pMarkers = markers.Pointer(0);

            char objectName[64];
            for(s32 i=0; i<numMarkers; i++) {
              snprintf(objectName, 64, "Marker%d", i);
              pMarkers[i].Serialize(objectName, debugStreamBuffer_);
            }
          }

          const s32 height = CameraModeInfo[debugStreamResolution_].height;
          const s32 width  = CameraModeInfo[debugStreamResolution_].width;

          Array<u8> imageSmall(height, width, offchipScratch);
          DownsampleHelper(image, imageSmall, ccmScratch);
          debugStreamBuffer_.PushBack("Robot Image", imageSmall);

          result = SendBuffer(debugStreamBuffer_, offchipScratch);

          // The UART can't handle this at full rate, so wait a bit between each frame
          if(debugStreamResolution_ == Vision::CAMERA_RES_QVGA) {
            //HAL::MicroWait(1000000);
          }
#endif // #if SEND_DEBUG_STREAM

          return result;
        } // Result SendDebugStream_Detection()

        Result SendTrackingUpdate(
          const Array<u8> &image,
          const Tracker &tracker,
          const TrackerParameters &parameters,
          const u8 meanGrayvalueError,
          const f32 percentMatchingGrayvalues,
          MemoryStack ccmScratch,
          MemoryStack onchipScratch,
          MemoryStack offchipScratch)
        {
          Result result = RESULT_OK;

#if SEND_DEBUG_STREAM
          const s32 height = CameraModeInfo[debugStreamResolution_].height;
          const s32 width  = CameraModeInfo[debugStreamResolution_].width;

          debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferData_[0], DEBUG_STREAM_BUFFER_SIZE);

          tracker.get_transformation().Serialize("Transformation", debugStreamBuffer_);

          debugStreamBuffer_.PushBack<u8>("meanGrayvalueError", &meanGrayvalueError, 1);

          debugStreamBuffer_.PushBack<f32>("percentMatchingGrayvalues", &percentMatchingGrayvalues, 1);

          frameNumber++;

          if(frameNumber % SEND_EVERY_N_FRAMES != 0) {
            return RESULT_OK;
          }

#if DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
          EdgeLists edgeLists;

          edgeLists.imageHeight = image.get_size(0);
          edgeLists.imageWidth = image.get_size(1);

          edgeLists.xDecreasing = FixedLengthList<Point<s16> >(parameters.edgeDetectionParams_update.maxDetectionsPerType, offchipScratch);
          edgeLists.xIncreasing = FixedLengthList<Point<s16> >(parameters.edgeDetectionParams_update.maxDetectionsPerType, offchipScratch);
          edgeLists.yDecreasing = FixedLengthList<Point<s16> >(parameters.edgeDetectionParams_update.maxDetectionsPerType, offchipScratch);
          edgeLists.yIncreasing = FixedLengthList<Point<s16> >(parameters.edgeDetectionParams_update.maxDetectionsPerType, offchipScratch);

          if(parameters.edgeDetectionParams_update.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
            DetectBlurredEdges_GrayvalueThreshold(
              image,
              tracker.get_lastUsedGrayvalueThrehold(),
              parameters.edgeDetectionParams_update.minComponentWidth, parameters.edgeDetectionParams_update.everyNLines,
              edgeLists);
          } else {
            DetectBlurredEdges_DerivativeThreshold(
              image,
              parameters.edgeDetectionParams_update.combHalfWidth, parameters.edgeDetectionParams_update.combResponseThreshold, parameters.edgeDetectionParams_update.everyNLines,
              edgeLists);
          }

          edgeLists.Serialize("Edge List", debugStreamBuffer_);

          Array<u8> imageSmall(height, width, offchipScratch);
          DownsampleHelper(image, imageSmall, ccmScratch);
          debugStreamBuffer_.PushBack("Robot Image", imageSmall);
#else
          Array<u8> imageSmall(height, width, offchipScratch);
          DownsampleHelper(image, imageSmall, ccmScratch);
          debugStreamBuffer_.PushBack("Robot Image", imageSmall);
#endif

          result = SendBuffer(debugStreamBuffer_, offchipScratch);

          // The UART can't handle this at full rate, so wait a bit between each frame
          if(debugStreamResolution_ == Vision::CAMERA_RES_QVGA) {
            //HAL::MicroWait(1000000);
          }

#endif // #if SEND_DEBUG_STREAM

          return result;
        } // static Result SendTrackingUpdate()

        Result SendFaceDetections(
          const Array<u8> &image,
          const FixedLengthList<Rectangle<s32> > &detectedFaces,
          const s32 detectedFacesImageWidth,
          MemoryStack ccmScratch,
          MemoryStack onchipScratch,
          MemoryStack offchipScratch)
        {
          Result result = RESULT_OK;

#if SEND_DEBUG_STREAM
          const s32 height = CameraModeInfo[debugStreamResolution_].height;
          const s32 width  = CameraModeInfo[debugStreamResolution_].width;

          debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferData_[0], DEBUG_STREAM_BUFFER_SIZE);

          if(detectedFaces.get_size() > 0) {
            debugStreamBuffer_.PushBack<s32>("detectedFacesImageWidth", &detectedFacesImageWidth, 1);
            debugStreamBuffer_.PushBack<Rectangle<s32> >("detectedFaces", detectedFaces);
          }

          frameNumber++;

          Array<u8> imageSmall(height, width, offchipScratch);
          DownsampleHelper(image, imageSmall, ccmScratch);
          debugStreamBuffer_.PushBack("Robot Image", imageSmall);

          result = SendBuffer(debugStreamBuffer_, offchipScratch);

          // The UART can't handle this at full rate, so wait a bit between each frame
          if(debugStreamResolution_ == Vision::CAMERA_RES_QVGA) {
            //HAL::MicroWait(1000000);
          }

#endif // #if SEND_DEBUG_STREAM

          return result;
        } // static Result SendFaceDetections()

#if DOCKING_ALGORITHM ==  DOCKING_BINARY_TRACKER
        Result SendBinaryTracker(const TemplateTracker::BinaryTracker &tracker, MemoryStack ccmScratch, MemoryStack onchipScratch, MemoryStack offchipScratch)
        {
#if SEND_DEBUG_STREAM

          // TODO: compute max allocation correctly
          const s32 requiredBytes = 48000;

          debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferData_[0], DEBUG_STREAM_BUFFER_SIZE);

          tracker.Serialize("Binary Tracker", debugStreamBuffer_);

          return SendBuffer(debugStreamBuffer_, offchipScratch);
#else
          return RESULT_OK;
#endif // #if SEND_DEBUG_STREAM
        }
#endif // DOCKING_ALGORITHM ==  DOCKING_BINARY_TRACKER

        Result SendArray(const Array<u8> &array, const char * objectName, MemoryStack scratch)
        {
#if SEND_DEBUG_STREAM
          debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferData_[0], DEBUG_STREAM_BUFFER_SIZE);
          debugStreamBuffer_.PushBack(objectName, array);
          return SendBuffer(debugStreamBuffer_, scratch);
#else
          return RESULT_OK;
#endif // #if SEND_DEBUG_STREAM
        }

        Result SendImage(const Array<u8> &array, const f32 exposureTime, const char * objectName, MemoryStack scratch)
        {
#if SEND_DEBUG_STREAM
          debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferData_[0], DEBUG_STREAM_BUFFER_SIZE);
          debugStreamBuffer_.PushBack(objectName, array);
          debugStreamBuffer_.PushBack<f32>("Exposure time", &exposureTime, 1);
          return SendBuffer(debugStreamBuffer_, scratch);
#else
          return RESULT_OK;
#endif // #if SEND_DEBUG_STREAM
        }

        Result SendBinaryImage(const Array<u8> &grayscaleImage, const char * objectName, const Tracker &tracker, const TrackerParameters &parameters, MemoryStack ccmScratch, MemoryStack onchipScratch, MemoryStack offchipScratch)
        {
          Result result = RESULT_OK;
#if SEND_DEBUG_STREAM && DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
          debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferData_[0], DEBUG_STREAM_BUFFER_SIZE);

          EdgeLists edgeLists;

          edgeLists.imageHeight = grayscaleImage.get_size(0);
          edgeLists.imageWidth = grayscaleImage.get_size(1);

          edgeLists.xDecreasing = FixedLengthList<Point<s16> >(parameters.edgeDetectionParams_update.maxDetectionsPerType, offchipScratch);
          edgeLists.xIncreasing = FixedLengthList<Point<s16> >(parameters.edgeDetectionParams_update.maxDetectionsPerType, offchipScratch);
          edgeLists.yDecreasing = FixedLengthList<Point<s16> >(parameters.edgeDetectionParams_update.maxDetectionsPerType, offchipScratch);
          edgeLists.yIncreasing = FixedLengthList<Point<s16> >(parameters.edgeDetectionParams_update.maxDetectionsPerType, offchipScratch);

          DetectBlurredEdges_GrayvalueThreshold(
            grayscaleImage,
            60,
            parameters.edgeDetectionParams_update.minComponentWidth, parameters.edgeDetectionParams_update.everyNLines,
            edgeLists);

          edgeLists.Serialize("Edge List", debugStreamBuffer_);

          const s32 height = CameraModeInfo[debugStreamResolution_].height;
          const s32 width  = CameraModeInfo[debugStreamResolution_].width;

          Array<u8> imageSmall(height, width, offchipScratch);

          DownsampleHelper(grayscaleImage, imageSmall, ccmScratch);

          debugStreamBuffer_.PushBack(objectName, imageSmall);

          result = SendBuffer(debugStreamBuffer_, offchipScratch);

#endif // #if SEND_DEBUG_STREAM

          return result;
        }

        Result Initialize()
        {
#if SEND_DEBUG_STREAM
          debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferData_[0], DEBUG_STREAM_BUFFER_SIZE);

          InitBenchmarking();
          BeginBenchmark("TotalTime");
#endif
          return RESULT_OK;
        }
      } // namespace DebugStream
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki
