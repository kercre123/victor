/**
File: captureImages.cpp
Author: Peter Barnum
Created: 2015-01-28

Function to capture images as quickly as possible, when a high and constant frame rate is critical.
You should ideally compile this as RelWithDebInfo or Release. It only depends on the OpenCV libraries, not all the normal Anki libraries.

Copyright Anki, Inc. 2015
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef ROBOT_HARDWARE

#include "opencv2/highgui/highgui.hpp"

#if defined(_MSC_VER)
#include <windows.h>
#else
#if defined(__ARM_ARCH_7A__)
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <sys/resource.h>
#include <unistd.h>
#endif

// Copied from utilities.cpp
static double GetTimeF64()
{
#if defined(_MSC_VER)
  double timeInSeconds;

  static double frequency = 0;
  static LONGLONG startCounter = 0;

  LARGE_INTEGER counter;

  if(frequency == 0) {
    LARGE_INTEGER frequencyTmp;
    QueryPerformanceFrequency(&frequencyTmp);
    frequency = (double)frequencyTmp.QuadPart;
  }

  QueryPerformanceCounter(&counter);

  // Subtract startSeconds, so the floating point number has reasonable precision
  if(startCounter == 0) {
    startCounter = counter.QuadPart;
  }

  timeInSeconds = (double)(counter.QuadPart - startCounter) / frequency;
#elif defined(__APPLE_CC__)
  struct timeval time;
#     ifndef ANKI_IOS_BUILD
  // TODO: Fix build error when using this in an iOS build for arm architectures
  gettimeofday(&time, NULL);
#     endif

  // Subtract startSeconds, so the floating point number has reasonable precision
  static long startSeconds = 0;
  if(startSeconds == 0) {
    startSeconds = time.tv_sec;
  }

  const double timeInSeconds = (double)(time.tv_sec-startSeconds) + ((double)time.tv_usec / 1000000.0);
#else // Generic Unix
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  const double timeInSeconds = (double)(ts.tv_sec) + (double)(ts.tv_nsec) * (1.0 / 1000000000.0);
#endif

  return timeInSeconds;
} // static double GetTimeF64()

namespace Anki
{
  int CaptureImages(const int cameraId, const int numImages, const cv::Size2i imageSize, std::vector<cv::Mat> &capturedImages, double &captureTime, bool &wasQuitPressed, const bool startCaptureImmediately, const bool showPreview, const bool showCrosshair, const bool cameraSettingsGui)
  {
    const int crosshairBlackHalfWidth = 1;
    const int crosshairWhiteHalfWidth = 0;

    wasQuitPressed = false;

    if(numImages < 1 || numImages > 100000) {
      printf("Error: invalid numImages %d\n", numImages);
      return -1;
    }

    if(!startCaptureImmediately && !showPreview) {
      printf("Error: !startCaptureImmediately && !showPreview\n");
      return -1;
    }

    capturedImages.resize(numImages);

    if(capturedImages.size() != numImages) {
      printf("Error: Out of memory\n");
      capturedImages.resize(0);
      return -2;
    }

    cv::VideoCapture capture(cameraId);

    if (!capture.isOpened()) {
      printf("Error: Capture cannot be opened\n");
      capturedImages.resize(0);
      return -3;
    }

    capture.set(CV_CAP_PROP_FRAME_WIDTH, imageSize.width);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT, imageSize.height);

    if(cameraSettingsGui) {
      capture.set(CV_CAP_PROP_WHITE_BALANCE_BLUE_U, 4000);
      capture.set(CV_CAP_PROP_SETTINGS, 1);
    }

    // Capture a few frames to get things started
    for(int i=0; i<5; i++) {
      if(!capture.read(capturedImages[0])) {
        printf("Error: Could not read from camera.\n");
        capturedImages.resize(0);
        return -4;
      }
    }

    for(int i=1; i<numImages; i++) {
      //capturedImages[i].create(capturedImages[0].rows, capturedImages[0].cols, capturedImages[0].type());
      capturedImages[0].copyTo(capturedImages[i]);

      if(capturedImages[i].rows == 0) {
        printf("Error: Out of memory.\n");
        capturedImages.resize(0);
        return -4;
      }
    }

    if(showPreview) {
      cv::namedWindow("Video preview", CV_WINDOW_AUTOSIZE);
    }

    const cv::Point2i center((imageSize.width+1) / 2, (imageSize.height+1) / 2);

    cv::Mat toShowImage;

    double startTime = GetTimeF64();
    bool capturingImages = startCaptureImmediately;
    int curFrame = 0;
    while(curFrame < numImages) {
      if(!capture.read(capturedImages[curFrame])) {
        printf("Error: Could not read from camera on frame %d\n", curFrame);
        capturedImages.resize(curFrame-1);
        return -5;
      }

      if(showPreview) {
        if(showCrosshair) {
          capturedImages[curFrame].copyTo(toShowImage);

          toShowImage(cv::Rect(0, center.y-crosshairBlackHalfWidth, imageSize.width, 2*crosshairBlackHalfWidth+1)).setTo(0);
          toShowImage(cv::Rect(center.x-crosshairBlackHalfWidth, 0, 2*crosshairBlackHalfWidth+1, imageSize.height)).setTo(0);

          toShowImage(cv::Rect(0, center.y-crosshairWhiteHalfWidth, imageSize.width, 2*crosshairWhiteHalfWidth+1)).setTo(255);
          toShowImage(cv::Rect(center.x-crosshairWhiteHalfWidth, 0, 2*crosshairWhiteHalfWidth+1, imageSize.height)).setTo(255);

          cv::imshow("Video preview", toShowImage);
        } else {
          cv::imshow("Video preview", capturedImages[curFrame]);
        }

        const int pressedKey = cv::waitKey(1);

        if((pressedKey & 0xFF) == 'h') {
          printf("Hold 'c' to start capture, 's' to change camera settings (windows only), or 'q' to quit.\n");
        } else if((pressedKey & 0xFF) == 'c') {
          if(!capturingImages) {
            capturingImages = true;
            printf("Capturing now...\n");
            startTime = GetTimeF64();
            continue;
          }
        } else if ((pressedKey & 0xFF) == 's') {
          capture.set(CV_CAP_PROP_SETTINGS, 1);
        } else if((pressedKey & 0xFF) == 'q') {
          wasQuitPressed = true;
          break;
        }
      }

      if(capturingImages) {
        curFrame++;
      }
    } // while(curFrame < numImages)

    captureTime = GetTimeF64() - startTime;

    capturedImages.resize(curFrame);

    return 0;
  } // CaptureImages()
} // namespace Anki

#endif // #ifndef ROBOT_HARDWARE
