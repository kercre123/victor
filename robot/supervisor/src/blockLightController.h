/**
 * File: blockLightController.h
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

#ifndef COZMO_BLOCK_LIGHT_CONTROLLER_H_
#define COZMO_BLOCK_LIGHT_CONTROLLER_H_

#include "anki/types.h"
#include "clad/types/ledTypes.h"

namespace Anki {
namespace Cozmo {
namespace BlockLightController {
  
  Result Update();

  // Register a block so that lights can be set on it
  Result RegisterBlock(u8 blockID);
  Result DeregisterBlock(u8 blockID);
  bool IsRegisteredBlock(u8 blockID);
  
  // Set lights for a blockID.
  Result SetLights(u8 blockID, LightState *params);
  
  
} // namespace BlockLightController
} // namespace Cozmo
} // namespace Anki

#endif // COZMO_BLOCK_LIGHT_CONTROLLER_H_
