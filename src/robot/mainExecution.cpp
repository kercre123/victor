
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/cozmoTypes.h"

#include "anki/cozmo/robot/mainExecution.h"
#include "anki/cozmo/robot/wheelController.h"
#include "anki/cozmo/robot/steeringController.h"
#include "anki/cozmo/robot/vehicleSpeedController.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/cozmo/robot/localization.h"
#include "anki/cozmo/robot/hal.h"

#include "anki/cozmo/robot/debug.h"
#include <stdio.h>
 

#if(DEBUG_MAIN_EXECUTION)
#include "anki/cozmo/robot/cozmoBot.h"
extern CozmoBot gCozmoBot;

#define MAX_PATH_ERROR_TEXT_LENGTH 256
static char pathErrorStr[MAX_PATH_ERROR_TEXT_LENGTH+1];
#endif

     
void CozmoMainExecution(void) 
{  
  static s16 fidx = 0;
  static float pathDistErr = 0;
  static float radErr = 0;


  Localization::UpdateLocalization();

  // Figure out what fidx should be according to pathFollower
  if (PathFollower::IsTraversingPath()) {
    BOOL gotError = PathFollower::GetPathError(pathDistErr, radErr);
    if (gotError) {
      fidx = pathDistErr*1000; // Convert to mm
      printf("fidx: %d\n\n", fidx);
#if(DEBUG_MAIN_EXECUTION)
      snprintf(pathErrorStr, MAX_PATH_ERROR_TEXT_LENGTH, "PathError: %f m, %f rad  => fidx: %d", pathDistErr, radErr, fidx);
      gCozmoBot.SetOverlayText(OT_PATH_ERROR, pathErrorStr);
#endif
    } else {
      SetUserCommandedDesiredVehicleSpeed(0);
    }
  }


  // Speed controller
  ManageVehicleSpeedController();


  // Steering controller
  ManageSteeringController(fidx, radErr);

  
  s16 wspeedlPWM = 0; s16 wspeedrPWM = 0;
  //Run one iteration of the encoder speed filter
  EncoderSpeedFilterIteration();
  // Wheel controller
  ManageWheelSpeedController(&wspeedlPWM, &wspeedrPWM);

}