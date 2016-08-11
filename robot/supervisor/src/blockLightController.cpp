/**
 * File: blockLightController.cpp
 *
 * Author: Kevin Yoon
 * Created: 09/24/2015
 *
 * Description:
 *
 *   Robot-side controller for commanding lights on the blocks
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "blockLightController.h"
#include "messages.h"
#include "clad/types/activeObjectTypes.h"
#include "anki/cozmo/robot/ledController.h"
#include "anki/cozmo/robot/hal.h"
#include <string.h>

namespace Anki {
namespace Cozmo {
namespace BlockLightController {
  namespace {

    // Assumes a block comms resolution of TIME_STEP
    const u8 MAX_NUM_CUBES = 4; // This whole module needs to be refactored. Hard code this for now

    // Parameters for each LED on each block
    LightState _ledParams[MAX_NUM_CUBES][NUM_CUBE_LEDS];
    // Keep track of where we are in the phase cycle for each LED
    TimeStamp_t _ledPhases[MAX_NUM_CUBES][NUM_CUBE_LEDS];

    // Messages to be sent to each block
    uint16_t _blockMsg[MAX_NUM_CUBES][NUM_CUBE_LEDS];
    
    struct Rotation
    {
      u8 rotationPeriod = 0;
      TimeStamp_t lastRotation = 0;
      u8 rotationOffset = 0;
    };
    
    Rotation _rotations[MAX_NUM_CUBES];

  }; // "private" variables


  Result SetLights(u8 blockID, const LightState *params, u8 rotationPeriod)
  {
    memcpy(_ledParams[blockID], params, sizeof(LightState)*NUM_CUBE_LEDS);
    memset(_ledPhases[blockID], 0, sizeof(TimeStamp_t)*NUM_CUBE_LEDS);

    _rotations[blockID].lastRotation = 0;
    _rotations[blockID].rotationPeriod = rotationPeriod;
    _rotations[blockID].rotationOffset = 0;
    
    return RESULT_OK;
  }

  void ApplyLEDParams(u8 blockID, TimeStamp_t currentTime)
  {
    bool blockLEDsUpdated = false;
    uint16_t* m = _blockMsg[blockID];
    
    if(_rotations[blockID].rotationPeriod > 0 &&
       TIMESTAMP_TO_FRAMES(currentTime - _rotations[blockID].lastRotation) > _rotations[blockID].rotationPeriod)
    {
      _rotations[blockID].lastRotation = currentTime;
      ++_rotations[blockID].rotationOffset;
      if(_rotations[blockID].rotationOffset >= NUM_CUBE_LEDS)
      {
        _rotations[blockID].rotationOffset = 0;
      }
    }

    for(u8 i=0; i<NUM_CUBE_LEDS; ++i) {
      u16 newColor;
      if (GetCurrentLEDcolor(_ledParams[blockID][i], currentTime, _ledPhases[blockID][i], newColor)) {
        m[(i + _rotations[blockID].rotationOffset) % NUM_CUBE_LEDS] = newColor;   // Currently, onColor is the only thing that matters in this message.
        blockLEDsUpdated = true;
      }
    }

    if (blockLEDsUpdated) {
      HAL::SetBlockLight(blockID, m);
    }
  }


  Result Update()
  {
    for (int i=0; i<MAX_NUM_CUBES; ++i) {
      // Apply LED settings to the block
      ApplyLEDParams(i, HAL::GetTimeStamp());
    }

    return RESULT_OK;
  }
} // namespace BlockLightController
} // namespace Cozmo
} // namespace Anki
