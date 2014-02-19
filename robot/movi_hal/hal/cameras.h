#ifndef CAMERAS_H
#define CAMERAS_H

#include "anki/cozmo/robot/hal.h"
#include "movidius.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      typedef enum
      {
        BAYER,
        BAYER_TO_Y
      } FrameType;

      typedef struct
      {
        u32 count;
        u16* registers;
      } CameraRegisters;

      typedef struct
      {
        FrameType type;
        u32 bytesPP;
        u32 referenceFrequency;
        u32 sensorI2CAddress;
        CameraRegisters registerValues[CAMERA_MODE_COUNT];
        u8* writePrototype;
      } CameraSpecification;

      typedef struct
      {
        CameraID cameraID;
        I2CM_Device* i2cHandle;
        CameraSpecification camSpec;
      } CameraHandle;

      void CameraInit(CameraHandle* handle, CameraID cameraID,
          FrameType frameType, u32 bytesPP, u32 referenceFrequency,
          u32 sensorI2CAddress, I2CM_Device* i2cHandle, const u16* regVGA,
          u32 regCGACount, const u16* regQVGA, u32 regQVGACount,
          const u16* regQQVGA, u32 regQQVGACount, u32 resetPin,
          bool isActiveLow, u8 camWritePrototype[]);
    }
  }
}

#endif

