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
#include "anki/cozmo/shared/activeBlockTypes.h"
#include "anki/cozmo/robot/ledController.h"
#include "anki/cozmo/robot/hal.h"

namespace Anki {
namespace Cozmo {
namespace BlockLightController {
  namespace {
    
    // Update period for a single block.
    const u32 BLOCK_LIGHT_UPDATE_PERIOD_MS = 35;
    TimeStamp_t _nextUpdateCycleStartTime = 0;
    
    // The next block to have its lights updated
    u8 _nextBlockToUpdate = 0;

    // Assumes a block comms resolution of TIME_STEP
    const u32 MAX_NUM_BLOCKS = BLOCK_LIGHT_UPDATE_PERIOD_MS / TIME_STEP;
    
    // Parameters for each LED on each block
    LEDParams_t _ledParams[MAX_NUM_BLOCKS][NUM_BLOCK_LEDS];
    
    // Messages to be sent to each block
    // TODO: The structure of this message will change to something simpler.
    Messages::SetBlockLights _blockMsg[MAX_NUM_BLOCKS];
    
    // Whether or not a blockID is registered
    bool _validBlock[MAX_NUM_BLOCKS] = {false};
    u8 _numValidBlocks = 0;
    
  }; // "private" variables
  
  
  bool IsRegisteredBlock(u8 blockID)
  {
    return (blockID < MAX_NUM_BLOCKS) && _validBlock[blockID];
  }
  
  // Returns the next valid blockID following the given blockID.
  Result GetNextValidBlock(const u8 currBlockID, u8 &nextBlockID)
  {
    // Check if ID is out of range and if there are any valid blocks at all
    if (currBlockID >= MAX_NUM_BLOCKS || _numValidBlocks == 0) {
      return RESULT_FAIL;
    }
    
    // Find next valid block
    u8 candidateBlockID = currBlockID + 1;
    for (u8 i=candidateBlockID; i < MAX_NUM_BLOCKS; ++i) {
      if (_validBlock[i]) {
        nextBlockID = i;
        return RESULT_OK;
      }
    }
    for (u8 i=0; i <= currBlockID; ++i) {
      if (_validBlock[i]) {
        nextBlockID = i;
        return RESULT_OK;
      }
    }

    return RESULT_FAIL;
  }
  
  
  Result RegisterBlock(u8 blockID)
  {
    // Check if ID out of range or already registered
    if (blockID >= MAX_NUM_BLOCKS || _validBlock[blockID]) {
      return RESULT_FAIL;
    }
    
    // Register block ID
    _validBlock[blockID] = true;
    ++_numValidBlocks;
    
    // If the current nextBlockToUpdate is invalid, set it to this block.
    if (!_validBlock[_nextBlockToUpdate]) {
      _nextBlockToUpdate = blockID;
    }
    
    return RESULT_OK;
  }
  
  Result DeregisterBlock(u8 blockID)
  {
    // Check if ID out of range or already deregistered
    if (blockID >= MAX_NUM_BLOCKS || !_validBlock[blockID]) {
      return RESULT_FAIL;
    }
    
    // Deregister block ID
    _validBlock[blockID] = false;
    --_numValidBlocks;
    
    // Update nextBlockToUpdate to the next valid block
    if (_nextBlockToUpdate == blockID) {
      GetNextValidBlock(_nextBlockToUpdate, _nextBlockToUpdate);
    }
    
    return RESULT_OK;
  }
  
  
  
  
  Result SetLights(u8 blockID, LEDParams_t *params)
  {
    if (IsRegisteredBlock(blockID)) {
      for (u8 i=0; i < NUM_BLOCK_LEDS; ++i) {
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
      Messages::SetBlockLights &m = _blockMsg[blockID];
      
      for(u8 i=0; i<NUM_BLOCK_LEDS; ++i) {
        u32 newColor;
        if (GetCurrentLEDcolor(_ledParams[blockID][i], currentTime, newColor)) {
          m.onColor[i] = newColor;   // Currently, onColor is the only thing that matters in this message.
          blockLEDsUpdated = true;
        }
      }
      
      if (blockLEDsUpdated) {
        HAL::SetBlockLight(blockID,
                           m.onColor, m.offColor,
                           m.onPeriod_ms, m.offPeriod_ms,
                           m.transitionOnPeriod_ms, m.transitionOffPeriod_ms);
      }
      
      return RESULT_OK;
      
    }
    return RESULT_FAIL;
  } // ApplyLEDParams()

  
  Result Update()
  {
    if (_numValidBlocks == 0) {
      return RESULT_OK;
    }
    
    TimeStamp_t currTime = HAL::GetTimeStamp();
    if (currTime >= _nextUpdateCycleStartTime) {
      
      // Apply LED settings to the block
      if (ApplyLEDParams(_nextBlockToUpdate, currTime) != RESULT_OK) {
        PRINT("ERROR(BlockLightController.Update): Failed to apply LED params to block %d", _nextBlockToUpdate);
        return RESULT_FAIL;
      }

      // Get the next block that should be updated
      u8 nextValidBlockID;
      if (GetNextValidBlock(_nextBlockToUpdate, nextValidBlockID) != RESULT_OK) {
        PRINT("ERROR(BlockLightController.Update): NoValidBlock");
        return RESULT_FAIL;
      }
      
      // If we've looped back to the start of the block list,
      // update the nextUpdateCycleStartTime.
      if (nextValidBlockID <= _nextBlockToUpdate) {
        if (_nextUpdateCycleStartTime == 0) {
          _nextUpdateCycleStartTime = currTime;
        } else {
          _nextUpdateCycleStartTime += BLOCK_LIGHT_UPDATE_PERIOD_MS;
        }
      }
      _nextBlockToUpdate = nextValidBlockID;
      
    }
    
    return RESULT_OK;
  }
  
  
  
} // namespace BlockLightController
} // namespace Cozmo
} // namespace Anki
