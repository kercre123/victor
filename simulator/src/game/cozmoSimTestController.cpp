/*
 * File:          cozmoSimTestController.h
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"

#include <stdio.h>
#include <string.h>



namespace Anki {
  namespace Cozmo {
    
      // Private memers:
      namespace {

      } // private namespace

    
  
    CozmoSimTestController::CozmoSimTestController(s32 step_time_ms)
    : UiGameController(step_time_ms)
    {

    }
    
    CozmoSimTestController::~CozmoSimTestController()
    {
    }
    
  } // namespace Cozmo
} // namespace Anki
