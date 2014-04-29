#ifndef ROBOT_HARDWARE

#define COZMO_ROBOT

#include "communication.h"
#include "threadSafeQueue.h"
#include "debugStreamClient.h"

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/serialize.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/geometry.h"
#include "anki/common/robot/benchmarking.h"

#include "anki/vision/robot/transformations.h"
#include "anki/vision/robot/fiducialMarkers.h"
#include "anki/vision/robot/binaryTracker.h"

#include <queue>

#include <ctime>

#include "opencv/cv.h"

using namespace std;
using namespace Anki;
using namespace Anki::Embedded;

//
// All these variables and constants are for InitDisplayDebuggingInfo() and DisplayDebuggingInfo()
//

static const s32 bigHeight = 480;
static const s32 bigWidth = 640;
static const f32 scale = bigWidth / 320.0f;

static const s32 scratchSize = 1000000;
static u8 scratchBuffer[scratchSize];

static const s32 outputFilenamePatternLength = 1024;
static char outputFilenamePattern[outputFilenamePatternLength] = "C:/Users/Pete/Box Sync/Cozmo SE/systemTestImages/cozmo_date%04d_%02d_%02d_time%02d_%02d_%02d_frame%d.%s";

static f64 lastTime = 0;

static cv::Mat lastImage;
static cv::Mat largeLastImage;
static cv::Mat toShowImage;

static u8 lastMeanError = 0;
static f32 lastPercentMatchingGrayvalues = 0;
static s32 detectedFacesImageWidth = 160;

static Transformations::PlanarTransformation_f32 lastPlanarTransformation;

static f32 benchmarkTimes[2];

static std::vector<VisionMarker> visionMarkerList;

static std::vector<Anki::Embedded::Rectangle<s32> > detectedFaces;

static bool aMessageAlreadyPrinted;
static bool isTracking;

static vector<DebugStreamClient::Object> currentObjects;

static void printUsage()
{
} // void printUsage()

static void InitDisplayDebuggingInfo()
{
  //innerObjectName = reinterpret_cast<char*>( scratch.Allocate(SerializedBuffer::DESCRIPTION_STRING_LENGTH + 1) );

  // Used for displaying detected fiducials
  lastImage = cv::Mat(240,320,CV_8U);
  largeLastImage = cv::Mat(bigHeight, bigWidth, CV_8U);
  toShowImage = cv::Mat(bigHeight, bigWidth, CV_8UC3);

  lastMeanError = 0;
  lastPercentMatchingGrayvalues = 0;
  detectedFacesImageWidth = 160;

  lastImage.setTo(0);
  largeLastImage.setTo(0);
  toShowImage.setTo(0);

  lastPlanarTransformation = Transformations::PlanarTransformation_f32();

  benchmarkTimes[0] = -1.0f;
  benchmarkTimes[1] = -1.0f;

  visionMarkerList.clear();
  detectedFaces.clear();
  currentObjects.clear();

  aMessageAlreadyPrinted = false;
}

static void DisplayDebuggingInfo(const DebugStreamClient::Object &newObject)
{
  //
  // If we've reached a new message, display the last image and messages
  //

  MemoryStack scratch = MemoryStack(scratchBuffer, scratchSize, Flags::Buffer(false, true, false));

  if(strcmp(newObject.objectName, "Benchmarks") == 0) {
    const f64 pixelsPerMillisecond = 2.0;
    const s32 imageHeight = 800;
    const s32 imageWidth = 1600;

    FixedLengthList<BenchmarkElement> benchmarks = *(reinterpret_cast<FixedLengthList<BenchmarkElement>*>(newObject.startOfPayload));

    //PrintBenchmarkResults(benchmarks, false, false);

    FixedLengthList<BenchmarkElementName> namesToDisplay(1, scratch, Flags::Buffer(true, false, true));
    snprintf(namesToDisplay[0].name, BenchmarkElement::NAME_LENGTH, "DetectFiducialMarkers");

    ShowBenchmarkResults(benchmarks, namesToDisplay, pixelsPerMillisecond, imageHeight, imageWidth);
  }

  if(newObject.timeReceived != lastTime) {
    if(lastImage.rows > 0) {
      // Print FPS
      if(benchmarkTimes[0] > 0.0f) {
        char benchmarkBuffer[1024];

        static f64 lastTime;
        const f64 curTime = GetTimeF64();
        const f64 receivedDelta = curTime - lastTime;
        lastTime = GetTimeF64();

        snprintf(benchmarkBuffer, 1024, "Total:%0.1ffps Algorithms:%0.1ffps   GrayvalueError:%d %f", 1.0f/benchmarkTimes[1], 1.0f/benchmarkTimes[0], lastMeanError, lastPercentMatchingGrayvalues);

        cv::putText(toShowImage, benchmarkBuffer, cv::Point(5,15), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0,255,0));
      }

      if(isTracking) {
        cv::Mat trackingBoxImage(bigHeight, bigWidth, CV_8UC3);

        trackingBoxImage.setTo(0);

        const cv::Scalar textColor    = cv::Scalar(0,255,0);
        const cv::Scalar boxColor     = cv::Scalar(0,16,0);
        const cv::Scalar topLineColor = cv::Scalar(16,0,0);

        const Quadrilateral<f32> orientedCorners = lastPlanarTransformation.get_transformedCorners(scratch).ComputeRotatedCorners<f32>(0.0f);

        const s32 cornerOrder[5] = {
          Quadrilateral<f32>::TopLeft,
          Quadrilateral<f32>::TopRight,
          Quadrilateral<f32>::BottomRight,
          Quadrilateral<f32>::BottomLeft,
          Quadrilateral<f32>::TopLeft};

        for(s32 iCorner=0; iCorner<4; iCorner++) {
          const s32 point1Index = cornerOrder[iCorner];
          const s32 point2Index = cornerOrder[iCorner+1];
          const cv::Point pt1(Round<s32>(orientedCorners[point1Index].x*scale), Round<s32>(orientedCorners[point1Index].y*scale));
          const cv::Point pt2(Round<s32>(orientedCorners[point2Index].x*scale), Round<s32>(orientedCorners[point2Index].y*scale));

          cv::Scalar thisLineColor = (iCorner==0) ? topLineColor : boxColor;
          cv::line(trackingBoxImage, pt1, pt2, thisLineColor, 15);
        }

        const Point<f32> center = orientedCorners.ComputeCenter<f32>();
        const s32 textX = Round<s32>(MIN(MIN(MIN(orientedCorners.corners[0].x*scale, orientedCorners.corners[1].x*scale), orientedCorners.corners[2].x*scale), orientedCorners.corners[3].x*scale));
        const cv::Point textStartPoint(textX, Round<s32>(center.y*scale));

        cv::putText(trackingBoxImage, "Tracking", textStartPoint, cv::FONT_HERSHEY_PLAIN, 1.0, textColor);

        const s32 numPixels = bigHeight * bigWidth * 3;

        for(s32 iPixel=0; iPixel<numPixels; iPixel++) {
          toShowImage.data[iPixel] += trackingBoxImage.data[iPixel];
        }
      } else { // if(isTracking)
        // Draw markers
        for(s32 iMarker=0; iMarker<static_cast<s32>(visionMarkerList.size()); iMarker++) {
          cv::Scalar boxColor, topLineColor, textColor;
          if(visionMarkerList[iMarker].isValid) {
            textColor = cv::Scalar(0,255,0);
            boxColor = cv::Scalar(0,128,0);
          } else {
            textColor = cv::Scalar(0,0,255);
            boxColor = cv::Scalar(0,0,128);
          }
          topLineColor = cv::Scalar(128,0,0);

          const f32 observedOrientation = visionMarkerList[iMarker].observedOrientation;
          const Quadrilateral<f32> orientedCorners = visionMarkerList[iMarker].corners.ComputeRotatedCorners<f32>(0.0f);

          const s32 cornerOrder[5] = {
            Quadrilateral<f32>::TopLeft,
            Quadrilateral<f32>::TopRight,
            Quadrilateral<f32>::BottomRight,
            Quadrilateral<f32>::BottomLeft,
            Quadrilateral<f32>::TopLeft};

          for(s32 iCorner=0; iCorner<4; iCorner++) {
            const s32 point1Index = cornerOrder[iCorner];
            const s32 point2Index = cornerOrder[iCorner+1];
            const cv::Point pt1(Round<s32>(orientedCorners[point1Index].x*scale), Round<s32>(orientedCorners[point1Index].y*scale));
            const cv::Point pt2(Round<s32>(orientedCorners[point2Index].x*scale), Round<s32>(orientedCorners[point2Index].y*scale));

            cv::Scalar thisLineColor = (iCorner==0) ? topLineColor : boxColor;
            cv::line(toShowImage, pt1, pt2, thisLineColor, 2);
          }

          const Anki::Vision::MarkerType markerType = visionMarkerList[iMarker].markerType;

          const char * typeString = "??";
          if(static_cast<s32>(markerType) >=0 && static_cast<s32>(markerType) <= Anki::Vision::NUM_MARKER_TYPES) {
            typeString = Anki::Vision::MarkerTypeStrings[markerType];
          }

          const Point<s16> center = visionMarkerList[iMarker].corners.ComputeCenter<s16>();
          const s32 textX = Round<s32>(MIN(MIN(MIN(visionMarkerList[iMarker].corners[0].x*scale, visionMarkerList[iMarker].corners[1].x*scale), visionMarkerList[iMarker].corners[2].x*scale), visionMarkerList[iMarker].corners[3].x*scale));
          const cv::Point textStartPoint(textX, Round<s32>(center.y*scale));

          cv::putText(toShowImage, typeString, textStartPoint, cv::FONT_HERSHEY_PLAIN, 1.0, textColor);
        }
      } // if(isTracking) ... else

      if(detectedFaces.size() != 0) {
        const f32 faceScale = static_cast<f32>(bigWidth) / static_cast<f32>(detectedFacesImageWidth);
        for( s32 i = 0; i < static_cast<s32>(detectedFaces.size()); i++ )
        {
          cv::Point center( Round<s32>(faceScale*(detectedFaces[i].left + detectedFaces[i].right)*0.5), Round<s32>(faceScale*(detectedFaces[i].top + detectedFaces[i].bottom)*0.5) );
          cv::ellipse( toShowImage, center, cv::Size( Round<s32>(faceScale*(detectedFaces[i].right-detectedFaces[i].left)*0.5), Round<s32>(faceScale*(detectedFaces[i].bottom-detectedFaces[i].top)*0.5)), 0, 0, 360, cv::Scalar( 255, 0, 0 ), 5, 8, 0 );
        }
      }

      cv::imshow("Robot Image", toShowImage);
      const s32 pressedKey = cv::waitKey(10);
      //printf("%d\n", pressedKey);
      if(pressedKey == 'c') {
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

    for(size_t iObject=0; iObject<currentObjects.size(); iObject++) {
      free(currentObjects[iObject].buffer);
    }

    visionMarkerList.clear();
    detectedFaces.clear();
    currentObjects.clear();

    lastTime = newObject.timeReceived;
  } // if(newObject.timeReceived != lastTime)

  //
  // Add the newObject to out lists of data to display
  //

  currentObjects.push_back(newObject);

  if(strcmp(newObject.typeName, "Basic Type Buffer") == 0) {
    if(strcmp(newObject.objectName, "Benchmark Times") == 0) {
      const f32* tmpBuffer = reinterpret_cast<f32*>(newObject.startOfPayload);

      for(s32 i=0; i<2; i++) {
        benchmarkTimes[i] = tmpBuffer[i];
      }
    } else if(strcmp(newObject.objectName, "meanGrayvalueError") == 0) {
      u8* tmpBuffer = reinterpret_cast<u8*>(newObject.startOfPayload);

      lastMeanError = tmpBuffer[0];
    } else if(strcmp(newObject.objectName, "percentMatchingGrayvalues") == 0) {
      f32* tmpBuffer = reinterpret_cast<f32*>(newObject.startOfPayload);

      lastPercentMatchingGrayvalues = tmpBuffer[0];
    } else if (strcmp(newObject.objectName, "detectedFacesImageWidth") == 0) {
      s32* tmpBuffer = reinterpret_cast<s32*>(newObject.startOfPayload);

      detectedFacesImageWidth = tmpBuffer[0];
    } else {
      const s32 sizeOfType = reinterpret_cast<s32*>(newObject.buffer)[0];
      const s32 isBasicType = reinterpret_cast<s32*>(newObject.buffer)[1];
      const s32 isInteger = reinterpret_cast<s32*>(newObject.buffer)[2];
      const s32 isSigned = reinterpret_cast<s32*>(newObject.buffer)[3];
      const s32 isFloat = reinterpret_cast<s32*>(newObject.buffer)[4];
      const s32 numElements = reinterpret_cast<s32*>(newObject.buffer)[5];

      printf("%s: ", newObject.objectName);

      if(isBasicType)
        if(isFloat) {
          if(sizeOfType == 4) {
            for(s32 i=0; i<numElements; i++) { printf("%0.5f, ", reinterpret_cast<f32*>(newObject.startOfPayload)[i]); }
          } else if(sizeOfType == 8) {
            for(s32 i=0; i<numElements; i++) { printf("%0.5f, ", reinterpret_cast<f64*>(newObject.startOfPayload)[i]); }
          }
        } else {
          if(isSigned) {
            if(sizeOfType == 1) {
              for(s32 i=0; i<numElements; i++) { printf("%d, ", reinterpret_cast<s8*>(newObject.startOfPayload)[i]); }
            } else if(sizeOfType == 2) {
              for(s32 i=0; i<numElements; i++) { printf("%d, ", reinterpret_cast<s16*>(newObject.startOfPayload)[i]); }
            } else if(sizeOfType == 4) {
              for(s32 i=0; i<numElements; i++) { printf("%d, ", reinterpret_cast<s32*>(newObject.startOfPayload)[i]); }
            } else if(sizeOfType == 8) {
              for(s32 i=0; i<numElements; i++) { printf("%lld, ", reinterpret_cast<s64*>(newObject.startOfPayload)[i]); }
            }
          } else {
            if(sizeOfType == 1) {
              for(s32 i=0; i<numElements; i++) { printf("%d, ", reinterpret_cast<u8*>(newObject.startOfPayload)[i]); }
            } else if(sizeOfType == 2) {
              for(s32 i=0; i<numElements; i++) { printf("%d, ", reinterpret_cast<u16*>(newObject.startOfPayload)[i]); }
            } else if(sizeOfType == 4) {
              for(s32 i=0; i<numElements; i++) { printf("%u, ", reinterpret_cast<u32*>(newObject.startOfPayload)[i]); }
            } else if(sizeOfType == 8) {
              for(s32 i=0; i<numElements; i++) { printf("%llu, ", reinterpret_cast<u64*>(newObject.startOfPayload)[i]); }
            }
          }
        }
        printf("\n");
    }
  } else if(strcmp(newObject.typeName, "Array") == 0) {
    if (strcmp(newObject.objectName, "Robot Image") == 0) {
      const Array<u8> arr = *(reinterpret_cast<Array<u8>*>(newObject.startOfPayload));

      const s32 arrHeight = arr.get_size(0);
      const s32 arrWidth = arr.get_size(1);

      // Do the copy explicitly, to prevent OpenCV trying to be smart with memory
      lastImage = cv::Mat(arrHeight, arrWidth, CV_8U);

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
    }
  } else if(strcmp(newObject.typeName, "String") == 0) {
    printf("Board>> %s\n", reinterpret_cast<const char*>(newObject.startOfPayload));
  } else if(strcmp(newObject.typeName, "VisionMarker") == 0) {
    VisionMarker marker = *reinterpret_cast<VisionMarker*>(newObject.startOfPayload);

    if(!marker.isValid)
      return;

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
  } else if(strcmp(newObject.typeName, "PlanarTransformation_f32") == 0) {
    lastPlanarTransformation = *reinterpret_cast<Transformations::PlanarTransformation_f32*>(newObject.startOfPayload);
    //lastPlanarTransformation.Print();
    isTracking = true;
  } else if(strcmp(newObject.typeName, "BinaryTracker") == 0) {
    TemplateTracker::BinaryTracker bt = *reinterpret_cast<TemplateTracker::BinaryTracker*>(newObject.startOfPayload);

    if(!bt.IsValid())
      return;

    bt.ShowTemplate("BinaryTracker Template", false, false, 2.0f);
  } else if(strcmp(newObject.typeName, "EdgeLists") == 0) {
    EdgeLists edges = *reinterpret_cast<EdgeLists*>(newObject.startOfPayload);

    cv::Mat toShow = edges.DrawIndexes();

    if(toShow.cols == 0)
      return;

    cv::Mat toShowLargeTmp(bigHeight, bigWidth, CV_8UC3);
    cv::resize(toShow, toShowLargeTmp, toShowLargeTmp.size(), 0, 0, cv::INTER_NEAREST);
    cv::imshow("Detected Binary Edges", toShowLargeTmp);

    //cv::resize(toShow, toShowImage, toShowImage.size(), 0, 0, cv::INTER_NEAREST);
    //cv::imshow("Detected Binary Edges", toShowImage);
  } else if(strcmp(newObject.typeName, "ArraySlice") == 0) {
    if(strcmp(newObject.objectName, "detectedFaces") == 0) {
      FixedLengthList<Anki::Embedded::Rectangle<s32> > newFaces = *reinterpret_cast<FixedLengthList<Anki::Embedded::Rectangle<s32> >*>(newObject.startOfPayload);

      for(s32 i=0; i<newFaces.get_size(); i++) {
        detectedFaces.push_back(newFaces[i]);
      }
    }
  } else {
    char toPrint[32];

    memcpy(toPrint, newObject.typeName, 31);
    toPrint[31] = '\0';
    printf("Unknown Type \"%s\"", toPrint);

    memcpy(toPrint, newObject.objectName, 31);
    toPrint[31] = '\0';
    printf(" \"%s\"\n", toPrint);
  }
} // void DisplayDebuggingInfo()

int main(int argc, char ** argv)
{
  // TCP
  const char * ipAddress = "192.168.3.30";
  const s32 port = 5551;

  printf("Starting display\n");

  SetLogSilence(true);

  DebugStreamClient parserThread(ipAddress, port);

  InitDisplayDebuggingInfo();

  while(true) {
    DebugStreamClient::Object newObject = parserThread.GetNextObject();
    //printf("Received %s %s\n", newObject.typeName, newObject.newObject.objectName);

    DisplayDebuggingInfo(newObject);

    // Simple example to display an image
    //if(strcmp(newObject.newObject.objectName, "Robot Image") == 0) {
    //  Array<u8> image = *(reinterpret_cast<Array<u8>*>(newObject.startOfPayload));
    //  image.Show("Robot Image", false);
    //  cv::waitKey(10);
    //}
    //if(newObject.buffer) {
    //  free(newObject.buffer);
    //  newObject.buffer = NULL;
    //}
  }

  return 0;
} // int main()
#endif // #ifndef ROBOT_HARDWARE
