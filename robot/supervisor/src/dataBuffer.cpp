/**
File: dataBuffer.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/cozmo/robot/hal.h"

#if USE_OFFBOARD_VISION

#define BUFFER_SIZE 50000000

static s32 imageNumber = 0;

__attribute__((section(".ddr.bss"))) static char buffer[BUFFER_SIZE] __attribute__ ((aligned (MEMORY_ALIGNMENT_RAW)));

using namespace Anki::Embedded;

static bool memoryInitialized = false;
static MemoryStack memory;

namespace Anki
{
  namespace Cozmo
  {
    namespace DataBuffer
    {
      const u32 headerSize_basic = 8;
      const u32 headerSize_image = headerSize_basic + 16;

      void InitializeDataBuffer()
      {
        memory = MemoryStack(buffer, BUFFER_SIZE, Flags::Buffer(false,true));
        memoryInitialized = true;
      }

      Result Clear()
      {
        if(!memoryInitialized) {
          InitializeDataBuffer();
        }

        imageNumber = 0;
      }

      Result PushBackImage(const u8 * image, const HAL::CameraMode resolution, const TimeStamp timestamp)
      {
        if(!memoryInitialized) {
          InitializeDataBuffer();
        }

        const u32 height = HAL::CameraModeInfo[resolution].height;
        const u32 width = HAL::CameraModeInfo[resolution].width;
        const s32 bytesToCopy = height*width;

        void * dataLocation = memory.Allocate(bytesToCopy + headerSize_image);

        if(!dataLocation) {
          return RESULT_FAIL;
        }

        reinterpret_cast<u32*>(&videoBuffer[bytesUsed])[0] = BUFFERED_DATA_IMAGE;
        reinterpret_cast<u32*>(&videoBuffer[bytesUsed+4])[0] = bytesToCopy + 16;

        reinterpret_cast<u32*>(&videoBuffer[bytesUsed+8])[0] = width;
        reinterpret_cast<u32*>(&videoBuffer[bytesUsed+12])[0] = height;
        reinterpret_cast<u32*>(&videoBuffer[bytesUsed+16])[0] = imageNumber;
        reinterpret_cast<u32*>(&videoBuffer[bytesUsed+20])[0] = timestamp;

        memcpy(videoBuffer + bytesUsed + 24, image, bytesToCopy);

        bytesUsed += 24 + bytesToCopy;
        bytesUsed = RoundUp(bytesUsed, MEMORY_ALIGNMENT);
      }

      Result SendBuffer()
      {
        if(!memoryInitialized) {
          InitializeDataBuffer();
        }

        HAL::USBSendDataBuffer(&buffer[0], bytesUsed);

        Clear();
      }
    } // namespace VideoBuffer
  } // namespace Cozmo
} // namespace Anki

#endif // #if USE_OFFBOARD_VISION