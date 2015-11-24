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

namespace Anki {
namespace Cozmo {
namespace BlockLightController {
  namespace {
    
    // The next block to have its lights updated
    u8 _nextBlockToUpdate = 0;

    // Assumes a block comms resolution of TIME_STEP
    const u32 MAX_NUM_CUBES= 4; // This whole module needs to be refactored. Hard code this for now
    
    // Parameters for each LED on each block
    LightState _ledParams[MAX_NUM_CUBES][NUM_CUBE_LEDS];
    
    // Messages to be sent to each block
    // TODO: The structure of this message will change to something simpler.
    CubeLights _blockMsg[MAX_NUM_CUBES];
    
  }; // "private" variables
  
  
  Result SetLights(u8 blockID, const LightState *params)
  {
    for (u8 i=0; i < NUM_CUBE_LEDS; ++i) {
      _ledParams[blockID][i] = params[i];
      _ledParams[blockID][i].nextSwitchTime = 0;
      _ledParams[blockID][i].state = LED_STATE_OFF;
    }
    return RESULT_OK;
  }
  
  void ApplyLEDParams(u8 blockID, TimeStamp_t currentTime)
  {
    bool blockLEDsUpdated = false;
    CubeLights &m = _blockMsg[blockID];
    
    for(u8 i=0; i<NUM_CUBE_LEDS; ++i) {
      u32 newColor;
      if (GetCurrentLEDcolor(_ledParams[blockID][i], currentTime, newColor)) {
        m.lights[i].onColor = newColor;   // Currently, onColor is the only thing that matters in this message.
        blockLEDsUpdated = true;
      }
    }
    
    if (blockLEDsUpdated) {
      HAL::SetBlockLight(blockID, m.lights);
    }
  }

  
  Result Update()
  {
    if (++_nextBlockToUpdate >= MAX_NUM_CUBES)
      _nextBlockToUpdate = 0;
    
    // Apply LED settings to the block
    ApplyLEDParams(_nextBlockToUpdate, HAL::GetTimeStamp());
    
    return RESULT_OK;
  }
} // namespace BlockLightController
} // namespace Cozmo
} // namespace Anki
