#ifndef ROBOT_HARDWARE

#define COZMO_ROBOT

#include "communication.h"
#include "threadSafeQueue.h"
#include "messageHandling.h"

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/serialize.h"

#ifdef _MSC_VER
#include <tchar.h>
#include <strsafe.h>

#undef printf
_Check_return_opt_ _CRTIMP int __cdecl printf(_In_z_ _Printf_format_string_ const char * _Format, ...);

#else // #ifdef _MSC_VER

#include <unistd.h>
#include <pthread.h>

#endif // #ifdef _MSC_VER ... #else

#include <queue>

#include <ctime>

#include "opencv/cv.h"

#include "messageHandling.h"

using namespace std;
using namespace Anki;
using namespace Anki::Embedded;

static void printUsage()
{
} // void printUsage()

int main(int argc, char ** argv)
{
  // TCP
  const char * ipAddress = "192.168.3.30";
  const s32 port = 5551;

  printf("Starting display\n");

  DebugStreamParserThread parserThread(ipAddress, port);

  while(true) {
    DebugStreamParserThread::Object newObject = parserThread.GetNextObject();
    printf("Received %s %s\n", newObject.typeName, newObject.objectName);
  }

  //
  //  const s32 USB_BUFFER_SIZE = useTcp ? 100000 : 5000;
  //  const s32 MESSAGE_BUFFER_SIZE = 1000000;
  //
  //  u8 *usbBuffer = reinterpret_cast<u8*>(malloc(USB_BUFFER_SIZE));
  //
  //  DisplayRawBuffer nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
  //
  //  ThreadSafeQueue<DisplayRawBuffer> messageQueue = ThreadSafeQueue<DisplayRawBuffer>();
  //
  //  if(argc == 1) {
  //    // just use defaults, but print the help anyway
  //    // printUsage();
  //  }
  //  //else if(argc == 3) {
  //  //  sscanf(argv[1], "%d", &comPort);
  //  //    sscanf(argv[2], "%d", &baudRate);
  //  //}
  //  else {
  //    // printUsage();
  //    return -1;
  //  }
  //
  //  if(useTcp) {
  //    while(socket.Open(ipAddress, port) != RESULT_OK) {
  //      printf("Trying again to open socket.\n");
  //#ifdef _MSC_VER
  //      Sleep(100);
  //#else
  //      usleep(100000);
  //#endif
  //    }
  //  } else {
  //#ifdef _MSC_VER
  //    if(serial.Open(comPort, baudRate) != RESULT_OK)
  //      return -2;
  //#else
  //    printf("Error: serial is only supported on Windows\n");
  //#endif
  //  }
  //
  //  printf("Connection opened\n");
  //
  //  lastUpdateTime = GetTime() + 10e10;
  //
  //#ifdef _MSC_VER
  //  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST); // THREAD_PRIORITY_ABOVE_NORMAL
  //
  //  DWORD threadId = -1;
  //  CreateThread(
  //    NULL,        // default security attributes
  //    0,           // use default stack size
  //    DisplayBuffersThread, // thread function name
  //    &messageQueue,    // argument to thread function
  //    0,           // use default creation flags
  //    &threadId);  // returns the thread identifier
  //#else
  //  // TODO: set thread priority
  //
  //  pthread_t thread;
  //  pthread_attr_t attr;
  //  pthread_attr_init(&attr);
  //
  //  pthread_create(&thread, &attr, DisplayBuffersThread, (void *)&messageQueue);
  //#endif
  //
  //  printf("Parsing thread created\n");
  //
  //  bool atLeastOneStartFound = false;
  //  s32 start_state = 0;
  //
  //  while(true) {
  //    if(!usbBuffer) {
  //      printf("\n\nCould not allocate usbBuffer\n\n");
  //      return -3;
  //    }
  //
  //    s32 bytesRead = 0;
  //
  //    if(useTcp) {
  //      while(socket.Read(usbBuffer, USB_BUFFER_SIZE-2, bytesRead) != RESULT_OK)
  //      {
  //        printf("socket read failure. Retrying...\n");
  //#ifdef _MSC_VER
  //        Sleep(1);
  //#else
  //        usleep(1000);
  //#endif
  //      }
  //    } else {
  //#ifdef _MSC_VER
  //      if(serial.Read(usbBuffer, USB_BUFFER_SIZE-2, bytesRead) != RESULT_OK)
  //        return -4;
  //#else
  //      return -4;
  //#endif
  //    }
  //
  //    if(bytesRead == 0) {
  //#ifdef _MSC_VER
  //      Sleep(1);
  //#else
  //      usleep(1000);
  //#endif
  //      continue;
  //    }
  //
  //    // Find the next SERIALIZED_BUFFER_HEADER
  //    s32 start_searchIndex = 0;
  //    s32 start_foundIndex = -1;
  //
  //    // This method can only find one message per usbBuffer
  //    // TODO: support more
  //    while(start_searchIndex < static_cast<s32>(bytesRead)) {
  //      if(start_state == SERIALIZED_BUFFER_HEADER_LENGTH) {
  //        start_foundIndex = start_searchIndex;
  //        start_state = 0;
  //        break;
  //      }
  //
  //      if(usbBuffer[start_searchIndex] == SERIALIZED_BUFFER_HEADER[start_state]) {
  //        start_state++;
  //      } else if(usbBuffer[start_searchIndex] == SERIALIZED_BUFFER_HEADER[0]) {
  //        start_state = 1;
  //      } else {
  //        start_state = 0;
  //      }
  //
  //      start_searchIndex++;
  //    } // while(start_searchIndex < static_cast<s32>(bytesRead))
  //
  //    // If we found a start header, handle it
  //    if(start_foundIndex != -1) {
  //      if(atLeastOneStartFound) {
  //        const s32 numBytesToCopy = start_foundIndex;
  //
  //        if((nextMessage.curDataLength + numBytesToCopy + 16) > nextMessage.maxDataLength) {
  //          nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
  //          printf("Buffer trashed\n");
  //          continue;
  //        }
  //
  //        memcpy(
  //          nextMessage.data + nextMessage.curDataLength,
  //          usbBuffer,
  //          numBytesToCopy);
  //
  //        nextMessage.curDataLength += numBytesToCopy;
  //
  //        //ProcessRawBuffer(nextMessage, true, false);
  //        messageQueue.Push(nextMessage);
  //
  //        nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
  //      } else {
  //        atLeastOneStartFound = true;
  //      }
  //
  //      const s32 numBytesToCopy = static_cast<s32>(bytesRead) - start_foundIndex;
  //
  //      // If we've filled up the buffer, just trash it
  //      if((nextMessage.curDataLength + numBytesToCopy + 16) > nextMessage.maxDataLength) {
  //        nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
  //        printf("Buffer trashed\n");
  //        continue;
  //      }
  //
  //      if(numBytesToCopy <= 0) {
  //        printf("negative numBytesToCopy");
  //        continue;
  //      }
  //
  //      //for(s32 i=0; i<50; i++) {
  //      //  printf("%d ", *(usbBuffer + start_foundIndex + i));
  //      //}
  //      //printf("\n");
  //
  //      memcpy(
  //        nextMessage.data + nextMessage.curDataLength,
  //        usbBuffer + start_foundIndex,
  //        numBytesToCopy);
  //
  //      nextMessage.curDataLength += numBytesToCopy;
  //    } else {// if(start_foundIndex != -1)
  //      if(atLeastOneStartFound) {
  //        const s32 numBytesToCopy = static_cast<s32>(bytesRead);
  //
  //        // If we've filled up the buffer, just trash it
  //        if((nextMessage.curDataLength + numBytesToCopy + 16) > nextMessage.maxDataLength) {
  //          nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
  //          printf("Buffer trashed\n");
  //          continue;
  //        }
  //
  //        memcpy(
  //          nextMessage.data + nextMessage.curDataLength,
  //          usbBuffer,
  //          numBytesToCopy);
  //
  //        nextMessage.curDataLength += numBytesToCopy;
  //      }
  //    }
  //  } // while(true)
  //
  //  if(useTcp) {
  //    if(socket.Close() != RESULT_OK)
  //      return -5;
  //  } else {
  //#ifdef _MSC_VER
  //    if(serial.Close() != RESULT_OK)
  //      return -5;
  //#endif
  //  }

  return 0;
} // int main()
#endif // #ifndef ROBOT_HARDWARE
