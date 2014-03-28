/**
File: messageHandling_display.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "messageHandling.h"

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/serialize.h"

#include "anki/vision/robot/fiducialMarkers.h"
#include "anki/vision/robot/binaryTracker.h"

#include <ctime>
#include <vector>

#include "opencv/cv.h"
#include "opencv2/imgproc/imgproc.hpp"

using namespace Anki::Embedded;
using namespace std;

#ifndef ROBOT_HARDWARE

#ifndef _MSC_VER
#error Currently, only visual c++ is supported
#endif

#ifndef ANKICORETECH_EMBEDDED_USE_OPENCV
#error OpenCV is required
#endif

static const s32 scratchSize = 1000000;
static u8 scratchBuffer[scratchSize];

const s32 outputFilenamePatternLength = 1024;
char outputFilenamePattern[outputFilenamePatternLength] = "C:/Users/Pete/Box Sync/Cozmo SE/systemTestImages/cozmo_date%04d_%02d_%02d_time%02d_%02d_%02d_frame%d.%s";

void ProcessRawBuffer_Display(DisplayRawBuffer &buffer, const bool requireMatchingSegmentLengths)
{
  const s32 bigHeight = 480;
  const s32 bigWidth = 640;
  const f32 scale = bigWidth / 320.0f;

  MemoryStack scratch(scratchBuffer, scratchSize, Flags::Buffer(false, true, false));

  char *innerObjectName = reinterpret_cast<char*>( scratch.Allocate(SerializedBuffer::DESCRIPTION_STRING_LENGTH + 1) );

  // Used for displaying detected fiducials
  cv::Mat lastImage(240,320,CV_8U);
  cv::Mat largeLastImage(bigHeight, bigWidth, CV_8U);
  cv::Mat toShowImage(bigHeight, bigWidth, CV_8UC3);

  lastImage.setTo(0);
  largeLastImage.setTo(0);
  toShowImage.setTo(0);

  bool isTracking = false;
  Transformations::PlanarTransformation_f32 lastPlanarTransformation = Transformations::PlanarTransformation_f32(); //(Transformations::TRANSFORM_PROJECTIVE, scratch);

  f32 benchmarkTimes[2] = {-1.0f, -1.0f};

  std::vector<VisionMarker> visionMarkerList;

  SerializedBuffer serializedBuffer(buffer.data, buffer.curDataLength, Anki::Embedded::Flags::Buffer(false, true, true));

  SerializedBufferReconstructingIterator iterator(serializedBuffer);

  bool aMessageAlreadyPrinted = false;

  while(iterator.HasNext()) {
    s32 dataLength;
    const char * typeName = NULL;
    const char * objectName = NULL;
    bool isReportedSegmentLengthCorrect;
    void * dataSegment = reinterpret_cast<u8*>(iterator.GetNext(&typeName, &objectName, dataLength, isReportedSegmentLengthCorrect));

    if(!dataSegment) {
      break;
    }

    if(requireMatchingSegmentLengths && !isReportedSegmentLengthCorrect) {
      continue;
    }

    //printf("Next segment is (%d,%d): ", dataLength, type);
    if(strcmp(typeName, "Basic Type Buffer") == 0) {
      u16 size;
      bool isBasicType;
      bool isInteger;
      bool isSigned;
      bool isFloat;
      s32 numElements;
      SerializedBuffer::EncodedBasicTypeBuffer::Deserialize(false, size, isBasicType, isInteger, isSigned, isFloat, numElements, &dataSegment, dataLength);

      // Hack to detect a benchmarking pair
      if(isFloat && size==4 && numElements==2) {
        PUSH_MEMORY_STACK(scratch);
        f32* tmpBuffer = SerializedBuffer::DeserializeRawBasicType<f32>(innerObjectName, &dataSegment, dataLength, scratch);
        for(s32 i=0; i<2; i++) {
          benchmarkTimes[i] = tmpBuffer[i];
        }
        //printf("Times: %f %f\n", times[0], times[1]);
      } else {
        printf("Basic type buffer segment (%d, %d, %d, %d, %d): ", size, isInteger, isSigned, isFloat, numElements);
        /*for(s32 i=0; i<remainingDataLength; i++) {
        printf("%x ", dataSegment[i]);
        }*/
      }
    } else if(strcmp(typeName, "Array") == 0) {
      dataSegment = reinterpret_cast<u8*>(dataSegment) + 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;

      s32 height;
      s32 width;
      s32 stride;
      Flags::Buffer flags;
      u16 basicType_size;
      bool basicType_isBasicType;
      bool basicType_isInteger;
      bool basicType_isSigned;
      bool basicType_isFloat;
      s32 basicType_numElements;
      SerializedBuffer::EncodedArray::Deserialize(false, height, width, stride, flags, basicType_size, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_numElements, &dataSegment, dataLength);

      //template<typename Type> static Result DeserializeArray(const void * data, const s32 dataLength, Array<Type> &out, MemoryStack &memory);
      if(basicType_size==1 && basicType_isInteger==1 && basicType_isSigned==0 && basicType_isFloat==0) {
        Array<u8> arr = SerializedBuffer::DeserializeRawArray<u8>(NULL, &dataSegment, dataLength, scratch);

        if(!arr.IsValid()) {
          continue;
        }

        const s32 arrHeight = arr.get_size(0);
        const s32 arrWidth = arr.get_size(1);

        // Do the copy explicitly, to prevent OpenCV trying to be smart with memory
        lastImage = cv::Mat(arrHeight, arrWidth, CV_8U);
        //const cv::Mat_<u8> &mat = arr.get_CvMat_();
        //const s32 numBytes = mat.size().width * mat.size().height;
        /*for(s32 i=0; i<numBytes; i++) {
        lastImage.data[i] = mat.data[i];
        }*/
        s32 cLastImage = 0;
        for(s32 y=0; y<arrHeight; y++) {
          const u8 * restrict pArr = arr.Pointer(y,0);
          for(s32 x=0; x<arrWidth; x++) {
            lastImage.data[cLastImage] = pArr[x];
            cLastImage++;
          }
        }

        cv::resize(lastImage, largeLastImage, largeLastImage.size(), 0, 0, cv::INTER_NEAREST);

        const s32 blinkerWidth = 7;

        //Draw a blinky rectangle at the upper right
        static s32 frameNumber = 0;
        frameNumber++;

        if(frameNumber%2 == 0) {
          for(s32 y=0; y<blinkerWidth; y++) {
            for(s32 x=bigWidth-blinkerWidth; x<bigWidth; x++) {
              largeLastImage.at<u8>(y,x) = 0;
            }
          }

          for(s32 y=1; y<blinkerWidth-1; y++) {
            for(s32 x=bigWidth+1-blinkerWidth; x<(bigWidth-1); x++) {
              largeLastImage.at<u8>(y,x) = 255;
            }
          }
          //largeLastImage.at<u8>(blinkerHalfWidth,320-blinkerHalfWidth) = 255;
        } else {
          for(s32 y=0; y<blinkerWidth; y++) {
            for(s32 x=bigWidth-blinkerWidth; x<bigWidth; x++) {
              largeLastImage.at<u8>(y,x) = 0;
            }
          }
        }

        // Grayscale to RGB
        vector<cv::Mat> channels;
        channels.push_back(largeLastImage);
        channels.push_back(largeLastImage);
        channels.push_back(largeLastImage);
        cv::merge(channels, toShowImage);

        //cv::resize(toShowImage, toShowLarge, toShowLargeTmp.size(), 0, 0, cv::INTER_NEAREST);
      } else {
        printf("Array: (%d, %d, %d, %d, %d, %d, %d, %d) ", height, width, stride, flags, basicType_size, basicType_isInteger, basicType_isSigned, basicType_isFloat);
      }
    } else if(strcmp(typeName, "String") == 0) {
      printf("Board>> %s", dataSegment);
    } else if(strcmp(typeName, "VisionMarker") == 0) {
      VisionMarker marker;

      marker.Deserialize(innerObjectName, reinterpret_cast<void**>(&dataSegment), dataLength);

      if(!aMessageAlreadyPrinted) {
        time_t rawtime;
        time (&rawtime);
        string timeString = string(ctime(&rawtime));
        timeString[timeString.length()-6] = '\0';
        printf("%s>> ", timeString.data());
        aMessageAlreadyPrinted = true;
      }

      marker.Print();
      printf("\n");
      visionMarkerList.push_back(marker);
      isTracking = false;
    } else if(strcmp(typeName, "PlanarTransformation_f32") == 0) {
      lastPlanarTransformation.Deserialize(innerObjectName, reinterpret_cast<void**>(&dataSegment), dataLength, scratch);
      //lastPlanarTransformation.Print();
      isTracking = true;
    } else if(strcmp(typeName, "BinaryTracker") == 0) {
      PUSH_MEMORY_STACK(scratch);
      TemplateTracker::BinaryTracker bt;
      bt.Deserialize(innerObjectName, reinterpret_cast<void**>(&dataSegment), dataLength, scratch);
      bt.ShowTemplate("BinaryTracker Template", false, false);
    } else if(strcmp(typeName, "EdgeLists") == 0) {
      PUSH_MEMORY_STACK(scratch);
      EdgeLists edges;
      edges.Deserialize(innerObjectName, reinterpret_cast<void**>(&dataSegment), dataLength, scratch);

      cv::Mat toShow = edges.DrawIndexes();

      if(toShow.cols == 0)
        continue;

      cv::Mat toShowLargeTmp(bigHeight, bigWidth, CV_8UC3);
      cv::resize(toShow, toShowLargeTmp, toShowLargeTmp.size(), 0, 0, cv::INTER_NEAREST);
      cv::imshow("Detected Binary Edges", toShowLargeTmp);

      //cv::resize(toShow, toShowImage, toShowImage.size(), 0, 0, cv::INTER_NEAREST);
      //cv::imshow("Detected Binary Edges", toShowImage);
    } else {
      printf("Unknown Type %s\n", typeName);
    }

    //printf("\n");
  } // while(iterator.HasNext())

  if(aMessageAlreadyPrinted) {
    printf("\n");
  }

  if(lastImage.rows > 0) {
    // std::queue<VisionMarker> visionMarkerList;
    //Quadrilateral<s16> corners; // SQ 15.0 (Though may be changed later)
    //Vision::MarkerType markerType;
    //bool isValid;

    // Print FPS
    if(benchmarkTimes[0] > 0.0f) {
      char benchmarkBuffer[1024];

      static f32 lastTime;
      const f32 curTime = GetTime();
      const f32 receivedDelta = curTime - lastTime;
      lastTime = GetTime();

      //snprintf(benchmarkBuffer, 1024, "Total:%dfps Algorithms:%dfps Received:%dfps", RoundS32(1.0f/benchmarkTimes[1]), RoundS32(1.0f/benchmarkTimes[0]), RoundS32(1.0f/receivedDelta));
      snprintf(benchmarkBuffer, 1024, "Total:%dfps Algorithms:%dfps", RoundS32(1.0f/benchmarkTimes[1]), RoundS32(1.0f/benchmarkTimes[0]));

      cv::putText(toShowImage, benchmarkBuffer, cv::Point(5,15), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0,255,0));
    }

    if(isTracking) {
      cv::Mat trackingBoxImage(bigHeight, bigWidth, CV_8UC3);

      trackingBoxImage.setTo(0);

      const cv::Scalar textColor = cv::Scalar(0,255,0);
      //const cv::Scalar boxColor = cv::Scalar(0,128,0);
      const cv::Scalar boxColor = cv::Scalar(48,48,48);

      const Quadrilateral<f32> transformedCorners = lastPlanarTransformation.get_transformedCorners(scratch);

      const Quadrilateral<f32> sortedCorners = transformedCorners.ComputeClockwiseCorners();

      for(s32 iCorner=0; iCorner<4; iCorner++) {
        const s32 point1Index = iCorner;
        const s32 point2Index = (iCorner+1) % 4;
        const cv::Point pt1(static_cast<s32>(sortedCorners[point1Index].x*scale), static_cast<s32>(sortedCorners[point1Index].y*scale));
        const cv::Point pt2(static_cast<s32>(sortedCorners[point2Index].x*scale), static_cast<s32>(sortedCorners[point2Index].y*scale));
        cv::line(trackingBoxImage, pt1, pt2, boxColor, 7);
      }

      const Point<f32> center = sortedCorners.ComputeCenter();
      const s32 textX = RoundS32(MIN(MIN(MIN(sortedCorners.corners[0].x*scale, sortedCorners.corners[1].x*scale), sortedCorners.corners[2].x*scale), sortedCorners.corners[3].x*scale));
      const cv::Point textStartPoint(textX, RoundS32(center.y*scale));

      cv::putText(trackingBoxImage, "Tracking", textStartPoint, cv::FONT_HERSHEY_PLAIN, 1.0, textColor);

      const s32 numPixels = bigHeight * bigWidth * 3;

      for(s32 iPixel=0; iPixel<numPixels; iPixel++) {
        toShowImage.data[iPixel] += trackingBoxImage.data[iPixel];
      }
    } else { // if(isTracking)
      // Draw markers
      for(s32 iMarker=0; iMarker<static_cast<s32>(visionMarkerList.size()); iMarker++) {
        cv::Scalar boxColor, textColor;
        if(visionMarkerList[iMarker].isValid) {
          textColor = cv::Scalar(0,255,0);
          boxColor = cv::Scalar(0,128,0);
        } else {
          textColor = cv::Scalar(0,0,255);
          boxColor = cv::Scalar(0,0,128);
        }

        const Quadrilateral<s16> sortedCorners = visionMarkerList[iMarker].corners.ComputeClockwiseCorners();

        for(s32 iCorner=0; iCorner<4; iCorner++) {
          const s32 point1Index = iCorner;
          const s32 point2Index = (iCorner+1) % 4;
          const cv::Point pt1(RoundS32(sortedCorners[point1Index].x*scale), RoundS32(sortedCorners[point1Index].y*scale));
          const cv::Point pt2(RoundS32(sortedCorners[point2Index].x*scale), RoundS32(sortedCorners[point2Index].y*scale));
          cv::line(toShowImage, pt1, pt2, boxColor, 2);
        }

        const Anki::Vision::MarkerType markerType = visionMarkerList[iMarker].markerType;

        const char * typeString = "??";
        if(static_cast<s32>(markerType) >=0 && static_cast<s32>(markerType) <= Anki::Vision::NUM_MARKER_TYPES) {
          typeString = Anki::Vision::MarkerTypeStrings[markerType];
        }

        const Point<s16> center = visionMarkerList[iMarker].corners.ComputeCenter();
        const s32 textX = RoundS32(MIN(MIN(MIN(visionMarkerList[iMarker].corners[0].x*scale, visionMarkerList[iMarker].corners[1].x*scale), visionMarkerList[iMarker].corners[2].x*scale), visionMarkerList[iMarker].corners[3].x*scale));
        const cv::Point textStartPoint(textX, RoundS32(center.y*scale));

        cv::putText(toShowImage, typeString, textStartPoint, cv::FONT_HERSHEY_PLAIN, 1.0, textColor);
      }
    } // if(isTracking) ... else

    cv::imshow("Robot Image", toShowImage);
    s32 pressedKey = cv::waitKey(10);
    //printf("%d\n", pressedKey);
    if(pressedKey == 99) { // c
      const time_t t = time(0);   // get time now
      const struct tm * currentTime = localtime(&t);
      char outputFilename[1024];
      snprintf(&outputFilename[0], 1024, outputFilenamePattern,
        currentTime->tm_year+1900, currentTime->tm_mon+1, currentTime->tm_mday,
        currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec,
        0,
        "png");

      printf("Saving to %s\n", outputFilename);
      cv::imwrite(outputFilename, lastImage);
    }
  } else { // if(lastImage.rows > 0)
    cv::waitKey(1);
  }

  free(buffer.rawDataPointer);
  buffer.rawDataPointer = NULL;
  buffer.data = NULL;

  return;
} // void ProcessRawBuffer()

#endif // #ifndef ROBOT_HARDWARE
