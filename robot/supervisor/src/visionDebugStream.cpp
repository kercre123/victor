
#include "anki/common/robot/config.h"
#include "anki/common/robot/serialize.h"

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/visionSystem.h"

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
          const s32 PRINTF_BUFFER_SIZE = 10000;
          const s32 DEBUG_STREAM_BUFFER_SIZE = 2000000;
          
          const s32 MAX_BYTES_PER_SECOND = 500000;
          
          const s32 SEND_EVERY_N_FRAMES = 7;
          
          OFFCHIP u8 printfBufferRaw_[PRINTF_BUFFER_SIZE];
          OFFCHIP u8 debugStreamBufferRaw_[DEBUG_STREAM_BUFFER_SIZE];
          
          SerializedBuffer printfBuffer_;
          SerializedBuffer debugStreamBuffer_;
          
          s32 lastSecond;
          s32 bytesSinceLastSecond;
          
          s32 frameNumber = 0;
          
          const HAL::CameraMode debugStreamResolution_ = HAL::CAMERA_MODE_QQQVGA;
          //const HAL::CameraMode debugStreamResolution_ = HAL::CAMERA_MODE_QVGA;
          
          f32 lastBenchmarkTime_algorithmsOnly;
          f32 lastBenchmarkDuration_algorithmsOnly;
          
          f32 lastBenchmarkTime_total;
          f32 lastBenchmarkDuration_total;
          
        } // private namespace
        
        ReturnCode SendBuffer(SerializedBuffer &toSend)
        {
#if SEND_DEBUG_STREAM
          const f32 curTime = GetTime();
          const s32 curSecond = RoundS32(curTime);
          
          // Hack, to prevent overwhelming the send buffer
          if(curSecond != lastSecond) {
            bytesSinceLastSecond = 0;
            lastSecond = curSecond;
          } else {
            if(bytesSinceLastSecond > MAX_BYTES_PER_SECOND) {
              lastBenchmarkTime_algorithmsOnly = GetTime();
              return EXIT_SUCCESS;
            } else {
              bytesSinceLastSecond += toSend.get_memoryStack().get_usedBytes();
            }
          }
          
          const f32 benchmarkTimes[2] = {lastBenchmarkDuration_algorithmsOnly, lastBenchmarkDuration_total};
          
          toSend.PushBack<f32>("Benchmark Times", &benchmarkTimes[0], 2);
          
          const s32 nothing[8] = {0,0,0,0,0,0,0,0};
          toSend.PushBack<s32>("Nothing", &nothing[0], 8);
          toSend.PushBack<s32>("Nothing", &nothing[0], 8);
          
          s32 startIndex;
          const u8 * bufferStart = reinterpret_cast<const u8*>(toSend.get_memoryStack().get_validBufferStart(startIndex));
          const s32 validUsedBytes = toSend.get_memoryStack().get_usedBytes() - startIndex;
          
          Anki::Cozmo::HAL::UARTPutBuffer(const_cast<u8*>(Embedded::SERIALIZED_BUFFER_HEADER), SERIALIZED_BUFFER_HEADER_LENGTH);
          Anki::Cozmo::HAL::UARTPutBuffer(const_cast<u8*>(bufferStart), validUsedBytes);
          Anki::Cozmo::HAL::UARTPutBuffer(const_cast<u8*>(Embedded::SERIALIZED_BUFFER_FOOTER), SERIALIZED_BUFFER_FOOTER_LENGTH);
          
          /*for(s32 i=0; i<Embedded::SERIALIZED_BUFFER_HEADER_LENGTH; i++) {
           Anki::Cozmo::HAL::UARTPutChar(Embedded::SERIALIZED_BUFFER_HEADER[i]);
           }
           
           for(s32 i=0; i<validUsedBytes; i++) {
           Anki::Cozmo::HAL::UARTPutChar(bufferStart[i]);
           }
           
           for(s32 i=0; i<Embedded::SERIALIZED_BUFFER_FOOTER_LENGTH; i++) {
           Anki::Cozmo::HAL::UARTPutChar(Embedded::SERIALIZED_BUFFER_FOOTER[i]);
           }*/
          
          lastBenchmarkTime_algorithmsOnly = GetTime();
#endif // SEND_DEBUG_STREAM
          
          return EXIT_SUCCESS;
        }
        
        // TODO: Commented out to prevent unused compiler warning. Add back if needed.
        //static ReturnCode SendPrintf(const char * string)
        //{
        //  printfBuffer_ = SerializedBuffer(&printfBufferRaw_[0], PRINTF_BUFFER_SIZE);
        //  printfBuffer_.PushBackString(string);
        
        //  return SendBuffer(printfBuffer_);
        //} // void SendPrintf(const char * string)
        
        ReturnCode SendFiducialDetection(const Array<u8> &image, const FixedLengthList<VisionMarker> &markers, MemoryStack ccmScratch, MemoryStack onchipScratch, MemoryStack offchipScratch)
        {
          ReturnCode result = EXIT_SUCCESS;
          
#if SEND_DEBUG_STREAM
          const f32 curTime = GetTime();
          
          // lastBenchmarkTime_algorithmsOnly is set again when the transmission is complete
          lastBenchmarkDuration_algorithmsOnly = curTime - lastBenchmarkTime_algorithmsOnly;
          
          lastBenchmarkDuration_total = curTime - lastBenchmarkTime_total;
          lastBenchmarkTime_total = curTime;
          
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
          
          result = SendBuffer(debugStreamBuffer_);
          
          // The UART can't handle this at full rate, so wait a bit between each frame
          if(debugStreamResolution_ == HAL::CAMERA_MODE_QVGA) {
            HAL::MicroWait(1000000);
          }
#endif // #if SEND_DEBUG_STREAM
          
          return result;
        } // ReturnCode SendDebugStream_Detection()
        
        ReturnCode SendTrackingUpdate(
          const Array<u8> &image, 
          const Tracker &tracker, 
          const TrackerParameters &parameters, 
          const u8 meanGrayvalueError,
          const f32 percentMatchingGrayvalues,
          MemoryStack ccmScratch, 
          MemoryStack onchipScratch, 
          MemoryStack offchipScratch)
        {
          ReturnCode result = EXIT_SUCCESS;
          
#if SEND_DEBUG_STREAM
          const f32 curTime = GetTime();
          
          // lastBenchmarkTime_algorithmsOnly is set again when the transmission is complete
          lastBenchmarkDuration_algorithmsOnly = curTime - lastBenchmarkTime_algorithmsOnly;
          
          lastBenchmarkDuration_total = curTime - lastBenchmarkTime_total;
          lastBenchmarkTime_total = curTime;
          
          const s32 height = CameraModeInfo[debugStreamResolution_].height;
          const s32 width  = CameraModeInfo[debugStreamResolution_].width;
          
          // TODO: compute max allocation correctly
          //const s32 requiredBytes = height*width + 1024;
          
          /*if(onchipScratch.ComputeLargestPossibleAllocation() >= requiredBytes) {
           void * buffer = onchipScratch.Allocate(requiredBytes);
           debugStreamBuffer_ = SerializedBuffer(buffer, requiredBytes);
           } else {*/
          debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
          //}
          
          //transformation.Print();
          
          tracker.get_transformation().Serialize("Transformation", debugStreamBuffer_);
          
          debugStreamBuffer_.PushBack<u8>("meanGrayvalueError", &meanGrayvalueError, 1);
          
          debugStreamBuffer_.PushBack<f32>("percentMatchingGrayvalues", &percentMatchingGrayvalues, 1);
          
          frameNumber++;
          
          if(frameNumber % SEND_EVERY_N_FRAMES != 0) {
            return EXIT_SUCCESS;
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
          
          result = SendBuffer(debugStreamBuffer_);
          
          // The UART can't handle this at full rate, so wait a bit between each frame
          if(debugStreamResolution_ == HAL::CAMERA_MODE_QVGA) {
            HAL::MicroWait(1000000);
          }
          
#endif // #if SEND_DEBUG_STREAM
          
          return result;
        } // static ReturnCode SendTrackingUpdate()
        
#if DOCKING_ALGORITHM ==  DOCKING_BINARY_TRACKER
        ReturnCode SendBinaryTracker(const TemplateTracker::BinaryTracker &tracker, MemoryStack ccmScratch, MemoryStack onchipScratch, MemoryStack offchipScratch)
        {
#if SEND_DEBUG_STREAM

          // TODO: compute max allocation correctly
          const s32 requiredBytes = 48000;
          
          if(onchipScratch.ComputeLargestPossibleAllocation() >= requiredBytes) {
            void * buffer = onchipScratch.Allocate(requiredBytes);
            debugStreamBuffer_ = SerializedBuffer(buffer, requiredBytes);
          } else {
            debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
          }
          
          tracker.Serialize("Binary Tracker", debugStreamBuffer_);
          
          return SendBuffer(debugStreamBuffer_);
#else
          return EXIT_SUCCESS;
#endif // #if SEND_DEBUG_STREAM
        }
#endif // DOCKING_ALGORITHM ==  DOCKING_BINARY_TRACKER
        
        ReturnCode SendArray(const Array<u8> &array)
        {
#if SEND_DEBUG_STREAM
          debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
          debugStreamBuffer_.PushBack("Array", array);
          return SendBuffer(debugStreamBuffer_);
#else
          return EXIT_SUCCESS;
#endif // #if SEND_DEBUG_STREAM
        }
        
        ReturnCode SendBinaryImage(const Array<u8> &grayscaleImage, const Tracker &tracker, const TrackerParameters &parameters, MemoryStack ccmScratch, MemoryStack onchipScratch, MemoryStack offchipScratch)
        {
          ReturnCode result = EXIT_SUCCESS;
#if SEND_DEBUG_STREAM && DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
          DebugStream::debugStreamBuffer_ = SerializedBuffer(&DebugStream::debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
          
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
          
          debugStreamBuffer_.PushBack("Robot Image", imageSmall);
          
          result = SendBuffer(debugStreamBuffer_);
          
#endif // #if SEND_DEBUG_STREAM
          
          return result;
        }
        
        ReturnCode Initialize()
        {
#if SEND_DEBUG_STREAM
          // TODO: add the rest
          debugStreamBuffer_ = SerializedBuffer(&debugStreamBufferRaw_[0], DEBUG_STREAM_BUFFER_SIZE);
          printfBuffer_ = SerializedBuffer(&printfBufferRaw_[0], PRINTF_BUFFER_SIZE);
#endif
          return EXIT_SUCCESS;
        }
      } // namespace DebugStream
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki
