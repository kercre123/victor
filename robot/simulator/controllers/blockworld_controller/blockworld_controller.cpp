/*
 * File:          blockworld_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include <webots/Supervisor.hpp>

#include "anki/common/basestation/general.h"
#include "anki/cozmo/basestation/basestation.h"
#include "anki/cozmo/robot/cozmoConfig.h"


webots::Supervisor basestationController;

using namespace Anki;
using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  BasestationMain bs;
  BasestationStatus status = bs.Init(nullptr, BasestationMode::BM_DEFAULT);
  if (status != BS_OK) {
    PRINT_NAMED_ERROR("Basestation.Init.Fail","status %d\n", status);
  }
  
  //
  // Main Execution loop: step the world forward forever
  //
  while (basestationController.step(BS_TIME_STEP) != -1)
  {
    status = bs.Update(SEC_TO_NANOS(basestationController.getTime()));
    if (status != BS_OK) {
      PRINT_NAMED_WARNING("Basestation.Update.NotOK","status %d\n", status);
    }
    
  } // while still stepping
  
  return 0;
}

