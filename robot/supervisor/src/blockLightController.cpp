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
    // Keep track of where we are in the fase cycle for each LED
    TimeStamp_t _ledPhases[MAX_NUM_CUBES][NUM_CUBE_LEDS];

    // Messages to be sent to each block
    uint16_t _blockMsg[MAX_NUM_CUBES][NUM_CUBE_LEDS];

  }; // "private" variables


  Result SetLights(u8 blockID, const LightState *params)
  {
    memcpy(_ledParams[blockID], params, sizeof(LightState)*NUM_CUBE_LEDS);
    return RESULT_OK;
  }

  void ApplyLEDParams(u8 blockID, TimeStamp_t currentTime)
  {
    bool blockLEDsUpdated = false;
    uint16_t* m = _blockMsg[blockID];

    for(u8 i=0; i<NUM_CUBE_LEDS; ++i) {
      u16 newColor;
      if (GetCurrentLEDcolor(_ledParams[blockID][i], currentTime, _ledPhases[blockID][i], newColor)) {
        m[i] = newColor;   // Currently, onColor is the only thing that matters in this message.
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
