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
    
    // Whether or not a blockID is registered
    bool _validBlock[MAX_NUM_CUBES] = {false};
    u8 _numValidBlocks = 0;
    
  }; // "private" variables
  
  
  bool IsRegisteredBlock(u8 blockID)
  {
    return (blockID < MAX_NUM_CUBES) && _validBlock[blockID];
  }
  
  Result RegisterBlock(u8 blockID)
  {
    // Check if ID out of range or already registered
    if (blockID >= MAX_NUM_CUBES || _validBlock[blockID]) {
      return RESULT_FAIL;
    }
    
    // Register block ID
    _validBlock[blockID] = true;
    ++_numValidBlocks;

    return RESULT_OK;
  }
  
  Result DeregisterBlock(u8 blockID)
  {
    // Check if ID out of range or already deregistered
    if (blockID >= MAX_NUM_CUBES || !_validBlock[blockID]) {
      return RESULT_FAIL;
    }
    
    // Deregister block ID
    _validBlock[blockID] = false;
    --_numValidBlocks;
    
    return RESULT_OK;
  }
  
  
  
  
  Result SetLights(u8 blockID, LightState *params)
  {
    if (IsRegisteredBlock(blockID)) {
      for (u8 i=0; i < NUM_CUBE_LEDS; ++i) {
        _ledParams[blockID][i] = params[i];
        _ledParams[blockID][i].nextSwitchTime = 0;
        _ledParams[blockID][i].state = LED_STATE_OFF;
      }
    } else {
      PRINT("ERROR(BlockLightController::SetLights): Invalid blockID %d", blockID);
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  }
  
  Result ApplyLEDParams(u8 blockID, TimeStamp_t currentTime)
  {
    if (IsRegisteredBlock(blockID)) {
      
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
      
      return RESULT_OK;
      
    }
    return RESULT_FAIL;
  } // ApplyLEDParams()

  
  Result Update()
  {
    // This code was simplified to ensure block updates run at a constant speed
    // Registering and unregistering blocks already happens in the body, so doing it here is redundant!
    if (++_nextBlockToUpdate >= MAX_NUM_CUBES)
      _nextBlockToUpdate = 0;

    if (IsRegisteredBlock(_nextBlockToUpdate)) {
      // Apply LED settings to the block
      if (ApplyLEDParams(_nextBlockToUpdate, HAL::GetTimeStamp()) != RESULT_OK) {
        PRINT("ERROR(BlockLightController.Update): Failed to apply LED params to block %d", _nextBlockToUpdate);
        return RESULT_FAIL;
      }
    }
    
    return RESULT_OK;
  }
} // namespace BlockLightController
} // namespace Cozmo
} // namespace Anki
