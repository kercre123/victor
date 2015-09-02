#ifndef COZMO_TYPES_H
#define COZMO_TYPES_H

#include "anki/common/types.h"

// Shared types between basestation and robot

namespace Anki {
  namespace Cozmo {

    // For DEV only
    typedef struct {
      s32 autoexposureOn;
      f32 exposureTime;      
      s32 integerCountsIncrement;
      f32 minExposureTime;
      f32 maxExposureTime;
      f32 percentileToMakeHigh;
      s32 limitFramerate;
      u8 highValue;
    } VisionSystemParams_t;

    typedef struct {
      f32 scaleFactor;
      s32 minNeighbors;
      s32 minObjectHeight;
      s32 minObjectWidth;
      s32 maxObjectHeight;
      s32 maxObjectWidth;
    } FaceDetectParams_t;

    
    typedef enum {
      SAVE_OFF = 0,
      SAVE_ONE_SHOT,
      SAVE_CONTINUOUS
    } SaveMode_t;
    
  }
}

#endif // COZMO_TYPES_H
