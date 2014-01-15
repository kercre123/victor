#ifndef ROBOT_HARDWARE

#ifndef _MSC_VER
#error Currently, only visual c++ is supported
#endif

#include "serial.h"
#include "threadSafeQueue.h"

#include "anki/common/robot/utilities.h"
#include "anki/cozmo/messages.h"
#include "anki/common/robot/serialize.h"

#undef printf
_Check_return_opt_ _CRTIMP int __cdecl printf(_In_z_ _Printf_format_string_ const char * _Format, ...);

#include <queue>

#include <tchar.h>
#include <strsafe.h>

#define BIG_BUFFER_SIZE 100000000

using namespace std;

volatile double lastUpdateTime;
volatile HANDLE lastUpdateTime_mutex;

const double secondsToWaitBeforeSavingABuffer = 1.0;

typedef struct {
  void * data;
  s32 dataLength;
} RawBuffer;

void ExtractUSBMessage(const void * rawBuffer, const s32 rawBufferLength, s32 &startIndex, s32 &endIndex)
{
  const u8 * rawBufferU8 = reinterpret_cast<const u8*>(rawBuffer);

  s32 state = 0;
  s32 index = 0;

  startIndex = -1;
  endIndex = -1;

  // Look for the header
  while(index < rawBufferLength) {
    if(state == SERIALIZED_BUFFER_HEADER_LENGTH) {
      startIndex = index;
      break;
    }

    if(rawBufferU8[index] == SERIALIZED_BUFFER_HEADER[state]) {
      state++;
    } else if(rawBufferU8[index] == SERIALIZED_BUFFER_HEADER[0]) {
      state = 1;
    } else {
      state = 0;
    }

    index++;
  } // while(index < rawBufferLength)

  // Look for the footer
  state = 0;
  while(index < rawBufferLength) {
    if(state == SERIALIZED_BUFFER_FOOTER_LENGTH) {
      endIndex = index-SERIALIZED_BUFFER_FOOTER_LENGTH-1;
      break;
    }

    //printf("%d) %d %x %x\n", index, state, rawBufferU8[index], SERIALIZED_BUFFER_FOOTER[state]);

    if(rawBufferU8[index] == SERIALIZED_BUFFER_FOOTER[state]) {
      state++;
    } else if(rawBufferU8[index] == SERIALIZED_BUFFER_FOOTER[0]) {
      state = 1;
    } else {
      state = 0;
    }

    index++;
  } // while(index < rawBufferLength)

  if(state == SERIALIZED_BUFFER_FOOTER_LENGTH) {
    endIndex = index-SERIALIZED_BUFFER_FOOTER_LENGTH-1;
  }
} // void ExtractUSBMessage(const void * rawBuffer, s32 &startIndex, s32 &endIndex)

// Based off example at http://msdn.microsoft.com/en-us/library/windows/desktop/ms682516(v=vs.85).aspx
DWORD WINAPI SaveBuffers(LPVOID lpParam)
{
  ThreadSafeQueue<RawBuffer> *buffers = (ThreadSafeQueue<RawBuffer>*)lpParam;

  // The bigBuffer is a 16-bytes aligned version of bigBufferRaw
  static u8 bigBufferRaw[BIG_BUFFER_SIZE];
  static u8 * bigBuffer = reinterpret_cast<u8*>(RoundUp<size_t>(reinterpret_cast<size_t>(&bigBufferRaw[0]), MEMORY_ALIGNMENT));
  s32 bigBufferIndex = 0;

  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

  while(true) {
    WaitForSingleObject(lastUpdateTime_mutex, INFINITE);
    const double lastUpdateTime_local = lastUpdateTime;
    ReleaseMutex(lastUpdateTime_mutex);

    // If we've gone more than a little bit without getting more data, we've probably received the
    // entire message
    if((GetTime()-lastUpdateTime_local) >= secondsToWaitBeforeSavingABuffer) {
      WaitForSingleObject(lastUpdateTime_mutex, INFINITE);
      lastUpdateTime = GetTime() + 1e10;
      ReleaseMutex(lastUpdateTime_mutex);

      s32 startIndex;
      s32 endIndex;
      ExtractUSBMessage(bigBuffer, bigBufferIndex, startIndex, endIndex);

      if(startIndex < 0 || endIndex < 0) {
        printf("Error: USB header or footer is missing (%d,%d)\n", startIndex, endIndex);
        bigBufferIndex = 0;
        continue;
      }

      const s32 messageLength = endIndex-startIndex+1;

      // Move this to a memory-aligned start (index 0 of bigBuffer)
      memmove(bigBuffer, bigBuffer+startIndex, messageLength);

      for(s32 i=0; i<messageLength; i++) {
        printf("%x ", bigBuffer[i]);
      }

      bigBufferIndex = 0;
    }

    if(buffers->IsEmpty()) {
      Sleep(10);
      continue;
    }

    RawBuffer nextPiece = buffers->Pop();

    if((bigBufferIndex+nextPiece.dataLength+2*MEMORY_ALIGNMENT) > BIG_BUFFER_SIZE) {
      printf("Out of memory in bigBuffer\n");
      Sleep(10);
      continue;
    }

    memcpy(&bigBuffer[0] + bigBufferIndex, nextPiece.data, nextPiece.dataLength);
    bigBufferIndex += nextPiece.dataLength;

    free(nextPiece.data); nextPiece.data = NULL;
  } // while(true)

  return 0;
} // DWORD WINAPI PrintfBuffers(LPVOID lpParam)

int main(int argc, char ** argv)
{
  ThreadSafeQueue<RawBuffer> buffers = ThreadSafeQueue<RawBuffer>();
  Serial serial;

  lastUpdateTime_mutex = CreateMutex(NULL, FALSE, NULL);

  if(serial.Open(8, 1000000) != RESULT_OK)
    return -1;

  WaitForSingleObject(lastUpdateTime_mutex, INFINITE);
  lastUpdateTime = GetTime() + 10e10;
  ReleaseMutex(lastUpdateTime_mutex);

  DWORD threadId = -1;
  HANDLE threadHandle = CreateThread(
    NULL,        // default security attributes
    0,           // use default stack size
    SaveBuffers, // thread function name
    &buffers,    // argument to thread function
    0,           // use default creation flags
    &threadId);  // returns the thread identifier

  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

  const s32 bufferLength = 1024;

  void * buffer = malloc(bufferLength);

  while(true) {
    if(!buffer) {
      printf("\n\nCould not allocate buffer\n\n");
      return -4;
    }

    DWORD bytesRead = 0;
    if(serial.Read(buffer, bufferLength-2, bytesRead) != RESULT_OK)
      return -3;

    if(bytesRead > 0) {
      WaitForSingleObject(lastUpdateTime_mutex, INFINITE);
      lastUpdateTime = GetTime();
      ReleaseMutex(lastUpdateTime_mutex);

      RawBuffer newBuffer;
      newBuffer.data = buffer;
      newBuffer.dataLength = bytesRead;

      buffers.Push(newBuffer);

      buffer = malloc(bufferLength);
    }
  }

  if(serial.Close() != RESULT_OK)
    return -2;

  return 0;
}
#endif