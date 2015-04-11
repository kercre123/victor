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

// TODO: set to 1 for testing on phone, etc
const int maxThreads = 4;

static Result InitializeTracker()
{
  if(initialized) {
    //printf("Already Initialized\n");
    return RESULT_OK;
  }
  
  if(FSDKE_OK != FSDK_ActivateLibrary(luxandLicense)) {
    return RESULT_FAIL;
  }
  
  if(FSDKE_OK != FSDK_Initialize("")) {
    return RESULT_FAIL;
  }
  
  if(FSDKE_OK != FSDK_SetNumThreads(maxThreads)) {
    return RESULT_FAIL;
  }
  
  if(FSDKE_OK != FSDK_CreateTracker(&tracker)) {
    return RESULT_FAIL;
  }
  
  //FSDK_SetFaceDetectionParameters(false, false, 640);
  
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
  Result LoadDatabase(const char * filename)
  {
    if(initialized) {
      FSDK_FreeTracker(tracker);
      tracker = 0;
      initialized = false;
    }
    
    InitializeTracker();

    if(filename) {
      int loadResult;
      if (FSDKE_OK != (loadResult = FSDK_LoadTrackerMemoryFromFile(&tracker, filename))) {
        printf("Could not load file %s. Initializing an empty tracker.\n", filename);
        return RESULT_FAIL;
      }
    }

    initialized = true;

    return RESULT_OK;
  } // LoadDatabase()
  
  Result SaveDatabase(const char * filename)
  {
    if(!initialized) {
      printf("Tracker is not initialized\n");
      return RESULT_FAIL;
    }
    
    if (FSDKE_OK != FSDK_SaveTrackerMemoryToFile(tracker, filename)) {
      printf("Could not save file %s\n", filename);
      return RESULT_FAIL;
    }

    return RESULT_OK;
  } // SaveDatabase()

  Result SetRecognitionParameters(const bool handleArbitraryRotations, const int internalResizeWidth, const int faceDetectionThreshold)
  {
    Result lastResult;
    if((lastResult = InitializeTracker()) != RESULT_OK) {
      return RESULT_FAIL;
    }
    
  	//FSDK_SetFaceDetectionParameters(handleArbitraryRotations, false, internalResizeWidth);
    
    Result result = RESULT_OK;
    
    char parameterBuffer[1024];
    
    snprintf(parameterBuffer, 1024, "%d", handleArbitraryRotations);
    if(FSDK_SetTrackerParameter(tracker, "HandleArbitraryRotations,", parameterBuffer) != FSDKE_OK) {
      printf("set/TrackerPameter fail 1\n");
      result = RESULT_FAIL;
    }
    
    snprintf(parameterBuffer, 1024, "%d", internalResizeWidth);
    if(FSDK_SetTrackerParameter(tracker, "InternalResizeWidth", parameterBuffer) != FSDKE_OK){
      printf("set/TrackerPameter fail 2\n");
      result = RESULT_FAIL;
    }
    
    snprintf(parameterBuffer, 1024, "%d", faceDetectionThreshold);
    if(FSDK_SetTrackerParameter(tracker, "FaceDetectionThreshold", parameterBuffer) != FSDKE_OK) {
      printf("set/TrackerPameter fail 3\n");
      result = RESULT_FAIL;
    }
    
    //int errorPosition = -1;
    //const int setResult = FSDK_SetTrackerMultipleParameters(tracker, "HandleArbitraryRotations=true; DetermineFaceRotationAngle=false; InternalResizeWidth=960; FaceDetectionThreshold=1;", &errorPosition);
    //printf("setResult:%d ErrorPosition: %d ", setResult, errorPosition);
    
    FSDK_GetTrackerParameter(tracker, "HandleArbitraryRotations", &parameterBuffer[0], 1024);
    printf("HandleArbitraryRotations:%s ", parameterBuffer);
    
    FSDK_GetTrackerParameter(tracker, "DetermineFaceRotationAngle", &parameterBuffer[0], 1024);
    printf("DetermineFaceRotationAngle:%s ", parameterBuffer);
    
    FSDK_GetTrackerParameter(tracker, "InternalResizeWidth", &parameterBuffer[0], 1024);
    printf("InternalResizeWidth:%s ", parameterBuffer);
    
    FSDK_GetTrackerParameter(tracker, "FaceDetectionThreshold", &parameterBuffer[0], 1024);
    printf("FaceDetectionThreshold:%s ", parameterBuffer);
    
    printf("set/TrackerPameter suceeded\n");
    
    return result;
  } // SetRecognitionParameters()

  Result RecognizeFaces(const cv::Mat_<u8> &image, std::vector<Face> &faces, const char * knownName)
  {
    Result lastResult;
    
    faces.clear();
    
    if((lastResult = InitializeTracker()) != RESULT_OK) {
      return RESULT_FAIL;
    }
    
    HImage imageHandle = OpencvToFsdk(image);
    
    /*
    char parameterBuffer[1024];
    
    FSDK_GetTrackerParameter(tracker, "HandleArbitraryRotations", &parameterBuffer[0], 1024);
    printf("HandleArbitraryRotations:%s ", parameterBuffer);
    
    FSDK_GetTrackerParameter(tracker, "DetermineFaceRotationAngle", &parameterBuffer[0], 1024);
    printf("DetermineFaceRotationAngle:%s ", parameterBuffer);
    
    FSDK_GetTrackerParameter(tracker, "InternalResizeWidth", &parameterBuffer[0], 1024);
    printf("InternalResizeWidth:%s ", parameterBuffer);
    
    FSDK_GetTrackerParameter(tracker, "FaceDetectionThreshold", &parameterBuffer[0], 1024);
    printf("FaceDetectionThreshold:%s ", parameterBuffer);
    */
    
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
      newFace.faceId = IDs[iFace];
      newFace.name = "";

      char name[1024];
      int res = FSDK_GetAllNames(tracker, IDs[iFace], name, sizeof(name));
      
      if (0 == res && strlen(name) > 0) { // draw name
        newFace.name = name;
      }
      
      faces.push_back(newFace);
    } // for(size_t iFace=0; iFace<faceCount; iFace++)

    if(knownName) {
      s32 largestWidth = 0;
      s32 bestId = -1;
      for(size_t iFace=0; iFace<faceCount; iFace++) {
        if(faces[iFace].w > largestWidth) {
          largestWidth = faces[iFace].w;
          bestId = iFace;
        }
      } // for(size_t iFace=0; iFace<faceCount; iFace++)
      
      if(bestId >= 0) {
        FSDK_SetName(tracker, IDs[bestId], knownName);
        faces[bestId].name = knownName;
      }
    } // if(knownName)
 
    FSDK_FreeImage(imageHandle);
  
    return RESULT_OK;
  } // RecognizeFaces()
} // namespace Anki

#endif // #ifndef ROBOT_HARDWARE
