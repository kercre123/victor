#ifndef ROBOT_HARDWARE

#ifndef _MSC_VER
#error Currently, only visual c++ is supported
#endif

#define COZMO_ROBOT

#include "serial.h"
#include "threadSafeQueue.h"

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"
#include "anki/cozmo/robot/messages.h"
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

const bool swapEndianForHeaders = true;
const bool swapEndianForContents = true;

using namespace std;

const double secondsToWaitBeforeSavingABuffer = 1.0;

const s32 outputFilenamePatternLength = 1024;
char outputFilenamePattern[outputFilenamePatternLength] = "C:/datasets/cozmoShort/cozmo_%04d-%02d-%02d_%02d-%02d-%02d_%d.%s";

typedef struct {
  u8 * data;
  s32 dataLength;
} RawBuffer;

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

void SaveAndFreeBuffer(RawBuffer &buffer, const string outputFilenamePattern)
{
  const s32 outputFilenameLength = 1024;
  char outputFilename[outputFilenameLength];

  u8 * const bufferDataOriginal = buffer.data;

  static u8 bigBufferRaw2[BIG_BUFFER_SIZE];
  u8 * const shiftedBufferOriginal = reinterpret_cast<u8*>(malloc(BIG_BUFFER_SIZE));
  u8 * shiftedBuffer = shiftedBufferOriginal;

  const time_t t = time(0);   // get time now
  const struct tm * currentTime = localtime(&t);

  s32 frameNumber = 0;

  MemoryStack memory(bigBufferRaw2, BIG_BUFFER_SIZE, Flags::Buffer(false, true, false));

#ifdef PRINTF_ALL_RECEIVED
  //for(s32 i=0; i<buffer.dataLength; i++) {
  for(s32 i=0; i<500; i++) {
    printf("%x ", buffer.data[i]);
  }
#endif

  s32 bufferDataOffset = 0;
  bool atLeastOneHeaderFound = false;
  while(bufferDataOffset < buffer.dataLength)
  {
    s32 usbMessageStartIndex;
    s32 usbMessageEndIndex;
    FindUSBMessage(buffer.data+bufferDataOffset, buffer.dataLength-bufferDataOffset, usbMessageStartIndex, usbMessageEndIndex);
    
    if(usbMessageStartIndex < 0 || usbMessageEndIndex < 0) {
      if(atLeastOneHeaderFound) {
        printf("No more USB headers and footers were found, so returning\n");
      } else {
        printf("Error: USB header or footer is missing (%d,%d) in message of size %d bytes\n", usbMessageStartIndex, usbMessageEndIndex, buffer.dataLength);
      }

      buffer.data = NULL;
      free(bufferDataOriginal);
	  free(shiftedBufferOriginal);
      return;
    }

    atLeastOneHeaderFound = true;

    usbMessageStartIndex += bufferDataOffset;
    usbMessageEndIndex += bufferDataOffset;
    bufferDataOffset = usbMessageEndIndex;

    const s32 messageLength = usbMessageEndIndex-usbMessageStartIndex+1;

    // Move this to a memory-aligned start (index 0 of bigBuffer), after the first MemoryStack::HEADER_LENGTH
    //const s32 bigBuffer_alignedStartIndex = (RoundUp(reinterpret_cast<size_t>(buffer.data), MEMORY_ALIGNMENT) - reinterpret_cast<size_t>(buffer.data)) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH;
    shiftedBuffer = reinterpret_cast<u8*>( RoundUp(reinterpret_cast<size_t>(shiftedBufferOriginal), MEMORY_ALIGNMENT) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH );
    memcpy(shiftedBuffer, buffer.data+usbMessageStartIndex, messageLength);
    //buffer.data += bigBuffer_alignedStartIndex;

    if(swapEndianForHeaders) {
      for(s32 i=0; i<messageLength; i+=4) {
        Swap(shiftedBuffer[i], shiftedBuffer[i^3]);
        Swap(shiftedBuffer[(i+1)], shiftedBuffer[(i+1)^3]);
      }
    }

    SerializedBuffer serializedBuffer(shiftedBuffer, messageLength, Anki::Embedded::Flags::Buffer(false, true, true));

    SerializedBufferIterator iterator(serializedBuffer);

    while(iterator.HasNext()) {
      s32 dataLength;
      SerializedBuffer::DataType type;
      u8 * const dataSegmentStart = reinterpret_cast<u8*>(iterator.GetNext(swapEndianForHeaders, dataLength, type));
      u8 * dataSegment = dataSegmentStart;

      if(!dataSegment) {
        break;
      }

      printf("Next segment is (%d,%d): ", dataLength, type);
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
          SerializedBuffer::DeserializeArray(swapEndianForHeaders, swapEndianForContents, dataSegmentStart, dataLength, arr, memory);

          snprintf(&outputFilename[0], outputFilenameLength, outputFilenamePattern.data(),
            currentTime->tm_year+1900, currentTime->tm_mon+1, currentTime->tm_mday,
            currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec,
            frameNumber,
            "png");

          frameNumber++;

          printf("Saving to %s", outputFilename);
          const cv::Mat_<u8> &mat = arr.get_CvMat_();
          cv::imwrite(outputFilename, mat);
          //cv::imwrite("c:/tmp/tt.bmp", mat);
        }
      } else if(type == SerializedBuffer::DATA_TYPE_STRING) {
        printf("Board>> %s", dataSegment);
      }

      printf("\n");
    } // while(iterator.HasNext())
  } // while(bufferDataOffset < buffer.dataLength)

  buffer.data = NULL;
  free(bufferDataOriginal);
  free(shiftedBufferOriginal);
  return;
} // void SaveAndFreeBuffer()

// Based off example at http://msdn.microsoft.com/en-us/library/windows/desktop/ms682516(v=vs.85).aspx
DWORD WINAPI SaveBuffersThread(LPVOID lpParam)
{
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

  RawBuffer *buffer = (RawBuffer*)lpParam;

  SaveAndFreeBuffer(*buffer, string(outputFilenamePattern));

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
  double lastUpdateTime;
  Serial serial;
  const s32 USB_BUFFER_SIZE = 100000000;
  u8 *usbBuffer = reinterpret_cast<u8*>(malloc(USB_BUFFER_SIZE));
  s32 usbBufferIndex = 0;

  s32 comPort = 8;
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
          SaveAndFreeBuffer(rawBuffer, string(outputFilenamePattern));

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
#endif