#ifndef ROBOT_HARDWARE

#ifndef _MSC_VER
#error Currently, only visual c++ is supported
#endif

#define COZMO_ROBOT

#include "serial.h"
#include "threadSafeQueue.h"
#include "messageHandling.h"

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/serialize.h"

#undef printf
_Check_return_opt_ _CRTIMP int __cdecl printf(_In_z_ _Printf_format_string_ const char * _Format, ...);

#include <queue>

#include <tchar.h>
#include <strsafe.h>
#include <ctime>

#include "opencv/cv.h"

using namespace std;

const double secondsToWaitBeforeSavingABuffer = 1.0;

const s32 outputFilenamePatternLength = 1024;
//char outputFilenamePattern[outputFilenamePatternLength] = "C:/datasets/systemTestImages/cozmo_%04d-%02d-%02d_%02d-%02d-%02d_%d.%s";
char outputFilenamePattern[outputFilenamePatternLength] = "C:/datasets/systemTestImages/cozmo_date%04d_%02d_%02d_time%02d_%02d_%02d_frame%d.%s";

// Based off example at http://msdn.microsoft.com/en-us/library/windows/desktop/ms682516(v=vs.85).aspx
DWORD WINAPI SaveBuffersThread(LPVOID lpParam)
{
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

  /*RawBuffer *buffer = (RawBuffer*)lpParam;

  ProcessRawBuffer(*buffer, string(outputFilenamePattern), true, BUFFER_ACTION_SAVE, true);*/

  return 0;
} // DWORD WINAPI PrintfBuffers(LPVOID lpParam)

void printUsage()
{
  printf(
    "usage: saveVideo <comPort> <baudRate> <outputPatternString>\n"
    "example: saveVideo 8 1000000 C:/datasets/systemTestImages/cozmo_%%04d-%%02d-%%02d_%%02d-%%02d-%%02d_%%d.%%s\n");
} // void printUsage()

int main(int argc, char ** argv)
{
  double lastUpdateTime;
  Serial serial;
  const s32 USB_BUFFER_SIZE = 100000000;
  u8 *usbBuffer = reinterpret_cast<u8*>(malloc(USB_BUFFER_SIZE));
  s32 usbBufferIndex = 0;

  s32 comPort = 11;
  s32 baudRate = 1000000;

  if(argc == 1) {
    // just use defaults, but print the help anyway
    printUsage();
  } else if(argc == 4) {
    sscanf(argv[1], "%d", &comPort);
    sscanf(argv[2], "%d", &baudRate);
    strcpy(outputFilenamePattern, argv[3]);
  } else {
    printUsage();
    return -1;
  }

  if(serial.Open(comPort, baudRate) != RESULT_OK)
    return -2;

  lastUpdateTime = GetTime() + 10e10;

  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST); // THREAD_PRIORITY_ABOVE_NORMAL

  while(true) {
    if(!usbBuffer) {
      printf("\n\nCould not allocate usbBuffer\n\n");
      return -3;
    }

    DWORD bytesRead = 0;
    if(serial.Read(usbBuffer+usbBufferIndex, USB_BUFFER_SIZE-usbBufferIndex-2, bytesRead) != RESULT_OK)
      return -4;

    usbBufferIndex += bytesRead;

    if(bytesRead > 0) {
      lastUpdateTime = GetTime();
    } else {
      if((GetTime()-lastUpdateTime) > secondsToWaitBeforeSavingABuffer) {
        lastUpdateTime = GetTime();

        if(usbBufferIndex > 0) {
          printf("Received %d bytes\n", usbBufferIndex);
          RawBuffer rawBuffer;
          rawBuffer.data = reinterpret_cast<u8*>(usbBuffer);
          rawBuffer.dataLength = usbBufferIndex;

          // Use a seperate thread
          /*
          DWORD threadId = -1;
          CreateThread(
          NULL,        // default security attributes
          0,           // use default stack size
          SaveBuffersThread, // thread function name
          &rawBuffer,    // argument to thread function
          0,           // use default creation flags
          &threadId);  // returns the thread identifier
          */

          // Just call the function
          ProcessRawBuffer_Save(rawBuffer, string(outputFilenamePattern), true);

          usbBuffer = reinterpret_cast<u8*>(malloc(USB_BUFFER_SIZE));
          usbBufferIndex = 0;
        }
      } else {
        //Sleep(1);
      }
    }
  }

  if(serial.Close() != RESULT_OK)
    return -5;

  return 0;
}
#endif // #ifndef ROBOT_HARDWARE