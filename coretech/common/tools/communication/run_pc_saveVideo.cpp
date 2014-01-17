#ifndef ROBOT_HARDWARE

#ifndef _MSC_VER
#error Currently, only visual c++ is supported
#endif

#include "serial.h"
#include "threadSafeQueue.h"

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"
#include "anki/cozmo/messages.h"
#include "anki/common/robot/serialize.h"

#undef printf
_Check_return_opt_ _CRTIMP int __cdecl printf(_In_z_ _Printf_format_string_ const char * _Format, ...);

#include <queue>

#include <tchar.h>
#include <strsafe.h>
#include <ctime>

#include "opencv/cv.h"

//#define PRINTF_ALL_RECEIVED

#define BIG_BUFFER_SIZE 100000000

using namespace std;

volatile double lastUpdateTime;
volatile HANDLE lastUpdateTime_mutex;

const double secondsToWaitBeforeSavingABuffer = 1.0;

const s32 outputFilenamePatternLength = 1024;
char outputFilenamePattern[outputFilenamePatternLength] = "C:/datasets/cozmoShort/cozmo_%04d-%02d-%02d_%02d-%02d-%02d_%d.%s";

typedef struct {
  void * data;
  s32 dataLength;
} RawBuffer;

typedef struct {
	ThreadSafeQueue<RawBuffer> *buffers;
	string outputFilenamePattern;
} ThreadParameters;

void FindUSBMessage(const void * rawBuffer, const s32 rawBufferLength, s32 &startIndex, s32 &endIndex)
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
} // void FindUSBMessage(const void * rawBuffer, s32 &startIndex, s32 &endIndex)

// Based off example at http://msdn.microsoft.com/en-us/library/windows/desktop/ms682516(v=vs.85).aspx
DWORD WINAPI SaveBuffers(LPVOID lpParam)
{
  const bool swapEndianForHeaders = true;
  const bool swapEndianForContents = true;

  const s32 outputFilenameLength = 1024;
  char outputFilename[outputFilenameLength];

  ThreadParameters *params = (ThreadParameters*)lpParam;
  //ThreadSafeQueue<RawBuffer> *buffers = (ThreadSafeQueue<RawBuffer>*)lpParam;
  ThreadSafeQueue<RawBuffer> *buffers = params->buffers;

  // The bigBuffer is a 16-bytes aligned version of bigBufferRaw
  static u8 bigBufferRaw[BIG_BUFFER_SIZE];
  static u8 bigBufferRaw2[BIG_BUFFER_SIZE];
  static u8 * bigBuffer = reinterpret_cast<u8*>(RoundUp<size_t>(reinterpret_cast<size_t>(&bigBufferRaw[0]), MEMORY_ALIGNMENT));
  s32 bigBufferIndex = 0;

  memset(&bigBufferRaw[0], 0, BIG_BUFFER_SIZE);

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

      const time_t t = time(0);   // get time now
      const struct tm * currentTime = localtime(&t);

      s32 frameNumber = 0;

      MemoryStack memory(bigBufferRaw2, BIG_BUFFER_SIZE, Flags::Buffer(false, true, false));

      s32 usbMessageStartIndex;
      s32 usbMessageEndIndex;
      FindUSBMessage(bigBuffer, bigBufferIndex, usbMessageStartIndex, usbMessageEndIndex);

      if(usbMessageStartIndex < 0 || usbMessageEndIndex < 0) {
        printf("Error: USB header or footer is missing (%d,%d) in message of size %d bytes\n", usbMessageStartIndex, usbMessageEndIndex, bigBufferIndex);
        bigBufferIndex = 0;
        bigBuffer = reinterpret_cast<u8*>(RoundUp<size_t>(reinterpret_cast<size_t>(&bigBufferRaw[0]), MEMORY_ALIGNMENT));
        continue;
      }

#ifdef PRINTF_ALL_RECEIVED
      printf("orig: ");
      for(s32 i=0; i<bigBufferIndex; i++) {
        printf("%x ", bigBuffer[i]);
      }
      printf("\n");
#endif

      const s32 messageLength = usbMessageEndIndex-usbMessageStartIndex+1;

      // Move this to a memory-aligned start (index 0 of bigBuffer), after the first MemoryStack::HEADER_LENGTH
      const s32 bigBuffer_alignedStartIndex = MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH;
      memmove(bigBuffer+bigBuffer_alignedStartIndex, bigBuffer+usbMessageStartIndex, messageLength);
      bigBuffer += bigBuffer_alignedStartIndex;

      if(swapEndianForHeaders) {
        for(s32 i=0; i<messageLength; i+=4) {
          Swap(bigBuffer[i], bigBuffer[i^3]);
          Swap(bigBuffer[(i+1)], bigBuffer[(i+1)^3]);
        }
      }

#ifdef PRINTF_ALL_RECEIVED
      printf("shifted: ");
      for(s32 i=0; i<messageLength; i++) {
        printf("%x ", bigBuffer[i]);
      }
      printf("\n");
#endif

      SerializedBuffer serializedBuffer(bigBuffer, messageLength, Anki::Embedded::Flags::Buffer(false, true, true));

      SerializedBufferIterator iterator(serializedBuffer);

      printf("\n");
      printf("\n");
      while(iterator.HasNext()) {
        s32 dataLength;
        SerializedBuffer::DataType type;
        u8 * const dataSegmentStart = reinterpret_cast<u8*>(iterator.GetNext(swapEndianForHeaders, dataLength, type));
        u8 * dataSegment = dataSegmentStart;

        printf("Next segment is (%d,%d)\n", dataLength, type);
        if(type == SerializedBuffer::DATA_TYPE_RAW) {
          printf("Raw segment: ");
          for(s32 i=0; i<dataLength; i++) {
            printf("%x ", dataSegment[i]);
          }
        } else if(type == SerializedBuffer::DATA_TYPE_BASIC_TYPE_BUFFER) {
          SerializedBuffer::EncodedBasicTypeBuffer code;
          for(s32 i=0; i<SerializedBuffer::EncodedBasicTypeBuffer::CODE_SIZE; i++) {
            code.code[i] = reinterpret_cast<const u32*>(dataSegment)[i];
          }
          dataSegment += SerializedBuffer::EncodedBasicTypeBuffer::CODE_SIZE * sizeof(u32);
          const s32 remainingDataLength = dataLength - SerializedBuffer::EncodedBasicTypeBuffer::CODE_SIZE * sizeof(u32);

          u8 size;
          bool isInteger;
          bool isSigned;
          bool isFloat;
          s32 numElements;
          SerializedBuffer::DecodeBasicTypeBuffer(swapEndianForHeaders, code, size, isInteger, isSigned, isFloat, numElements);

          printf("Basic type buffer segment (%d, %d, %d, %d, %d): ", size, isInteger, isSigned, isFloat, numElements);
          for(s32 i=0; i<remainingDataLength; i++) {
            printf("%x ", dataSegment[i]);
          }
        } else if(type == SerializedBuffer::DATA_TYPE_ARRAY) {
          SerializedBuffer::EncodedArray code;
          for(s32 i=0; i<SerializedBuffer::EncodedArray::CODE_SIZE; i++) {
            code.code[i] = reinterpret_cast<const u32*>(dataSegment)[i];
          }
          dataSegment += SerializedBuffer::EncodedArray::CODE_SIZE * sizeof(u32);
          const s32 remainingDataLength = dataLength - SerializedBuffer::EncodedArray::CODE_SIZE * sizeof(u32);

          s32 height;
          s32 width;
          s32 stride;
          Flags::Buffer flags;
          u8 basicType_size;
          bool basicType_isInteger;
          bool basicType_isSigned;
          bool basicType_isFloat;
          SerializedBuffer::DecodeArrayType(swapEndianForHeaders, code, height, width, stride, flags, basicType_size, basicType_isInteger, basicType_isSigned, basicType_isFloat);

          printf("Array: (%d, %d, %d, %d, %d, %d, %d, %d) ", height, width, stride, flags, basicType_size, basicType_isInteger, basicType_isSigned, basicType_isFloat);
          //template<typename Type> static Result DeserializeArray(const void * data, const s32 dataLength, Array<Type> &out, MemoryStack &memory);
          if(basicType_size==1 && basicType_isInteger==1 && basicType_isSigned==0 && basicType_isFloat==0) {
            Array<u8> arr;
            SerializedBuffer::DeserializeArray(swapEndianForHeaders, dataSegmentStart, dataLength, arr, memory);

            snprintf(&outputFilename[0], outputFilenameLength, params->outputFilenamePattern.data(),
              currentTime->tm_year+1900, currentTime->tm_mon+1, currentTime->tm_mday,
              currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec,
              frameNumber,
              "png");

            frameNumber++;

            printf("Saving to %s\n", outputFilename);
            const cv::Mat_<u8> &mat = arr.get_CvMat_();
            cv::imwrite(outputFilename, mat);
			//cv::imwrite("c:/tmp/tt.bmp", mat);
          }
        }

        printf("\n");
      }

      bigBufferIndex = 0;
      bigBuffer = reinterpret_cast<u8*>(RoundUp<size_t>(reinterpret_cast<size_t>(&bigBufferRaw[0]), MEMORY_ALIGNMENT));
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

void printUsage()
{
  printf(
    "usage: saveVideo <comPort> <baudRate> <outputPatternString>\n"
    "example: saveVideo 8 1500000 C:/datasets/cozmoShort/cozmo_%%04d-%%02d-%%02d_%%02d-%%02d-%%02d_%%d.%%s\n");
} // void printUsage()

int main(int argc, char ** argv)
{
  ThreadSafeQueue<RawBuffer> buffers = ThreadSafeQueue<RawBuffer>();
  Serial serial;

  lastUpdateTime_mutex = CreateMutex(NULL, FALSE, NULL);

  s32 comPort = 10;
  s32 baudRate = 1500000;

  if(argc == 1) {
    // just use defaults, but print the help anyway
    printUsage();
  } else if(argc == 4) {
    sscanf(argv[1], "%d", &comPort);
    sscanf(argv[2], "%d", &baudRate);
	strcpy(outputFilenamePattern, argv[3]);
    //snprintf(outputFilenamePattern, outputFilenamePatternLength, "%s", argv[3]);
  } else {
    printUsage();
    return -1;
  }

  if(serial.Open(comPort, baudRate) != RESULT_OK)
    return -1;

  WaitForSingleObject(lastUpdateTime_mutex, INFINITE);
  lastUpdateTime = GetTime() + 10e10;
  ReleaseMutex(lastUpdateTime_mutex);

  ThreadParameters params;
  params.buffers = &buffers;
  params.outputFilenamePattern = string(outputFilenamePattern);

  DWORD threadId = -1;
  HANDLE threadHandle = CreateThread(
    NULL,        // default security attributes
    0,           // use default stack size
    SaveBuffers, // thread function name
    &params,    // argument to thread function
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