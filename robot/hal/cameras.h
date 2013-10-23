#ifndef CAMERAS_H
#define CAMERAS_H

#include "anki/cozmo/robot/hal.h"
#include "movidius.h"

typedef enum
{
  YUV422i,
  YUV420p,
  YUV422p,
  YUV400p,
  RGBA8888,
  RGB888,
  LUT2,
  LUT4,
  LUT16,
  RAW16,
  NONE
} FrameType;

typedef struct
{
  FrameType type;
  u32 width;
  u32 height;
  u32 stride;
  u32 bytesPP;
  u32 referenceFrequency;
  u32 sensorI2CAddress;
  u32 registerCount;
  const unsigned short (*regValues)[2];
} CameraSpecification;

typedef struct
{
  u8* p1;
  u8* p2;
  u8* p3;
} FrameBuffer;

typedef void (*FrameReadyCallback)(FrameBuffer*);

typedef struct
{
  u32 sensorNumber;
  I2CM_Device* i2cHandle;
  FrameBuffer* currentFrame;
  CameraSpecification* camSpec;
  FrameReadyCallback cbFrameReady;
} CameraHandle;

void CameraInit(CameraHandle* handle, tyCIFDevice deviceType,
    I2CM_Device* i2cHandle, FrameBuffer* currentFrame, 
    CameraSpecification* camSpec, FrameReadyCallback cbFrameReady);

void CameraStart(CameraHandle* handle, int resetPin, bool isActiveLow, 
    u8 camWriteProto[]);

#endif

