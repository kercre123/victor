/**
File: captureImages.h
Author: Peter Barnum
Created: 2015-01-28

Function to capture images as quickly as possible, when a high and constant frame rate is critical.
You should ideally compile this as RelWithDebInfo or Release. It only depends on the OpenCV libraries, not all the normal Anki libraries.

Copyright Anki, Inc. 2015
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _CAPTURE_IMAGES_H_
#define _CAPTURE_IMAGES_H_

#include "opencv/cv.h"

namespace Anki
{
  // Capture a bunch of images efficiently
  // captureTime is the time right between right before the first frame to right after the last frame
  // NOTE: cameraSettingsGui doesn't work on OSX
  // Returns 0 if successful, or a negative number if it fails
  int CaptureImages(const int cameraId, const int numImages, const cv::Size2i imageSize, std::vector<cv::Mat> &capturedImages, double &captureTime, bool &wasQuitPressed, const bool startCaptureImmediately=false, const bool showPreview=true, const bool showCrosshair=true, const bool cameraSettingsGui=true);
}

#endif // #ifndef _CAPTURE_IMAGES_H_
