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

static std::vector<VisionMarker> visionMarkerList;

static std::vector<Anki::Embedded::Rectangle<s32> > detectedFaces;

static bool aMessageAlreadyPrinted;
static bool isTracking;

static vector<DebugStreamClient::Object> currentObjects;

static void printUsage()
{
} // void printUsage()

int main(int argc, char ** argv)
{
  // Comment out to use serial
#define USE_SOCKET

  const char outputFilenamePattern[outputFilenamePatternLength] = "C:/datasets/systemTestImages/cozmo_date%04d_%02d_%02d_time%02d_%02d_%02d_frame%d.%s";

  printf("Starting display\n");
  SetLogSilence(true);

#ifdef USE_SOCKET
  // TCP
  const char * ipAddress = "192.168.3.33";
  const s32 port = 5551;
  DebugStreamClient parserThread(ipAddress, port);
#else
  const s32 comPort = 11;
  const s32 baudRate = 1000000;
  DebugStreamClient parserThread(comPort, baudRate);
#endif

  const time_t t = time(0);   // get time now
  struct tm * lastTime = localtime(&t);

  char outputFilename[DebugStreamClient::ObjectToSave::SAVE_FILENAME_PATTERN_LENGTH];

  s32 frameNumber = 0;

  while(true) {
    DebugStreamClient::Object newObject = parserThread.GetNextObject();
    //printf("Received %s %s\n", newObject.typeName, newObject.newObject.objectName);

    if(strcmp(newObject.typeName, "Array") == 0 && strcmp(newObject.objectName, "Robot Image") == 0) {
      const time_t t = time(0);   // get time now
      const struct tm * currentTime = localtime(&t);
      if(lastTime->tm_sec == currentTime->tm_sec && lastTime->tm_min == currentTime->tm_min && lastTime->tm_year == currentTime->tm_year) {
        frameNumber++;
      } else {
        frameNumber = 0;
      }

      snprintf(&outputFilename[0], DebugStreamClient::ObjectToSave::SAVE_FILENAME_PATTERN_LENGTH, outputFilenamePattern,
        currentTime->tm_year+1900, currentTime->tm_mon+1, currentTime->tm_mday,
        currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec,
        frameNumber,
        "png");

      parserThread.SaveObject(newObject, outputFilename);
    }
  }

  return 0;
} // int main()
#endif // #ifndef ROBOT_HARDWARE
