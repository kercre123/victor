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

const double secondsToWaitBeforeDisplayingABuffer = 0.1;

const s32 outputFilenamePatternLength = 1024;
char outputFilenamePattern[outputFilenamePatternLength] = "C:/datasets/cozmoShort/cozmo_%04d-%02d-%02d_%02d-%02d-%02d_%d.%s";

// Based off example at http://msdn.microsoft.com/en-us/library/windows/desktop/ms682516(v=vs.85).aspx
DWORD WINAPI DisplayBuffersThread(LPVOID lpParam)
{
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

  //RawBuffer *buffer = (RawBuffer*)lpParam;

  //ProcessRawBuffer(*buffer, string(outputFilenamePattern), true, BUFFER_ACTION_DISPLAY, false);

  return 0;
} // DWORD WINAPI PrintfBuffers(LPVOID lpParam)

static void printUsage()
{
  printf(
    "usage: displayVideo <comPort> <baudRate>\n"
    "example: displayVideo 8 1000000 \n");
} // void printUsage()

static DisplayRawBuffer AllocateNewRawBuffer(const s32 bufferRawSize)
{
  DisplayRawBuffer rawBuffer;

  rawBuffer.rawDataPointer = reinterpret_cast<u8*>(malloc(bufferRawSize));
  rawBuffer.data = reinterpret_cast<u8*>( RoundUp(reinterpret_cast<size_t>(rawBuffer.rawDataPointer), MEMORY_ALIGNMENT) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH );
  rawBuffer.maxDataLength = bufferRawSize - (reinterpret_cast<size_t>(rawBuffer.data) - reinterpret_cast<size_t>(rawBuffer.rawDataPointer));
  rawBuffer.curDataLength = 0;

  if(rawBuffer.rawDataPointer == NULL) {
    printf("Could not allocate memory");
    rawBuffer.data = NULL;
    rawBuffer.maxDataLength = 0;
  }

  return rawBuffer;
}

int main(int argc, char ** argv)
{
  double lastUpdateTime;
  Serial serial;
  const s32 USB_BUFFER_SIZE = 1024;
  const s32 MESSAGE_BUFFER_SIZE = 1000000;

  u8 *usbBuffer = reinterpret_cast<u8*>(malloc(USB_BUFFER_SIZE));

  DisplayRawBuffer nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);

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

  bool atLeastOneStartFound = false;
  s32 start_state = 0;

  while(true) {
    if(!usbBuffer) {
      printf("\n\nCould not allocate usbBuffer\n\n");
      return -3;
    }

    DWORD bytesRead = 0;
    if(serial.Read(usbBuffer, USB_BUFFER_SIZE-2, bytesRead) != RESULT_OK)
      return -4;

    // Find the next SERIALIZED_BUFFER_HEADER
    s32 start_searchIndex = 0;
    s32 start_foundIndex = -1;

    // This method can only find one message per usbBuffer
    // TODO: support more
    while(start_searchIndex < static_cast<s32>(bytesRead)) {
      if(start_state == SERIALIZED_BUFFER_HEADER_LENGTH) {
        start_foundIndex = start_searchIndex;
        start_state = 0;
        break;
      }

      if(usbBuffer[start_searchIndex] == SERIALIZED_BUFFER_HEADER[start_state]) {
        start_state++;
      } else if(usbBuffer[start_searchIndex] == SERIALIZED_BUFFER_HEADER[0]) {
        start_state = 1;
      } else {
        start_state = 0;
      }

      start_searchIndex++;
    } // while(start_searchIndex < USB_BUFFER_SIZE)

    // If we found a start header, handle it
    if(start_foundIndex != -1) {
      if(atLeastOneStartFound) {
        const s32 numBytesToCopy = start_foundIndex;

        if((nextMessage.curDataLength + numBytesToCopy + 16) > nextMessage.maxDataLength) {
          nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
          printf("Buffer trashed\n");
          continue;
        }

        memcpy(
          nextMessage.data + nextMessage.curDataLength,
          usbBuffer,
          numBytesToCopy);

        nextMessage.curDataLength += numBytesToCopy;

        ProcessRawBuffer_Display(nextMessage, true, false);

        nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
      } else {
        atLeastOneStartFound = true;
      }

      const s32 numBytesToCopy = static_cast<s32>(bytesRead) - start_foundIndex;

      // If we've filled up the buffer, just trash it
      if((nextMessage.curDataLength + numBytesToCopy + 16) > nextMessage.maxDataLength) {
        nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
        printf("Buffer trashed\n");
        continue;
      }

      if(numBytesToCopy <= 0) {
        printf("negative numBytesToCopy");
        continue;
      }

      //for(s32 i=0; i<50; i++) {
      //  printf("%d ", *(usbBuffer + start_foundIndex + i));
      //}
      //printf("\n");

      memcpy(
        nextMessage.data + nextMessage.curDataLength,
        usbBuffer + start_foundIndex,
        numBytesToCopy);

      nextMessage.curDataLength += numBytesToCopy;
    } else {// if(start_foundIndex != -1)
      if(atLeastOneStartFound) {
        const s32 numBytesToCopy = static_cast<s32>(bytesRead);

        // If we've filled up the buffer, just trash it
        if((nextMessage.curDataLength + numBytesToCopy + 16) > nextMessage.maxDataLength) {
          nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
          printf("Buffer trashed\n");
          continue;
        }

        memcpy(
          nextMessage.data + nextMessage.curDataLength,
          usbBuffer,
          numBytesToCopy);

        nextMessage.curDataLength += numBytesToCopy;
      }
    }
  } // while(true)

  if(serial.Close() != RESULT_OK)
    return -5;

  return 0;
} // int main()
#endif // #ifndef ROBOT_HARDWARE