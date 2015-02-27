#ifndef ROBOT_HARDWARE

#include "recognizeFaces.h"

#include "opencv2/highgui/highgui.hpp"

#include "LuxandFaceSDK.h"

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"

using namespace Anki;

static bool initialized = false;

static const char * luxandLicense = "d1mT6FU+dbiXs+gfPa3iFp2KcWHh/FsIN5khjZ3UkfvJswLajGu0+OxP8Q0Aatw7oqqEvXV6nXs0tl1lh0F4tPQX4uq5PwxmMzYFhfUyfLN01N+zCq4iZMdKLYPCWSbDWQH5+yMbqyQcoW1bvus67WKaaLG8/RZsxf7wm5bMdJw=";

static HTracker tracker = 0;

static Result InitializeTracker()
{
  if(initialized) {
    return RESULT_OK;
  }
  
  if(FSDKE_OK != FSDK_ActivateLibrary(luxandLicense)) {
    return RESULT_FAIL;
  }
  
  if(FSDKE_OK != FSDK_Initialize("")) {
    return RESULT_FAIL;
  }
  
  if(FSDKE_OK != FSDK_SetNumThreads(1)) {
    return RESULT_FAIL;
  }
  
  if(FSDKE_OK != FSDK_CreateTracker(&tracker)) {
    return RESULT_FAIL;
  }
  
  int err = 0; // set realtime face detection parameters
	FSDK_SetTrackerMultipleParameters(tracker, "HandleArbitraryRotations=false; DetermineFaceRotationAngle=false; InternalResizeWidth=100; FaceDetectionThreshold=5;", &err);
  
  initialized = true;
  
  printf("Tracker initialized\n");
  
  return RESULT_OK;
} // InitializeTracker()

static HImage OpencvToFsdk(const cv::Mat_<u8> &in) {
  HImage fsdkImage = 0;

  FSDK_LoadImageFromBuffer(&fsdkImage, in.ptr(), in.cols, in.rows, in.step[0], FSDK_IMAGE_GRAYSCALE_8BIT);
  
  return fsdkImage;
} // OpencvToFsdk()

namespace Anki
{
  Result AddFaceToDatabase(const cv::Mat_<u8> &image, const u32 faceId)
  {
    Result lastResult;
    
    if((lastResult = InitializeTracker()) != RESULT_OK) {
      return RESULT_FAIL;
    }
    
    HImage imageHandle = OpencvToFsdk(image);
    
    long long IDs[256]; // detected faces
    long long faceCount = 0;
    //FSDK_FeedFrame(tracker, 0, imageHandle, &faceCount, IDs, sizeof(IDs));
    
    FSDK_FreeImage(imageHandle);
    
    return RESULT_OK;
  } // AddFaceToDatabase()

  Result RecognizeFaces(const cv::Mat_<u8> &image, std::vector<Face> &faces)
  {
    Result lastResult;
    
    faces.clear();
    
    if((lastResult = InitializeTracker()) != RESULT_OK) {
      return RESULT_FAIL;
    }
    
    HImage imageHandle = OpencvToFsdk(image);
    
    long long IDs[256]; // detected faces
    long long faceCount = 0;
    FSDK_FeedFrame(tracker, 0, imageHandle, &faceCount, IDs, sizeof(IDs));
    
    for(size_t iFace=0; iFace<faceCount; iFace++) {
      TFacePosition facePosition;
			FSDK_GetTrackerFacePosition(tracker, 0, IDs[iFace], &facePosition);
      
      Face newFace;
      newFace.xc = facePosition.xc;
      newFace.yc = facePosition.yc;
      newFace.w = facePosition.w;
      newFace.padding = facePosition.padding;
      newFace.angle = facePosition.angle;
      newFace.faceId = -1;
      
      faces.push_back(newFace);
    } //     for(size_t iFace=0; iFace<faceCount; iFace++)
    
    FSDK_FreeImage(imageHandle);
  
    return RESULT_OK;
  } // RecognizeFaces()
} // namespace Anki

#endif // #ifndef ROBOT_HARDWARE
