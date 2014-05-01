#ifndef ROBOT_HARDWARE

#define COZMO_ROBOT

#include "communication.h"
#include "threadSafeQueue.h"
#include "debugStreamClient.h"

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/serialize.h"
#include "anki/common/robot/errorHandling.h"

#include <ctime>

#include "opencv/cv.h"

using namespace Anki::Embedded;

static void printUsage()
{
} // void printUsage()

int main(int argc, char ** argv)
{
  // Comment out to use serial
#define USE_SOCKET

  const char outputFilenamePattern[DebugStreamClient::ObjectToSave::SAVE_FILENAME_PATTERN_LENGTH] = "C:/datasets/systemTestImages/cozmo_date%04d_%02d_%02d_time%02d_%02d_%02d_frame%d.%s";

  const f64 waitBeforeStarting = 10.0; // Wait a few seconds before starting saving, to flush the buffer

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

  time_t t = time(0);   // get time now
  struct tm * lastTime = localtime(&t);

  s32 last_tm_sec = lastTime->tm_sec;
  s32 last_tm_min = lastTime->tm_min;
  s32 last_tm_hour = lastTime->tm_hour;

  char outputFilename[DebugStreamClient::ObjectToSave::SAVE_FILENAME_PATTERN_LENGTH];

  s32 rawFrameNumber = -1;
  s32 frameNumber = 0;

  const f64 startTime = GetTimeF64();

  while(true) {
    rawFrameNumber++;

    DebugStreamClient::Object newObject = parserThread.GetNextObject();
    //printf("Received %s %s\n", newObject.typeName, newObject.newObject.objectName);

    const f64 waitingTime = (startTime + waitBeforeStarting) - GetTimeF64();
    if(waitingTime > 0.0 )
    {
      //if(rawFrameNumber % 5 == 0)
      {
        Array<u8> *imageRaw = (reinterpret_cast<Array<u8>*>(newObject.startOfPayload));

        if(imageRaw->IsValid()) {
          Array<u8> image = *imageRaw;
          const cv::Mat_<u8> &refMat = image.get_CvMat_();
          cv::imshow("Robot Image", image.get_CvMat_());
          cv::waitKey(1);
        }
      }

      printf("Waiting for %f more seconds\n", waitingTime);

      free(newObject.buffer); newObject.buffer = NULL;
      continue;
    }

    if(strcmp(newObject.typeName, "Array") == 0 && strcmp(newObject.objectName, "Robot Image") == 0) {
      const time_t t = time(0);   // get time now
      const struct tm * currentTime = localtime(&t);
      if(last_tm_sec == currentTime->tm_sec && last_tm_min == currentTime->tm_min && last_tm_hour == currentTime->tm_hour) {
        frameNumber++;
      } else {
        frameNumber = 0;
      }

      snprintf(&outputFilename[0], DebugStreamClient::ObjectToSave::SAVE_FILENAME_PATTERN_LENGTH, outputFilenamePattern,
        currentTime->tm_year+1900, currentTime->tm_mon+1, currentTime->tm_mday,
        currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec,
        frameNumber,
        "png");

      last_tm_sec = currentTime->tm_sec;
      last_tm_min = currentTime->tm_min;
      last_tm_hour = currentTime->tm_hour;

      printf("Saving %s\n", outputFilename);

      Array<u8> *imageRaw = (reinterpret_cast<Array<u8>*>(newObject.startOfPayload));

      if(imageRaw->IsValid()) {
        Array<u8> image = *(reinterpret_cast<Array<u8>*>(newObject.startOfPayload));
        cv::imshow("Robot Image", image.get_CvMat_());

        parserThread.SaveObject(newObject, outputFilename);

        cv::waitKey(10);
      }
    }
  }

  return 0;
} // int main()
#endif // #ifndef ROBOT_HARDWARE
